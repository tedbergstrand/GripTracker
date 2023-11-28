# GripTracker
ESP32-based Climbing Hangboard Force Gauge with data collection to CSV and a webserver for management and data viewing. Allows CSV download from the web UI, as well as charting and rudimentary data analysis. Also features a hangboard timer with pre-populated exercises.

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
- GripTracker automatically begins logging data to a time-stamped CSV file. It will create a new file each time the device turns on.
- New sessions can be started by entering a custom name on the main page.
- - GripTracker only needs power, not a computer connection
  - Besides identifying the server's IP address.
- Tare scale button will zero out the sensor after a grip has been hung on it.
- Clicking the CSV file will let you download it.
- Data Analysis gives a zoomable (on desktop) Google chart to visualize the hangs, as well as Duration, Peak Force, Average Force and Standard Deviation for each hang in the session.
  - To Zoom, left click and select the area you want to zoom in on.
  - Right Click to return to full view.
- Hangs are automatically separated in the code and adds a "0,NaN" line to the CSV to indicate a new hang.
- Raw data can be viewed without downloading the CSV.
- You are able to delete CSV files through the Web UI. (Note: this is irreversible).

# Hangboard Timer
- Pre-populated with Repeaters, a version of Eva Lopez' MaxHangz, and Emil Abrahamsson's No Hangs protocols.
- Select the exercise from the drop-down and click the Start Button.
- Timer will beep at each transition between Get Ready, Hang, and Rest.
- The idea is that you'll be able to see differences in your force production over the course of the exercise.

# Notes
  - I'm currently filtering the readings because I was experiencing very occasional signals that didn't belong. This may prove to be a poor solution, but it's the best I could come up with for now.
  - I was having trouble getting the webserver to read external CSS and JS files, so everything is baked into the ESP code, right now. It's ugly, but it works.
 
  # To-Do
  - Put CSS and JS into their own files to clean up the ESP code.
  - Add instructions for using the Sparkfun sketches for calibration of HX711/Load Sensor to README.
  - Add photos to README
