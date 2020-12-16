# Arcade-Spinner v1.00
This is an Arduino Micro (ATMega32U4) (Leonardo and Pro Micro also work) arcade spinner with 2 axis digital joystick and 10 buttons for use with MAME or any other emulator which can use the X axis of the mouse as a paddle/spinner controller. This code should also work on any board which uses the ATmega32U4 as long as the port pins are mapped to the same "digital pins" as the Micro. I created this spinner because I wanted a cheaper alternative to the commercially available ones. I find it works well for ball and paddle games, Tempest, and also makes a decent controller for driving games.

To construct this you will need a 2-phase rotary encoder which can operate at 5v and some momentary switch buttons. The rotary encoder I used is: https://www.amazon.com/Signswise-Incremental-Encoder-Dc5-24v-Voltage/dp/B00UTIFCVA 
However, any 2-phase 5v one should work. You can even use the little 20-30 position encoders that come with a lot of Arduino kits, although you'll have to alter the code and I don't recommend using those. You will also need the Arduino joystick library available at: https://github.com/MHeironimus/ArduinoJoystickLibrary

This device will be detected as both a mouse and a joystick/gamepad. The joystick only has X and Y axes. The spinner controls the mouse X axis which is by default mapped to the analog dial in MAME (don't forget to enable the mouse in MAME under advanced options!). The buttons will work as regular gamepad/joystick buttons. The 2400 different positions (transitions) that can be detected on the 600ppr(pulse per revolution) encoder I'm using are way too many for our purposes so they are halved in the code to 1200. The code uses the Atmega32u4 ports directly because that's faster than using digitalRead/Write. I'm not doing any debouncing of the buttons or encoder as it seems to work great for me as is, but you might want to add debouncing depending on your hardware.

There is now a joystick/keyboard switch, a dedicated 'Quit' button, and a second function 'Shift' button. Each button/joystick direction can be mapped to do a second keyboard press, which happens regardless of the joystick/keyboard switch. (So, if in joystick mode, second function and joystick up is a keyboard press).

Currently the second functions are mapped to retroarch defaults and support: Quit, save state, load state, increase and decrease save state number, pause, menu and take screenshot. Joystick Player 1 up and down increase and decrease volume.

'Shift' and 'Quit' together press [ALT] + [F4] to enable the windows keyboard shortcut to force closing of a program.

The enclosure that I used can be bought at: https://www.galco.com/buy/Hammond-Manufacturing/RL6015BK
The knob for the encoder that I used can be bought at: https://www.amazon.com/gp/product/B01D2IIC3S


You should now be able to use the info found at http://wiki.arcadecontrols.com/index.php/Spinner_Turn_Count to adjust the analog sensitivity in MAME to more accurately simulate the original hardware with this spinner. 


## NOTE ##

Make sure to turn off mouse acceleration for the spinner to work properly.



## Updates ##

### 2020-11-14 ### (MF)

* Added support for 4 players (4x Joysticks)
* Added Keyboard support (All 4 players use the same keyboard, or 4 different joysticks, with a similar keyboard layout to MAME)
* Added Keyboard / Joystick toggle switch. In joystick mode, each player has their own joystick. In keyboard mode, each player uses the one keyboard.
* Added a dedicated [QUIT] button.
* Code for reading 74HC165's supports only player 1 and 2 at the moment, and is handled in software only.
* Player 3 and 4 each only use 4 inputs in MAME: 4 directions, button 1-3 and Start
* Added second function button (called SHIFT on the PCBs). Each button/direction may now press a key as a secondary function, if [SHIFT] is held down.
* Added second function to [QUIT] button (Normal quit is [ESC] but [SHIFT]+[QUIT] is [ALT]+[F4])
* Added second function to player 1 buttons. Full layout in the source. (It's mostly for a Retroarch layout atm)
* Added MAME config for players 1-4. (Layout is for main buttons/keys only. Next, is the second/SHIFT functions. I either need to map MAME to Retroarch layout, or vice versa. Undecided as of yet.)
* Modified keyboard HID to allow 30 keypresses at once. Very preliminary at the moment, since I'm not 100% sure it's correct or even works. I don't have enough buttons to test yet.



### 2020-11-01 ### (MF)

* ALL PCB layouts are untested.

* Added PCB for 2 players with 2x 74HC165's (SMD).
* Added PCB for 1 player with 15 pin Neo Geo input (THT).
* Added PCB for 1 player with 9 pin Atari/Amiga input (THT). (9Pin input supports 2 buttons (Amiga layout))



### 2020-10-01 ### (MF)

* Switched over development from the Arduino IDE to PlatformIO using Visual Studio Code.
* Program/sketch is now main.cpp instead of Arcade-Spiner.ino
* Rewrote much of the code to better understand it.
* Expanded code to support 1-6 Serial shift registers (74HC165, CD4041 etc) and 1-6 players, in theory.



### 2020-08-01 ### (MF+JD)

* Added 2 axis joystick.
* Removed Arduino Pro Micro comments. (All Arduino Pro Micro's I've purchased have the same pinout as the standard Arduino Micro)
* Added example Mame controller config file with button layout.



### 2019-07-18 ###

* No more mouse movement modes, you shouldn't need them. Also added a Y axis to the controller as it was reported it was not being detected on RetroPie without it. 



### 2019-07-03 ###

* Craig B contributed some code for optimizing button polling and for different mouse movement modes (ACCM/DROP). There is now also a maxBut #define to choose how many buttons you want, up to 10, and #if logic to set the initial state of those buttons. Adding more buttons if you need them or adapting the code to your specific board should be trivial.
