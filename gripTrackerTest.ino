#include <WiFi.h>
#include <time.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include "HX711.h"

// Load cell setup
#define LOADCELL_DOUT_PIN  4
#define LOADCELL_SCK_PIN   6
HX711 scale;

WebServer server(80);

String currentFileName;
float hangStartTime = 0.0;
bool hangInProgress = false;
bool currentlyHanging = false;

struct HangData {
    float startTime;
    float endTime;
    std::vector<String> dataLines;
};

HangData currentHang;

const int lowForceThresholdCount = 5;
const int consistentForceDuration = 300; // 300 milliseconds
unsigned long lastForceTime = 0; // Timestamp when the force was last above threshold
float lastForce = 0; // Last force value that was above the threshold
const float forceThreshold = 2.5; // Threshold for force
const int consecutiveReadingsThreshold = 5; // Number of consecutive readings to start logging
static int consecutiveAboveThresholdCount = 0; // Counter for consecutive readings above threshold
float lastWeight = 0;
float lastTwoWeights[2] = {0, 0}; // Array to store the last two weights
float lastTwoTimestamps[2] = {0, 0}; // Array to store the timestamps of the last two weights



// Setup

void setup() {
  Serial.begin(115200);
  Serial.println("GripTracker");

  // Set up as an Access Point
  const char* ap_ssid = "GripTracker"; // Name of the access point
  const char* ap_password = ""; // Password for the access point, can be empty for an open network

  // Create an Access Point
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("Access Point \"");
  Serial.print(ap_ssid);
  Serial.println("\" started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());


  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Scan for and delete any empty CSV files
  File root = SPIFFS.open("/");
  while (File file = root.openNextFile()) {
    String fileName = "/" + String(file.name()); // Ensure the filename starts with '/'
    if (fileName.endsWith(".csv") && file.size() == 0) {
      file.close(); // Close the file before attempting to delete it
      if (SPIFFS.remove(fileName)) {
        Serial.println("Deleted empty file: " + fileName);
      } else {
        Serial.println("Failed to delete empty file: " + fileName);
      }
    }
  }

  // Load cell setup
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(-4200.00);
  scale.tare();

  // Create a new file for this session
  currentFileName = createNewFile();

  // Start the server with new endpoint
  server.on("/", HTTP_GET, handleRoot);
  server.on("/delete", HTTP_GET, handleDelete);
  server.on("/rawdata", HTTP_GET, handleRawData);
  server.on("/create", HTTP_GET, handleCreate);
  server.on("/dataView", HTTP_GET, handleDataView);
//  server.on("/getTimerSettings", HTTP_GET, handleGetTimerSettings); //Timer dictionary is handled in JS, now.
  server.onNotFound(handleNotFound);
  server.on("/tare", HTTP_GET, handleTare);
  server.on("/forceData", HTTP_GET, handleForceData);
  server.on("/listCSV", HTTP_GET, handleListCSV);
//  server.on("/getHangSummary", HTTP_GET, handleGetHangSummary);
  server.on("/getRawData", HTTP_GET, handleGetRawData);
  server.on("/timers.html", HTTP_GET, handleTimers);
  server.on("/resume", HTTP_GET, handleResume);
  server.on("/currentFile", HTTP_GET, handleCurrentFile);



  server.serveStatic("/style.css", SPIFFS, "/style.css");
  server.serveStatic("/timer.js", SPIFFS, "/timer.js");

  server.begin();
  Serial.println("HTTP server started");
}



// Server Endpoint Handlers

void handleRoot() {
  if(SPIFFS.exists("/index.html")){
    File file = SPIFFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(404, "text/plain", "404: Not Found");
  }
}


void handleCreate() {
  String newFileName = "/" + server.arg("name") + ".csv";
  if (newFileName.length() > 1 && !SPIFFS.exists(newFileName)) {
    File file = SPIFFS.open(newFileName, FILE_WRITE);
    if (file) {
      file.close();
      currentFileName = newFileName; // Set the new file as the current file
      server.sendHeader("Location", "/", true); // Redirect to the main page
      server.send(302, "text/plain", "");
    } else {
      server.send(500, "text/plain", "Error creating file");
    }
  } else {
    server.send(400, "text/plain", "Invalid file name or file already exists");
  }
}

void handleDelete() {
  String fileToDelete = "/" + server.arg("file"); // Ensure the file path starts with a slash
  if (fileToDelete.length() > 1 && SPIFFS.exists(fileToDelete)) { // Check if file exists
    SPIFFS.remove(fileToDelete); // Delete the file
    Serial.println("File deleted: " + fileToDelete);
  } else {
    Serial.println("File not found: " + fileToDelete);
  }
  server.sendHeader("Location", "/", true); // Redirect back to the root page
  server.send(302, "text/plain", ""); // HTTP status code for redirection
}

void handleResume() {
  String fileToResume = "/" + server.arg("file");
  if (fileToResume.length() > 1 && SPIFFS.exists(fileToResume)) {
    currentFileName = fileToResume;
    server.send(200, "text/plain", "Resumed session with file: " + fileToResume);
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

void handleCurrentFile() {
  if (currentFileName.length() > 0) {
    server.send(200, "text/plain", currentFileName);
  } else {
    server.send(200, "text/plain", "None");
  }
}

void handleRawData() {
  String fileName = "/" + server.arg("file");
  if (SPIFFS.exists(fileName)) {
    if(SPIFFS.exists("/rawData.html")){
      File file = SPIFFS.open("/rawData.html", "r");
      server.streamFile(file, "text/html");
      file.close();
    } else {
      server.send(404, "text/plain", "404: Not Found");
    }
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

void handleGetRawData() {
  String fileName = "/" + server.arg("file");
  if (SPIFFS.exists(fileName)) {
    File file = SPIFFS.open(fileName, "r");
    if (!file) {
      server.send(500, "text/plain", "Error opening file");
      return;
    }
    server.streamFile(file, "text/plain");
    file.close();
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

void handleDataView() {
  String fileName = "/" + server.arg("file");
  if (SPIFFS.exists(fileName)) {
    if(SPIFFS.exists("/dataView.html")){
      File file = SPIFFS.open("/dataView.html", "r");
      server.streamFile(file, "text/html");
      file.close();
    } else {
      server.send(404, "text/plain", "404: Not Found");
    }
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

void handleTimers() {
  if(SPIFFS.exists("/timers.html")) {
    File file = SPIFFS.open("/timers.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(404, "text/plain", "404: Not Found");
  }
}

void handleListCSV() {
  File root = SPIFFS.open("/");
  String jsonResponse = "[";
  bool first = true;

  while (File file = root.openNextFile()) {
    String fileName = String(file.name());
    if (fileName.endsWith(".csv")) {
      if (!first) {
        jsonResponse += ",";
      }
      jsonResponse += "\"" + fileName + "\"";
      first = false;
    }
  }
  jsonResponse += "]";
  server.send(200, "application/json", jsonResponse);
}

void handleTare() {
  scale.tare();
  server.send(200, "text/plain", "Scale Tared");
}

void handleForceData() {
  scale.set_scale(-4200.00);
  float weight = scale.get_units() * -1;

  String jsonResponse = "{\"force\": " + String(weight, 1) + "}";
  server.send(200, "application/json", jsonResponse);
}

void handleNotFound() {
  if (loadFromSPIFFS(server.uri())) return;
  server.send(404, "text/plain", "File Not Found");
}

bool loadFromSPIFFS(String path) {
  File file = SPIFFS.open(path, "r");
  if (!file) {
    return false;
  }


  String contentType = "text/plain";
  if (path.endsWith(".csv")) {
    contentType = "text/csv";
  }

  if (server.streamFile(file, contentType) != file.size()) {
    Serial.println("Sent less data than expected!");
  }
  file.close();
  return true;
}


// Utility Functions

String createNewFile() {
  int maxSessionNumber = 0;

  // Scan existing files to find the highest session number
  File root = SPIFFS.open("/");
  while (File file = root.openNextFile()) {
    String fileName = file.name();

    // Check if the file is a session file
    if (fileName.startsWith("/Set-") && fileName.endsWith(".csv")) {
      int sessionNumber = fileName.substring(9, fileName.length() - 4).toInt();
      if (sessionNumber > maxSessionNumber) {
        maxSessionNumber = sessionNumber; // Update the max session number
      }
    }
  }

  // Find a unique session number for the new file
  String newFileName;
  do {
    newFileName = "/Set-" + String(++maxSessionNumber) + ".csv";
  } while (SPIFFS.exists(newFileName)); // Check if the new file name exists

  // Attempt to create the new file
  File newFile = SPIFFS.open(newFileName, FILE_WRITE);
  if (newFile) {
    Serial.println("Created new file: " + newFileName);
    newFile.close();
    return newFileName;
  } else {
    Serial.println("Failed to create file: " + newFileName);
    return "";
  }
}

void logWeight(float weight, float timestamp) {
    if (!hangInProgress) {
        hangInProgress = true;
        currentHang.startTime = timestamp;
        currentHang.dataLines.clear();
    }
    currentHang.dataLines.push_back(String(timestamp - currentHang.startTime, 2) + "," + String(weight));
}


void markNewHangSession() {
    if (hangInProgress) {
        currentHang.endTime = millis() / 1000.0;
        if (currentHang.endTime - currentHang.startTime >= 1.0) {
            // Write hang data to file
            File file = SPIFFS.open(currentFileName, FILE_APPEND);
            if (file) {
                for (const auto& line : currentHang.dataLines) {
                    file.println(line);
                }
                file.println("0,NaN"); // Mark end of hang
                file.close();
            }
        }
        // Reset hang data
        currentHang.dataLines.clear();
        hangInProgress = false;
    }
}


// Loop Function

void loop() {
    float weight = scale.get_units() * -1;
    float currentTimestamp = millis() / 1000.0;

    // Update the readings
    lastTwoWeights[0] = lastTwoWeights[1];
    lastTwoTimestamps[0] = lastTwoTimestamps[1];
    lastTwoWeights[1] = weight;
    lastTwoTimestamps[1] = currentTimestamp;

    // Check for hang session start
    if ((abs(lastTwoWeights[0]) > forceThreshold || abs(lastTwoWeights[1]) > forceThreshold) && !hangInProgress) {
        hangInProgress = true;
        currentHang.startTime = lastTwoTimestamps[0];
        currentHang.dataLines.clear();
        logWeight(lastTwoWeights[0], lastTwoTimestamps[0]);
    }
    
    // Continue logging if in a hang session
    if (hangInProgress) {
        logWeight(weight, currentTimestamp);

        // Check for hang session end
        if (abs(weight) <= forceThreshold) {
            float currentEndTime = millis() / 1000.0;
            if (currentEndTime - currentHang.startTime >= 1.0) {
                markNewHangSession();
            } else {
                currentHang.dataLines.clear();
            }
            hangInProgress = false;
        }
    }

    server.handleClient();
    delay(10);
}
