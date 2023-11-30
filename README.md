# GripTracker
ESP32-based Climbing Hangboard Force Gauge with data collection to CSV and a webserver for management and data viewing. Allows CSV download from the web UI, as well as data display ("Data") and rudimentary analysis ("View"). Also features a hangboard timer with pre-populated exercises.

__This is a prototype and may stay that way forever, depending on how excited I get on improving it. I'd love for your to contribute. I hacked this together over Thanksgiving and I'm not a pro coder, so it could certainly use some love from someone smarter than me. Right now, it's a huge mess of C, HTML, CSS, and JS all in one place and needs to be organized and split up.__



# Hardware:
- Load Sensor - (https://amzn.to/3RfIloY)
  - I took mine out of a $25 Crane Scale. 
- HX711 - (https://amzn.to/40VuAPo)
- ESP32-S3-DevKitC-1-N8R2 - (https://amzn.to/3uAc5Eh)


# To build the sensor:

I took the load sensor out of this Fuzion Hanging Crane Scale: https://amzn.to/3RfIloY. You can desolder or cut the 4 wires coming from the sensor. Any load sensor should work, but you'll likely need to tweak the calibration, no matter what.

## Solder the load sensor to the HX711. 
- The Red wire goes to E+
- Black to E-
- White to A-
- Green to A+

## Solder 4 wires from the HX711 to the ESP32. 
- GND will go to a ground pin on the ESP
- VCC (Power) to to a 3.3v Out pin
- DT (Data) to an open GPIO port (I used GPIO4)
- SCK (Serial Clock) to another open GPIO port (I used GPIO6)

## Load code onto the ESP32.
- Modify the WiFi credentials and your GPIO pins after the \#includes in the code.

## Access the server from a web browser
- The ESP will print the IP address to the serial console when it connects. This is the URL you type into the browser.

---

# Using GripTracker
- GripTracker automatically begins logging data to a new CSV file each boot. It will create a new file each time the device turns on.
- New sessions can be started by entering a custom name on the main page.
- GripTracker will create a WiFi Access Point called GripTracker-**:**, which you need to connect to in order to use the web UI.
- GripTracker only needs power, not a computer connection.
  - Besides identifying the server's IP address, which has always been 192.168.4.1 in my testing.
- GripTracker displays live force data on the web UI.
- Tare scale button will zero out the sensor after a grip has been hung on it.
- Clicking the CSV file will let you download it.
- "View" gives Duration, Peak Force, Average Force and Standard Deviation for each hang in the session.
- Hangs are automatically separated in the code and adds a "0,NaN" line to the CSV to indicate a new hang.
- "Data" launches a page to view raw data, separated into Hangs.
- You are able to delete CSV files through the Web UI. (Note: this is irreversible).

# Hangboard Timer
- Pre-populated with Repeaters, a version of Eva Lopez' MaxHangz, and Emil Abrahamsson's No Hangs protocols.
- Select the exercise from the drop-down and click the Start Button.
- Timer will beep at each transition between Get Ready, Hang, and Rest.
- The idea is that you'll be able to see differences in your force production over the course of the exercise and compare the same exercise over time.

# Notes

 
  # To-Do
  - Put CSS and JS into their own files to clean up the ESP code.
    - Arduino IDE 2 makes SPIFFS uploading more difficult. I'll split them up, but I will also leave the one-shot code for people who don't want to install 1.8.19 to handle SPIFFS uploading.  
  - Add instructions for using the Sparkfun sketches for calibration of HX711/Load Sensor to README.
  - Add photos to README
