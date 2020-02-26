# Restaurant_FinderV1
The first version of the Restaurant Finder involves the use of a joystick to move the cursor around the map of Edmonton displayed on the TFT Display and view nearby restaurants. You can also view which restaurants are near you in Mode 1 and find out exactly where they are on the map.

Included files are:
--------------------------------------
- a1part1.cpp
- README
- Makefile

Required Components:
-----------------------
- 1 x LCD Screen
- 1 x Arduino MEGA 2560 Board
- 1 x Arduino USB cable
- 5 x Jumper Wires
- 1 x Joystick

Wiring Instructions:
----------------------
- Arduino Analog Pin A8 <--> Joystick VRy (bend wire)
- Arduino Analog Pin A9 <--> Joystick VRx (bend wire)
- Arduino Digital Pin 53 <--> Joystick SW
- Arduino GND <--> Joystick GND
- Arduino 5V <--> Joystick 5V
- Arduino <--> LCD Screen


Running Instructions:
---------------------
- Use the "arduino-port-select" to ensure that the connected Arduino is port 1 for both.
- Open the directory containing joy_cursor.cpp file in the terminal window. Use the "make upload a1part1.cpp" to upload the code to the Arduino. Once the map fully loads on the LCD screen, you are now able to use the joystick to control and move the red cursor around the map.
- This is mode 0, if you go over any parts of the map, a new patch will be loaded and you will be able to move around there too.
- If you tap on the screen, you will be able to see which restaraunts are currently on the same patch as the one you are on and they will be
displayed as black dots.
- To enter Mode1, press the joystick and a screen will load with 21 of the restaraunts near you, you can use the joystick to scroll down and up the list of restaraunts, once you select the restaraunt click the joystick and the map will reload with the cursor near the selected restauraunt.

