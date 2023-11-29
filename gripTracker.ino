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

void setup() {
  Serial.begin(9600);
  Serial.println("GripTracker");

  // Set up as an Access Point
  const char* ap_ssid = "GripTrainer"; // Name of the access point
  const char* ap_password = "12345678"; // Password for the access point, can be empty for an open network

  // Create an Access Point
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("Access Point \"");
  Serial.print(ap_ssid);
  Serial.println("\" started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  configTime(0, 0, "pool.ntp.org"); // Configure NTP client

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
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
  server.on("/getTimerSettings", HTTP_GET, handleGetTimerSettings);
  server.onNotFound(handleNotFound);
  server.on("/tare", HTTP_GET, handleTare);

  server.serveStatic("/style.css", SPIFFS, "/style.css");
  server.serveStatic("/timer.js", SPIFFS, "/timer.js");
  
  server.begin();
  Serial.println("HTTP server started");
}


void handleTare() {
    scale.tare();
    server.send(200, "text/plain", "Scale Tared");
}


void handleGetTimerSettings() {
  String protocol = server.arg("protocol");

  int repAmount, repTime, repRest, setAmount, setRest;
  if (protocol == "maxHangs") {
    repAmount = 1; repTime = 8; repRest = -1; setAmount = 6; setRest = 180;
  } else if (protocol == "noHang") {
    repAmount = 12; repTime = 10; repRest = 50; setAmount = 1; setRest = -1;
  } else { // Default to Repeaters
    repAmount = 6; repTime = 7; repRest = 3; setAmount = 6; setRest = 180;
  }

  String jsonResponse = "{\"repAmount\": " + String(repAmount) + ", \"repTime\": " + String(repTime) + ", \"repRest\": " + String(repRest) + ", \"setAmount\": " + String(setAmount) + ", \"setRest\": " + String(setRest) + "}";
  server.send(200, "application/json", jsonResponse);
}



void handleDataView() {
  String fileName = "/" + server.arg("file");
  if (SPIFFS.exists(fileName)) {
    // Open the file to read data for the chart
    File file = SPIFFS.open(fileName, "r");
    if (!file) {
      server.send(500, "text/plain", "Error opening file: " + fileName);
      return;
    }


    String html = "<!DOCTYPE html><html><head><link rel='stylesheet' type='text/css' href='/style.css'>";
    html += R"(
</head>

<body>
  <!-- Include Google Charts Library -->
  <script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>
  <script type='text/javascript'>
    google.charts.load('current', {'packages':['corechart']});
    google.charts.setOnLoadCallback(drawChart);

    // JavaScript function to plot the graph
    function drawChart() {
      var data = new google.visualization.DataTable();
      data.addColumn('number', 'Time');
      data.addColumn('number', 'Force');
      data.addRows([
)";

    // Add data rows
    String tableData = "<table border='1'><tr><th>Time (s)</th><th>Force (lbs)</th></tr>";
    bool firstLine = true;
    while (file.available()) {
      String line = file.readStringUntil('\n');
      int commaIndex = line.indexOf(',');
      String timeString = line.substring(0, commaIndex);
      String forceString = line.substring(commaIndex + 1);
      if (!firstLine) {
        html += ",";
      }
      html += "[" + timeString + ", " + forceString + "]";
      tableData += "<tr><td>" + timeString + "</td><td>" + forceString + "</td></tr>";
      firstLine = false;
    }
    file.close();

html += R"(
]);

var options = {
  explorer: {
    actions: ['dragToZoom', 'rightClickToReset'],
    axis: 'horizontal',
    keepInBounds: true,
    maxZoomIn: 10,   // You can adjust this value as needed
    maxZoomOut: 1     // You can adjust this value as needed
  },
  chartArea: {
    left: 0,
    top: 0,
    width: '100%', // Set the width to 100% to fill the container
    height: '100%' // Set the height to 100% to fill the container
  },
  legend: {
    position: 'none' // This will remove the legend entirely
  }
};

var chart = new google.visualization.ScatterChart(document.getElementById('chart_div'));
chart.draw(data, options);
}

</script>
</head><body>

<h1>Data Analysis</h1>
<div id='chart_div' style='width: 900px; height: 500px;'></div> <!-- Chart container -->
)";

   
   
    file.close(); // Close the file after reading data for the chart


    // Reopen the file to calculate the summary
    file = SPIFFS.open(fileName, "r");
    if (!file) {
      server.send(500, "text/plain", "Error opening file for summary: " + fileName);
      return;
    }
    // Calculate the summary
    String summary = calculateHangSummary(file);
    file.close(); // Close the file after calculating the summary


    // Append summary to HTML
    html += "<h2>Hang Data</h2><pre>" + summary + "</pre>";


    html += "<br><a href='/'>Back to Main Page</a>";


    // Closing HTML tags and send response
    html += "<p>---</p><br><p>Made by Ted Bergstrand - 2023</p><br></body></html>";
    server.send(200, "text/html", html);
  } else {
    server.send(404, "text/plain", "File not found");
  }
}




void handleRawData() {
  String fileName = "/" + server.arg("file"); // Ensure the file path starts with a slash
  if (fileName.length() > 1) {
    File file = SPIFFS.open(fileName, "r");
    if (!file) {
      server.send(500, "text/plain", "Error opening file: " + fileName);
      return;
    }


   String html = "<!DOCTYPE html><html><head><link rel='stylesheet' type='text/css' href='/style.css'><meta name='viewport' content='width=device-width, initial-scale=1'>";


  html += "</head><body><h1>Raw Data</h1><table border='1'><tr><th>Time (s)</th><th>Force (lbs)</th></tr>";


    while (file.available()) {
      String line = file.readStringUntil('\n');
      int commaIndex = line.indexOf(',');
      if (commaIndex != -1) {
        String timeString = line.substring(0, commaIndex);
        String forceString = line.substring(commaIndex + 1);
        html += "<tr><td>" + timeString + "</td><td>" + forceString + "</td></tr>";
      }
    }


    html += "</table><br><a href='/'>Back to Main Page</a><br><p>---</p><br><p>Made by Ted Bergstrand - 2023</p><br></body></html>";


    server.send(200, "text/html", html);
    file.close();
  } else {
    server.send(400, "text/plain", "No file specified");
  }
}








String calculateHangSummary(File &file) {
  // Start the HTML table with headers
  String hangSummary = "<table id='hangAnalysisTable' border='1'><tr><th>Hang</th><th>Duration (s)</th><th>Peak Force (lbs)</th><th>Average Force (lbs)</th><th>Std Dev (lbs)</th></tr>";

  // Initialize variables for hang data analysis
  int hangCount = 1; // Start hang counter at 1
  float peakForce = 0.0, totalForce = 0.0, avgForce = 0.0, hangDuration = 0.0, stdDev = 0.0;
  int countReadings = 0;
  bool newHang = true;
  float startTime = 0.0, endTime = 0.0, currentTime = 0.0;
  bool hangInProgress = false;
  std::vector<float> forces;

  // Read each line in the file
  while (file.available()) {
    String line = file.readStringUntil('\n');
    int commaIndex = line.indexOf(',');
    String timeString = line.substring(0, commaIndex);
    String forceString = line.substring(commaIndex + 1);

    // Process valid data entries
    if (forceString != "NaN" && timeString != "0") {
      float force = forceString.toFloat();
      currentTime = timeString.toFloat();

      // Start a new hang or continue the current hang
      if (newHang) {
        // If a hang is already in progress, calculate summary for it
        if (hangInProgress) {
          hangDuration = endTime - startTime;
          avgForce = totalForce / countReadings;
          stdDev = calculateStdDev(forces, avgForce);
          hangSummary += "<tr><td>" + String(hangCount) + "</td><td>" + String(hangDuration, 2) + "</td><td>" + String(peakForce) + "</td><td>" + String(avgForce) + "</td><td>" + String(stdDev) + "</td></tr>";
          hangCount++;
          forces.clear();
        }

        // Initialize new hang
        startTime = currentTime;
        peakForce = force;
        totalForce = force;
        countReadings = 1;
        newHang = false;
        hangInProgress = true;
        forces.push_back(force);
      } else {
        // Continue current hang
        peakForce = max(peakForce, force);
        totalForce += force;
        countReadings++;
        forces.push_back(force);
      }

      endTime = currentTime;
    } else {
      // Mark the end of a hang
      newHang = true;
    }
  }

  // Add summary for the last hang, if it was in progress
  if (hangInProgress) {
    hangDuration = endTime - startTime;
    avgForce = totalForce / countReadings;
    stdDev = calculateStdDev(forces, avgForce);
    hangSummary += "<tr><td>" + String(hangCount) + "</td><td>" + String(hangDuration, 2) + "</td><td>" + String(peakForce) + "</td><td>" + String(avgForce) + "</td><td>" + String(stdDev) + "</td></tr>";
  }

  // Close the HTML table
  hangSummary += "</table>";
  return hangSummary;
}




float calculateStdDev(const std::vector<float>& values, float mean) {
    float sum = 0.0;
    for (float value : values) {
        sum += pow(value - mean, 2);
    }
    return sqrt(sum / values.size());
}












bool currentlyHanging = false; // To track if the user is currently hanging




// Global variable to count consecutive low-force readings
int lowForceReadingsCount = 0;
const int lowForceThresholdCount = 5; // Number of readings to confirm end of hang


// Define a constant for the minimum duration a force must be consistent
const int consistentForceDuration = 300; // 300 milliseconds
unsigned long lastForceTime = 0; // Timestamp when the force was last above threshold
float lastForce = 0; // Last force value that was above the threshold

const float forceThreshold = 2.5; // Threshold for force
const int consecutiveReadingsThreshold = 5; // Number of consecutive readings to start logging
static int consecutiveAboveThresholdCount = 0; // Counter for consecutive readings above threshold

float lastWeight = 0;

void loop() {
    scale.set_scale(-4200.00);
    float weight = scale.get_units() * -1;

    // Use the direct reading without filtering
    Serial.print("Reading: ");
    Serial.print(weight, 1);
    Serial.print(" lbs");
    Serial.println();

    // Logic for handling the force reading
    if (abs(weight) > forceThreshold) {
        consecutiveAboveThresholdCount++;

        // Log the weight
        logWeight(weight);

        // Start a new hang session if sustained force is detected
        if (!currentlyHanging && consecutiveAboveThresholdCount >= consecutiveReadingsThreshold) {
            currentlyHanging = true;
            // Optionally, log a new hang session start marker here
            // markNewHangSession(); // Uncomment if needed
        }
    } else {
        // Reset the counter and flags if force is below the threshold
        consecutiveAboveThresholdCount = 0;
        if (currentlyHanging) {
            currentlyHanging = false;
            markNewHangSession();
        }
    }

    server.handleClient();
    delay(100);
}










void logWeight(float weight) {
  File file = SPIFFS.open(currentFileName, FILE_APPEND);
  if (file) {
    float timestampInSeconds = millis() / 1000.0; // Convert milliseconds to seconds
    String formattedTimestamp = String(timestampInSeconds, 2); // Format with 2 decimal places
    file.print(formattedTimestamp);
    file.print(",");
    file.println(weight);
    file.close();
  }
}


void markNewHangSession() {
  File file = SPIFFS.open(currentFileName, FILE_APPEND);
  if (file) {
    file.println("0,NaN"); // Use 0 as a timestamp placeholder
    file.close();
  }
}


bool isFileEmpty(const String& fileName) {
  File file = SPIFFS.open(fileName, "r");
  if (!file) {
    return true; // File doesn't exist or couldn't be opened
  }


  bool isEmpty = file.size() == 0;
  file.close();
  return isEmpty;
}


String createNewFile() {
  // Set up time and get the current timestamp
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "";
  }


  // Format the new file name based on the current time with dashes
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%m-%d-%H-%M.csv", &timeinfo);
  String newFileName = "/" + String(buffer);


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


  // Create a new file with the current timestamp
  File newFile = SPIFFS.open(newFileName, FILE_WRITE);
  if (newFile) {
    newFile.close();
    return newFileName;
  } else {
    Serial.println("Failed to create file");
    return "";
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><link rel='stylesheet' type='text/css' href='/style.css'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += R"(
</head><body><script src="/timer.js"></script>
<h1>GripTracker</h1>
<form action='/create' method='get'>
New Session: <input type='text' placeholder='Enter Session Name' name='name'>
<input type='submit' id='newSessionButton' value='Create New Session'>
</form>
<div id='tareButton' class='button-container'><button onclick='tareScale()'>Tare Scale</button></div>
<br>

<h3>Hangboard Timer</h3>
<div id='timerDisplay'>00:00</div>
<div class=button-container>
<button onclick='startTimer()'>Start Timer</button>
<button onclick='stopTimer()'>Stop Timer</button>
</div>

<select id='protocolSelect' onchange='updateTimerSettings()'>
<option value='' disabled selected>Select an Exercise</option>
<option value='climbingRepeater'>Repeaters</option>
<option value='maxHangs'>Eva Lopez' MaxHangz</option>
<option value='noHang'>Emil Abrahamsson's No Hangs</option>
</select>



<h2>Sessions</h2>
<div class='session-container'>
)";



  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    html += "<p><a href=\"" + fileName + "\">" + fileName + "</a> | <a href=\"/dataView?file=" + fileName + "\">Data Analysis</a> | <a href=\"/rawdata?file=" + fileName + "\">Raw Data</a> |        <a href=\"/delete?file=" + fileName + "\">Delete</a></p>";
    file = root.openNextFile();
  }
html += R"(
</div>
<p>---</p><br><p>Made by Ted Bergstrand - 2023</p><br></body></html>
)";

  server.send(200, "text/html", html);
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
