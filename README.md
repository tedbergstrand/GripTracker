# GripTracker
ESP32-based Climbing Hangboard Force Gauge with data collection to CSV and a webserver for management and data viewing. Allows CSV download from the web UI, as well as charting and rudimentary data analysis. Also features a hangboard timer with pre-populated exercises.

## This is in Alpha and may stay that way forever, depending on how excited I get on improving it. I'd love for your to contribute. I hacked this together over Thanksgiving and I'm not a pro coder, so it could certainly use some love from someone smarter than me.


# Hardware:
- S Type Load Sensor - (ESP32-S3-DevKitC-1-N8R2)
- HX711 - (https://amzn.to/40VuAPo)
- ESP32-S3-DevKitC-1-N8R2 - (https://amzn.to/3uAc5Eh)


# To build the sensor:

I took the load sensor out of this Fuzion Hanging Crane Scale: https://amzn.to/3RfIloY. You can desolder or cut the 4 wires coming from the sensor. Any load sensor should work, but you'll likely need to tweak the calibration, no matter what.

## Solder the load sensor to the HX711. 
- The Red wire goes to E+
- Black to E-
- White to A-
- Green to A+.

## Solder 4 wires from the HX711 to the ESP32. 
- GND will go to a ground pin on the ESP.
- VCC (Power) to to a 3.3v Out pin.
- DT (Data) to an open GPIO port (I used GPIO4)
- SCK (Serial Clock) to another open GPIO port (I used GPIO6)

## Load code onto the ESP32.
- Modify the WiFi credentials and your GPIO pins after the includes in the code.

## Access the server from a web browser
- The ESP will print the IP address to the serial console when it connects. This is the URL you type into the browser.

# Using GripTracker
- GripTracker automatically begins logging data to a time-stamped CSV file. It will create a new file each time the device turns on.
- New sessions can be started by entering a custom name on the main page.
- Clicking the CSV file will let you download it.
- Data Analysis gives a zoomable (on desktop) Google chart to visualize the hangs, as well as Duration, Peak Force, Average Force and, Standard Deviation for each hang in the session.
- Hangs are automatically separated in the code and adds a "0,NaN" line to the CSV to indicate a new hang.
- Raw data can be viewed without downloading the CSV.
- You are able to delete CSV files through the Web UI. (Note: this is irreversible).

# Hangboard Timer
- Pre-populated with Repeaters, A version of Eva Lopez' MaxHangz, and Emil Abrahamsson's No Hangs protocols.
- Select the exercise from the drop-down and click the Start Button.
- Timer will begin and beep at the end of each Hang and Rest period.
- The idea is that you'll be able to see differences in your force production over the course of the exercise.

# Notes
  - I'm currently filtering the readings because I was experiencing very occasional signals that didn't belong. This may prove to be a poor solution, but it's the best I could come up with for now.
 
  # To-Do
  - Add 10 second delay to Hangboard Timer start to allow you to get into position.
