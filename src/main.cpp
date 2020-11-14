/*   Arcade Spinner v1.0
*    Copyright 2018 Joe W (jmtw000 a/t gmail.com)
*                   Craig B - Updated code for mouse movement modes(DROP, ACCM) and case statement for Button port bit validation
*    Copyright 2020 Mike F (mikerochip a/t hotmail.com)
*							badgered Joe W into adding Joystick Code, and rewrote (almost) from scratch, to better understand how it all works, 
*							adding a keyboard mode, a keyboard/joystick switch, and a second function [SHIFT] button)
*                          	Code may be expanded with a third 74HC165 for player 3, and a fourth one for player 4, 
*							BUT no 74HC165 code is written for this yet. Keyboard layout is set though. 
*							P3/4 only use 4 butons, so 1 mcu each (8 inputs) is enough. U, D, L, R, B1-3, Start (Coin 3/4 can be SHIFT+Start)

*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

# include <Arduino.h>
# include "Keyboard30.h"	// For secondary <shift> button, and keyboard presses
# include "Mouse.h"	  		// For Spinner (Mouse X axis)
# include "Joystick.h"		// For Joystick control

#define maxBut 9		 	// The number of buttons you are using up to 9, for 2 player mode. (If just using 1 joystick, and no shift registers, can set to 10)
#define JOYSTICK_COUNT 2 	// The number of joysticks connected to the Arduino

#define pinA 2            	// The pins that the rotary encoder's A and B terminals are connected to.
#define pinB 3            	//

volatile int previousReading = 0;   //The previous state of the AB pins
volatile int rotPosition = 0;       //Keeps track of how much the encoder has been moved
volatile int rotMulti = 0;

#define NUMBER_OF_SHIFT_CHIPS JOYSTICK_COUNT

#if JOYSTICK_COUNT == 1			// Disable the 74CH165 etc in 1 player mode, since they're unneeded
#define NUMBER_OF_SHIFT_CHIPS 0
#endif

#define DATA_WIDTH NUMBER_OF_SHIFT_CHIPS * 8

#define TotalPresses maxBut+4 // Number of total keybard presses per person. All buttons, plus 4 directions

/* Sets the initial "last joypad state" count for each joystick/player. 4, for each of the 4 directions */
int lastJoyState[JOYSTICK_COUNT][4];

/* Set the initial "last state count" of the buttons for each joystick/player */
int lastButtonState[JOYSTICK_COUNT][maxBut];

//Create Joystick objects
#if JOYSTICK_COUNT == 1
Joystick_ Joystick[JOYSTICK_COUNT] =
{
  Joystick_(0x03, JOYSTICK_TYPE_GAMEPAD, maxBut, 0, true, true, false, false, false, false, false, false, false, false, false)
};
#endif

#if JOYSTICK_COUNT == 2
Joystick_ Joystick[JOYSTICK_COUNT] =
{
  Joystick_(0x03, JOYSTICK_TYPE_GAMEPAD, maxBut, 0, true, true, false, false, false, false, false, false, false, false, false),
  Joystick_(0x04, JOYSTICK_TYPE_GAMEPAD, maxBut, 0, true, true, false, false, false, false, false, false, false, false, false),
};
#endif

#if JOYSTICK_COUNT == 3
Joystick_ Joystick[JOYSTICK_COUNT] =
{
  Joystick_(0x03, JOYSTICK_TYPE_GAMEPAD, maxBut, 0, true, true, false, false, false, false, false, false, false, false, false),
  Joystick_(0x04, JOYSTICK_TYPE_GAMEPAD, maxBut, 0, true, true, false, false, false, false, false, false, false, false, false),
  Joystick_(0x05, JOYSTICK_TYPE_GAMEPAD, maxBut, 0, true, true, false, false, false, false, false, false, false, false, false),
};
#endif

#if JOYSTICK_COUNT == 4
Joystick_ Joystick[JOYSTICK_COUNT] =
{
  Joystick_(0x03, JOYSTICK_TYPE_GAMEPAD, maxBut, 0, true, true, false, false, false, false, false, false, false, false, false),
  Joystick_(0x04, JOYSTICK_TYPE_GAMEPAD, maxBut, 0, true, true, false, false, false, false, false, false, false, false, false),
  Joystick_(0x05, JOYSTICK_TYPE_GAMEPAD, maxBut, 0, true, true, false, false, false, false, false, false, false, false, false),
  Joystick_(0x06, JOYSTICK_TYPE_GAMEPAD, maxBut, 0, true, true, false, false, false, false, false, false, false, false, false),
};
#endif


// Use ID 03 for first joystick, 04 for second joystick
// Joystick type: gamepad / digital joystick
// maxBut: Button Count (Both sticks will be the same)
// 0:  Hat Switch Count
// X Axis. We need at least two axes. 
// Y Axis. Second axis used.
// No Z Axis  
// No Rx
// No Ry
// No Rz
// No rudder
// No throttle      
// No accelerator
// No brake
// No steering

//int dataInPin = 14; // MISO // Change to MISO to use a Leonardo, which has no SS pin.
int dataInPin = 17; // SS
int loadPin = 22;   // A4
int clockPin = 23;  // A5
int clockEnablePin = 16; // MOSI

int clockdelay = 5; // Set clock delay in microseconds (Millionths of seconds!)

#if NUMBER_OF_SHIFT_CHIPS < 2
unsigned long ShiftReg = 0b11111111;
#endif

#if NUMBER_OF_SHIFT_CHIPS == 2
unsigned long ShiftReg = 0b1111111111111111;
#endif

#if NUMBER_OF_SHIFT_CHIPS == 3
unsigned long ShiftReg = 0b111111111111111111111111;
#endif

#if NUMBER_OF_SHIFT_CHIPS == 4
unsigned long ShiftReg = 0b11111111111111111111111111111111;
#endif

#if NUMBER_OF_SHIFT_CHIPS == 5
unsigned long ShiftReg = 0b1111111111111111111111111111111111111111;
#endif

#if NUMBER_OF_SHIFT_CHIPS == 6
unsigned long ShiftReg = 0b111111111111111111111111111111111111111111111111;
#endif


int dataWidth = NUMBER_OF_SHIFT_CHIPS * 8;

// Key press array, for each user. Up, Right, Down, Left, B1-B9 (B10 is really only for Player 1, to use as a right flipper/coin2)
// Arduino treats values below 128 as printable and uses an ascii lookup table. Add 136 to the value to overcome this.
// https://forum.arduino.cc/index.php?topic=179548.0
// F16 used as a placeholder key.

unsigned char keylist[8][14] =
{
	{ 0xE8, 0xE6, 0xE2, 0xE4, KEY_LEFT_CTRL, KEY_LEFT_ALT, 0x20, KEY_LEFT_SHIFT, 'z', 'x', 'c', '3', '1', '4'},										// Player 1 keyboard keys
	{ 'r', 'g', 'f', 'd', 'a', 's', 'q', 'w', 'e', '[', ']', '4', '2', KEY_F16},																	// Player 2 keyboard keys
	{ 'i', 'l', 'k', 'j', KEY_RIGHT_CTRL, KEY_RIGHT_SHIFT, KEY_RETURN, KEY_F16, KEY_F16, KEY_F16, KEY_F16, KEY_F16, '5', KEY_F16},					// Player 3 keyboard keys
	{ KEY_UP_ARROW, KEY_RIGHT_ARROW, KEY_DOWN_ARROW, KEY_LEFT_ARROW,0xEA,0xEB,0xE0, KEY_F16, KEY_F16, KEY_F16, KEY_F16, KEY_F16, '6', KEY_F16 },	// Player 4 keyboard keys
	{ 0xDF, 0xE6, 0xDE, 0xE4, KEY_F2, KEY_F7, KEY_F4, KEY_F8, KEY_F6, 'p', KEY_F1, '3', '1', '4'},	            									// Player 1 [SHIFT] keys
	{ 'r', 'g', 'f', 'd', 'a', 's', 'q', 'w', 'e', '[', ']', '4', '2', KEY_F16},                                     								// Player 2 [SHIFT] keys
	{ 'i', 'l', 'k', 'j', KEY_RIGHT_CTRL, KEY_RIGHT_SHIFT, KEY_RETURN, KEY_F16, KEY_F16, KEY_F16, KEY_F16, KEY_F16, '7', KEY_F16},					// Player 3 [SHIFT] keys
	{ KEY_UP_ARROW, KEY_RIGHT_ARROW, KEY_DOWN_ARROW, KEY_LEFT_ARROW,0xEA,0xEB,0xE0, KEY_F16, KEY_F16, KEY_F16, KEY_F16, KEY_F16, '8', KEY_F16 },	// Player 4 [SHIFT] keys
};

/* Key Map */
// Player 1
// UP		NUMPAD_8
// Down		NUMPAD_2
// Left		NUMPAD_4
// Right	NUMPAD_6
// B1		CTRL
// B2		ALT
// B3		SPACE
// B4		LEFT_SHIFT
// B5		Z
// B6		X
/* B7		C				Used for Select key in RetroArch		Also, C, SHIFT, Z, X are used for the 4 button Neo-Geo layout in MAME */
// B8		3				Player 1 Coin (COIN1) / Left Flipper
// B9		1				Player 1 Start
// B10      4               Player 2 Coin (COIN2) / Right Flipper   <-- Not set / used by default. Set to Buttons to 10 to use.

// Player 2
// Up		R
// Down		F
// Left		D
// Right	G
// B1		A
// B2		S
// B3		Q
// B4		W
// B5		E
// B6		[
// B7		]				Used for Select key in RetroArch / Button 7 in MAME
// B8		4				Player 2 Coin (COIN2) / Right Flipper
// B9		2				Player 2 Start
// B10      F16									                   <-- Not set / used by default.

// Player 3
// Up		I
// Down		K
// Left		J
// Right	L
// B1		Right CTRL
// B2		Right SHIFT
// B3		Enter/Return (Not Numeric)
// B4		F16									                   <-- Not set / used by default.
// B5		F16									                   <-- Not set / used by default.
// B6		F16									                   <-- Not set / used by default.
// B7		F16									                   <-- Not set / used by default.			
// B8		F16									                   <-- Not set / used by default.		
// B9		5				Player 3 Start
// B10      F16									                   <-- Not set / used by default.

// Player 4
// Up		Up Arrow
// Down		Down Arrow
// Left		Left Arrow
// Right	Right Arrow
// B1		0 (Numeric Keypad)
// B2		. (Numeric Keypad)
// B3		Enter (Numeric Keypad)
// B4		F16									                   <-- Not set / used by default.
// B5		F16									                   <-- Not set / used by default.
// B6		F16									                   <-- Not set / used by default.
// B7		F16									                   <-- Not set / used by default.			
// B8		F16									                   <-- Not set / used by default.		
// B9		6				Player 4 Start
// B10      F16									                   <-- Not set / used by default.

// Player 1 [SHIFT]
// UP		NUMPAD_+		Used for Volume Up in RetroArch
// Down		NUMPAD_2
// Left		NUMPAD_-		Used for Volume Down in RetroArch
// Right	NUMPAD_6
// B1		F2				Used for Save Save_State in RetroArch
// B2		F7				Used for [Increase Save_State number]
// B3		F4				Used for Load Save_State in RetroArch
// B4		F8              Used for Take Screenshot in RetroArch
// B5		F6				Used for [Decrease Save_State number]
// B6		P				Used for Pause in RetroArch
// B7		F1				Used for MENU key in RetroArch
// B8		3				Player 1 Coin (COIN1) / Left Flipper
// B9		1				Player 1 Start
// B10		4				Player 2 Coin (COIN2) / Right Fipper

// Player 2 [SHIFT]
// Up		R
// Down		F
// Left		D
// Right	G
// B1		A
// B2		S
// B3		Q
// B4		W
// B5		E
// B6		[
// B7		]				Used for Select key in RetroArch / Button 7 in MAME
// B8		4				Player 2 Coin (COIN2) / Right Flipper
// B9		2				Player 2 Start
// B10      F16									                   <-- Not set / used by default.

// Player 3 [SHIFT]
// Up		I
// Down		K
// Left		J
// Right	L
// B1		Right CTRL
// B2		Right SHIFT
// B3		Enter/Return (Not Numeric)
// B4		F16									                   <-- Not set / used by default.
// B5		F16									                   <-- Not set / used by default.
// B6		F16									                   <-- Not set / used by default.
// B7		F16									                   <-- Not set / used by default.			
// B8		F16									                   <-- Not set / used by default.		
// B9		7				Player 3 Coin (COIN3)
// B10      F16									                   <-- Not set / used by default.

// Player 4 [SHIFT]
// Up		Up Arrow
// Down		Down Arrow
// Left		Left Arrow
// Right	Right Arrow
// B1		0 (Numeric Keypad)
// B2		. (Numeric Keypad)
// B3		Enter (Numeric Keypad)
// B4		F16									                   <-- Not set / used by default.
// B5		F16									                   <-- Not set / used by default.
// B6		F16									                   <-- Not set / used by default.
// B7		F16									                   <-- Not set / used by default.			
// B8		F16									                   <-- Not set / used by default.		
// B9		8				Player 4 Coin (COIN4)
// B10      F16									                   <-- Not set / used by default.

void pinChange()
{
	//Set the currentReading variable to the current state of encoder terminals A and B which are conveniently located in bits 0 and 1 (digital pins 2 and 3) of PIND
	//This will give us a nice binary number, eg. 0b00000011, representing the current state of the two terminals.
	//You could do int currentReading = (digitalRead(pinA) << 1) | digitalRead(pinB); to get the same thing, but it would be much slower.
	int currentReading = PIND & 0b00000011;

	//Take the nice binary number we got last time there was an interrupt and shift it to the left by 2 then OR it with the current reading.
	//This will give us a nice binary number, eg. 0b00001100, representing the former and current state of the two encoder terminals.
	int combinedReading = (previousReading << 2) | currentReading;

	//Now that we know the previous and current state of the two terminals we can determine which direction the rotary encoder is turning.

	//Going to the right
	if (combinedReading == 0b0010 ||
		combinedReading == 0b1011 ||
		combinedReading == 0b1101 ||
		combinedReading == 0b0100)
	{
		rotPosition++;                   //update the position of the encoder
	}

	//Going to the left
	if (combinedReading == 0b0001 ||
		combinedReading == 0b0111 ||
		combinedReading == 0b1110 ||
		combinedReading == 0b1000)
	{
		rotPosition--;                   //update the position of the encoder    
	}
	//Save the previous state of the A and B terminals for next time
	previousReading = currentReading;
}

void setup()
{
	// put your setup code here, to run once:
	// No need to set the pin modes with DDRx = DDRx | 0b00000000 as we're using all input and that's the initial state of the pins
	// Use internal input resistors for all the pins we're using
	PORTD = 0b11010011; // Digital pins D2, D3, D4, D6, D12
	PORTB = 0b11111111; // Digital pins D8, D9, D10, D11, MISO, MOSI, SCLK, SS
	PORTC = 0b11000000; // Digital pin D5, D13
	PORTE = 0b01000000; // Digital pin D7
	PORTF = 0b11110011; // Digital pin A0, A1, A2, A3, A4, A5

	//Layout (Arduino Pins):
	// D2, D3: Arcade Spinner A and B
	// D4, D5, D6, D7, D8, D9, D10, D11, D12, D13 : P1 Buttons 1-9, QUIT
	// A0, A1, A2, A3: P1 Joystick Up, Right, Down, Left
	// A4: Shift (Modifier)
	// A5: Keyboard

/*  Not needed, with PORTX=0bxxxxxxxx;
	pinMode(PD4, INPUT_PULLUP); // D4
	pinMode(PC6, INPUT_PULLUP); // D5
	pinMode(PD7, INPUT_PULLUP); // D6
	pinMode(PE6, INPUT_PULLUP); // D7
	pinMode(PB4, INPUT_PULLUP); // D8
	pinMode(PB5, INPUT_PULLUP); // D9
	pinMode(PB6, INPUT_PULLUP); // D10
	pinMode(PB7, INPUT_PULLUP); // D11
	pinMode(PD6, INPUT_PULLUP); // D12
	pinMode(PC7, INPUT_PULLUP); // D13
	pinMode(PF7, INPUT_PULLUP); // D18 / A0
	pinMode(PF6, INPUT_PULLUP); // D19 / A1
	pinMode(PF5, INPUT_PULLUP); // D20 / A2
	pinMode(PF4, INPUT_PULLUP); // D21 / A3
*/
	#if (NUMBER_OF_SHIFT_CHIPS < 1)
	{
		// Use A4 and A5 for input (SHIFT mode and KEYB mode) when not using serial shift register chips
		pinMode(PF1, INPUT_PULLUP); // D22 / A4
		pinMode(PF0, INPUT_PULLUP); // D23 / A5
	}
	#else
	{
		// 74HC165 pins
		pinMode(loadPin, OUTPUT);           // A4
		pinMode(clockPin, OUTPUT);          // A5
		pinMode(clockEnablePin, OUTPUT);    // MOSI : No Clock Enable Pin on PCB. Always Low. Is this a Problem?!?!? If so, modify PCB!!
		pinMode(dataInPin, INPUT);          // SS
	}
	#endif

	//Start the joysticks
	for (int index = 0; index < JOYSTICK_COUNT; index++)
	{
		Joystick[index].begin();

		// Set Range for digital joystick / joypad ie on/off.
		Joystick[index].setXAxisRange(-1, 1);
		Joystick[index].setYAxisRange(-1, 1);

		//Center the X and Y axes on the joystick
		Joystick[index].setXAxis(0);
		Joystick[index].setYAxis(0);

		// Sets the initial last button state (unpressed) for the 4 directions
		lastJoyState[JOYSTICK_COUNT][0] = 1;
		lastJoyState[JOYSTICK_COUNT][1] = 1;
		lastJoyState[JOYSTICK_COUNT][2] = 1;
		lastJoyState[JOYSTICK_COUNT][3] = 1;

		// Sets the initial last button state (unpressed) for all the buttons for this joystick/player
		for (int s = 0; s < maxBut; s = s + 1)
		{
			lastButtonState[JOYSTICK_COUNT][s] = 1;
		}
	}

	//Set up the interrupt handler for the encoder's A and B terminals on digital pins 2 and 3 respectively. Both interrupts use the same handler.
	attachInterrupt(digitalPinToInterrupt(pinA), pinChange, CHANGE);
	attachInterrupt(digitalPinToInterrupt(pinB), pinChange, CHANGE);

	//Start the keyboard
	Keyboard.begin();
	Keyboard.releaseAll();  // Not required, but just in case.
	Mouse.begin();			/* Start the mouse. Required for spinner. (Also, disable mouse acceleration in windows) */
}

void loop()
{
	int currentButtonState;
	int currentJoyState;

	//If the encoder has moved 1 or more transitions move the mouse in the appropriate direction 
	//and update the rotPosition variable to reflect that we have moved the mouse. The mouse will move 1/2 
	//the number of pixels of the value currently in the rotPosition variable. We are using 1/2 (rotPosition>>1) because the total number 
	//of transitions(positions) on our encoder is 2400 which is way too high. 1200 positions is more than enough.

	if (rotPosition >= 1 || rotPosition <= -1)
	{
		rotMulti = rotPosition >> 1;                 //copy rotPosition/2 to a temporary variable in case there's an interrupt while we're moving the mouse 
		Mouse.move(rotMulti, 0, 0);
		rotPosition -= (rotMulti << 1);              //adjust rotPosition to account for mouse movement
	}

	// Special Button: Quit (Quits game with ESC key. If shift pressed, quits with ALT-F4)
	int currentQuitKeyState;

	// Special Button: Shift (Allows all other buttons to have second function)
	int currentSHIFTState;

	// Special button/toggle: KEYB (Allows Arduino to operate in Joystick mode, or keyboard mode)
	int currentKeyboardModeState;

	currentQuitKeyState = (PINC & 0b10000000) >> 7;      // Read Keyboard quit button state - Digital pin D13

	#if (NUMBER_OF_SHIFT_CHIPS < 1) // If not using parallel shift chips, use SHIFT and Keyboard mode from A4 and A5
	{
		currentSHIFTState = (PINF & 0b00000010) >> 1;        // A4: Read SHIFT button state (This button will allow a secondary action on every existing button/joystick direction)
		currentKeyboardModeState = (PINF & 0b00000001) >> 0; // A5: Read Keyboard toggle button state (Puts controller in Keyboard or Joystick mode)
	}
	#else
	{
        // Read the entire shifter output, for each chip
		digitalWrite(loadPin, LOW);     // Write pulse to load pin
		delayMicroseconds(clockdelay);  // Wait clock amount

		digitalWrite(loadPin, HIGH);    // 
		delayMicroseconds(clockdelay);

		// Get data from 74HC165
		digitalWrite(clockPin, HIGH);
		digitalWrite(clockEnablePin, LOW);

		for (int c = 0; c < dataWidth; c = c + 1)
		{
			//int v = digitalRead(dataInPin); // Read Pin MISO with digitalRead (Only useful on Leonardo, which has no SS pin)
			//int v = (PINB & 0b0000100) >> 2; // Read MISO Pin directly (Only useful on Leonardo, which has no SS pin)

			//int v = digitalRead(dataInPin); // Read Pin D17 (SS) with digitalRead
			int v = (PINB & 0b00000001) >> 0; // Read SS Pin directly (Faster than using digitalRead)
			ShiftReg |= (v << ((dataWidth - 1) - c));   // Get single bit, and place into Binary register of all 16 input bits.
			digitalWrite(clockPin, HIGH);
			delayMicroseconds(clockdelay);
			digitalWrite(clockPin, LOW);
		}
		digitalWrite(clockEnablePin, HIGH);

		//74HC165(1) Input (G) - Shift
		currentSHIFTState = (ShiftReg & 0b0000000001000000) >> 6;           // Read SHIFT button state (This button will allow a secondary action on every existing button/joystick direction)

		//74HC165(1) Input (H) - Keyboard Toggle
		currentKeyboardModeState = (ShiftReg & 0b0000000010000000) >> 7;    // Read Keyboard toggle button state (Puts controller in Keyboard or 2x Joystick mode)
	}
	#endif

	switch (currentSHIFTState)  //Handle Special Keys
	{
	case 0: // SHIFT button pressed - Quit is [ALT]+[F4]
		if (currentQuitKeyState == 1)
		{
			// Button Not Pressed
			Keyboard.release(KEY_LEFT_ALT);
			Keyboard.release(KEY_F4);
		}
		else
		{
			// Button Pressed: Shifted Quit key
			Keyboard.press(KEY_LEFT_ALT);
			Keyboard.press(KEY_F4);
		}
		break;

	case 1: // SHIFT not pressed - Quit is [ESC]
		if (currentQuitKeyState == 1)
		{
			// Button Not Pressed
			Keyboard.release(KEY_ESC);
		}
		else
		{
			// Button Pressed: Normal Quit key
			Keyboard.press(KEY_ESC);
		}
		break;
	}

	//Iterate through the 4 axis button presses (0-4) assigning the current state of the pin for each button, HIGH(0b00000001) or LOW(0b00000000), to the currentState variable
	int jb[JOYSTICK_COUNT];
	int button[JOYSTICK_COUNT];

	//Check to see if arduino needs to be in Keyboard or Joystick Mode, depending on the value of the KEYB input on the 74HC165 if serial load registers are used, or A5 if not.
	switch (currentKeyboardModeState)       // Swap case values to change default behaviour. Default is KEYB button is off, keyboard mode is off, joystick mode is on.
	{
	case 0: // Key is pressed, so, enable KEYBOARD mode
		/* keyboard code here */
		for (int i = 0; i < TotalPresses; i = i + 1) // Player 1
		{
			int curPlayerOffset = 0;
			int curPlayerShiftOffset = 4; // Player one keyset is zero, shifted keyset is four. Player 2 is 1/5, Player 3 is 2/6 and Player 4 is 3/7
			int curKey;
			switch (i)  // Read the input value from the Arduino
			{
			case 0: //on digital pin A0, PF7 - Joystick Up
				curKey = (PINF & 0b10000000) >> 7; //logical AND the 8-bit pin reading with a mask to isolate the specific bit we're interested in and then shift it to the end of the byte
				break;
			case 1: //on digital pin A1, PF6 - Joystick Right
				curKey = (PINF & 0b01000000) >> 6;
				break;
			case 2: //on digital pin A2, PF5 - Joystick Down
				curKey = (PINF & 0b00100000) >> 5;
				break;
			case 3: //on digital pin A3, PF4 - Joystick Left
				curKey = (PINF & 0b00010000) >> 4;
				break;
			case 4: //on digital pin D4, PD4 - Arcade Button 1
				curKey = (PIND & 0b00010000) >> 4;
				break;
			case 5: //on digital pin D5, PC6 - Arcade Button 2
				curKey = (PINC & 0b01000000) >> 6;
				break;
			case 6: //on digital pin D6, PD7 - Arcade Button 3
				curKey = (PIND & 0b10000000) >> 7;
				break;
			case 7: //on digital pin D7, PE6 - Arcade Button 4
				curKey = (PINE & 0b01000000) >> 6;
				break;
			case 8: //on digital pin D8, PB4 - Arcade Button 5
				curKey = (PINB & 0b00010000) >> 4;
				break;
			case 9: //on digital pin D9, PB5 - Arcade Button 6
				curKey = (PINB & 0b00100000) >> 5;
				break;
			case 10: //on digital pin D10, PB6 - Arcade Button 7
				curKey = (PINB & 0b01000000) >> 6;
				break;
			case 11: //on digital pin D11, PB7 - Arcade Button 8
				curKey = (PINB & 0b10000000) >> 7;
				break;
			case 12: //on digital pin D12, PD6 - Arcade Button 9
				curKey = (PIND & 0b01000000) >> 6;
				break;
			case 13:  //on digital pin MISO, PB3 - Arcade Button 10
				curKey = (PINB & 0b00001000) >> 3; // MISO
				break;
			default: //should never happen
				curKey = 0b00000000;
				break;
			}

			switch (currentSHIFTState) // Actually Press the correct key.
			{
			case 0: // SHIFT IS pressed
				if ((curKey) == 1) // If the current input is not pressed, release both keys, else press the correct shifted key
				{
					// Button IS NOT pressed (SHIFT *is* pressed)
					Keyboard.release(keylist[curPlayerOffset][i]);      // Release Normal Key
					Keyboard.release(keylist[curPlayerShiftOffset][i]); // Release Shifted key
				}
				else
				{
					// Button IS pressed (SHIFT *is* pressed)
					Keyboard.release(keylist[curPlayerOffset][i]);      // Release Normal Key
					Keyboard.press(keylist[curPlayerShiftOffset][i]);   // Press Shifted Key
				}
				break;

			case 1: // SHIFT NOT pressed
				if (curKey == 1) // If the current input is not pressed, release both keys, else press the correct normal key
				{
					// Button IS NOT pressed (SHIFT is *not* pressed)
					Keyboard.release(keylist[curPlayerOffset][i]);      // Release Normal Key
					Keyboard.release(keylist[curPlayerShiftOffset][i]); // Release Shifted key
				}
				else
				{
					// Button IS pressed (SHIFT is *not* pressed)
					Keyboard.release(keylist[curPlayerShiftOffset][i]); // Release Shifted Key
					Keyboard.press(keylist[curPlayerOffset][i]);        // Press Normal Key
				}
				break;
			}
		}

		for (int i = 0; i < TotalPresses; i = i + 1) // Player 2
		{
			int curPlayerOffset = 1;
			int curPlayerShiftOffset = 3;
			int curKey;
			switch (i)  // Read the input value from the Arduino and 74HC165
			{
			case 0: //74HC165(1) Input (C) - Joystick Up
				curKey = (ShiftReg & 0b0000000000000100) >> 2; //logical AND the 16-bit pin reading with a mask to isolate the specific bit we're interested in and then shift it to the end of the byte
				break;
			case 1: //74HC165(1) Input (D) - Joystick Right
				curKey = (ShiftReg & 0b0000000000001000) >> 3;
				break;
			case 2: //74HC165(1) Input (E) - Joystick Down
				curKey = (ShiftReg & 0b0000000000010000) >> 4;
				break;
			case 3: //74HC165(1) Input (F) - Joystick Left
				curKey = (ShiftReg & 0b0000000000100000) >> 5;
				break;
			case 4: //74HC165(2) Input (A) - Arcade Button 1
				curKey = (ShiftReg & 0b0000000100000000) >> 8; //logical AND the 16-bit pin reading with a mask to isolate the specific bit we're interested in and then shift it to the end of the byte
				break;
			case 5: //74HC165(2) Input (B) - Arcade Button 2
				curKey = (ShiftReg & 0b0000001000000000) >> 9;
				break;
			case 6: //74HC165(2) Input (C) - Arcade Button 3
				curKey = (ShiftReg & 0b0000010000000000) >> 10;
				break;
			case 7: //74HC165(2) Input (D) - Arcade Button 4
				curKey = (ShiftReg & 0b0000100000000000) >> 11;
				break;
			case 8: //74HC165(2) Input (E)  - Arcade Button 5
				curKey = (ShiftReg & 0b0001000000000000) >> 12;
				break;
			case 9: //74HC165(2) Input (F) - Arcade Button 6
				curKey = (ShiftReg & 0b0010000000000000) >> 13;
				break;
			case 10: //74HC165(2) Input (G) - Arcade Button 7
				curKey = (ShiftReg & 0b0100000000000000) >> 14;
				break;
			case 11: //74HC165(2) Input (H) - Arcade Button 8
				curKey = (ShiftReg & 0b1000000000000000) >> 15;
				break;
			case 12: //74HC165(1) Input (A) - Arcade Button 9
				curKey = (ShiftReg & 0b0000000000000001) >> 0;
				break;
			case 13:  //74HC165(1) Input (B) - Arcade Button 10
				curKey = (ShiftReg & 0b0000000000000010) >> 1;
				break;
			default: //should never happen
				curKey = 0b0000000000000000;
				break;
			}

			switch (currentSHIFTState) // Actually Press the correct key.
			{
			case 0: // SHIFT IS pressed
				if ((curKey) == 1) // If the current input is not pressed, release both keys, else press the correct shifted key
				{
					// Button IS NOT pressed
					Keyboard.release(keylist[curPlayerOffset][i]);      // Release Normal Key
					Keyboard.release(keylist[curPlayerShiftOffset][i]); // Release Shifted key
				}
				else
				{
					// Button IS pressed
					Keyboard.release(keylist[curPlayerOffset][i]);      // Release Normal Key
					Keyboard.press(keylist[curPlayerShiftOffset][i]);   // Press Shifted Key
				}
				break;

			case 1: // SHIFT NOT pressed
				if (curKey == 1) // If the current input is not pressed, release both keys, else press the correct normal key
				{
					// Button IS NOT pressed
					Keyboard.release(keylist[curPlayerOffset][i]);      // Release Normal Key
					Keyboard.release(keylist[curPlayerShiftOffset][i]); // Release Shifted key
				}
				else
				{
					// Button IS pressed
					Keyboard.release(keylist[curPlayerShiftOffset][i]); // Release Shifted Key
					Keyboard.press(keylist[curPlayerOffset][i]);        // Press Normal Key
				}
				break;
			}
		}
		break;

	case 1: // Key is not pressed, so enable JOYSTICK mode
		/* Joystick code here */
		for (int i = 0; i < JOYSTICK_COUNT; i = i + 1)
		{
			jb[i] = 0;
			do
			{
				if (i == 0) // For first set of joystick direction inputs, read states from Arduino Pins
				{
					switch (jb[i])
					{
					case 0: //on digital pin A0, PF7 - Joystick Up
						currentJoyState = (PINF & 0b10000000) >> 7; //logical AND the 8-bit pin reading with a mask to isolate the specific bit we're interested in and then shift it to the end of the byte
						break;
					case 1: //on digital pin A1, PF6 - Joystick Right
						currentJoyState = (PINF & 0b01000000) >> 6;
						break;
					case 2: //on digital pin A2, PF5 - Joystick Down
						currentJoyState = (PINF & 0b00100000) >> 5;
						break;
					case 3: //on digital pin A3, PF4 - Joystick Left
						currentJoyState = (PINF & 0b00010000) >> 4;
						break;
					default: //should never happen
						currentJoyState = 0b00000000;
						break;
					}
				}
				else
				{
					/* code for 74HC165 etc here */
					switch (jb[i])
					{
					case 0: //74HC165(1) Input (C) - Joystick Up
						currentJoyState = (ShiftReg & 0b0000000000000100) >> 2; //logical AND the 16-bit pin reading with a mask to isolate the specific bit we're interested in and then shift it to the end of the byte
						break;
					case 1: //74HC165(1) Input (D) - Joystick Right
						currentJoyState = (ShiftReg & 0b0000000000001000) >> 3;
						break;
					case 2: //74HC165(1) Input (E) - Joystick Down
						currentJoyState = (ShiftReg & 0b0000000000010000) >> 4;
						break;
					case 3: //74HC165(1) Input (F) - Joystick Left
						currentJoyState = (ShiftReg & 0b0000000000100000) >> 5;
						break;
					default: //should never happen
						currentJoyState = 0b0000000000000000;
						break;
					}
				}

				if (currentJoyState != lastJoyState[i][jb[i]])
				{
					//add code here for what you want to happen when the axis state has changed
					int KeyTable;
					if (currentSHIFTState == 1)
					{
						KeyTable = i; // Unshifted Key list
					}
					else
					{
						KeyTable = i + 2; // Keys + SHIFT button modifier list
					}
					switch (currentKeyboardModeState)
					{
					case 0: // Keyboard Mode
						switch (jb[i])
						{
						case 0: // Keyboard NUMPAD_8 [UP]
							if (currentJoyState == 1)
							{
								Keyboard.release(keylist[KeyTable][jb[i]]);
							}
							else
							{
								Keyboard.press(keylist[KeyTable][jb[i]]);
							}
							break;
						case 2: // Keyboard NUMPAD_2 [Down]
							if (currentJoyState == 1)
							{
								Keyboard.release(keylist[KeyTable][jb[i]]);
							}
							else
							{
								Keyboard.press(keylist[KeyTable][jb[i]]);
							}
							break;
						case 3: // Keyboard NUMPAD_4 [LEFT]
							if (currentJoyState == 1)
							{
								Keyboard.release(keylist[KeyTable][jb[i]]);
							}
							else
							{
								Keyboard.press(keylist[KeyTable][jb[i]]);
							}
							break;
						case 1: // Keyboard NUMPAD_6 [RIGHT]
							if (currentJoyState == 1)
							{
								Keyboard.release(keylist[KeyTable][jb[i]]);
							}
							else
							{
								Keyboard.press(keylist[KeyTable][jb[i]]);
							}
							break;
						}
						break;

					case 1: // Joystick Mode
						switch (jb[i])
						{
						case 0: // Joystick Up
							Joystick[i].setYAxis(-!currentJoyState);
							break;
						case 2: // Joystick Down
							Joystick[i].setYAxis(!currentJoyState);
							break;
						case 3: // Joystick Left
							Joystick[i].setXAxis(-!currentJoyState);
							break;
						case 1: // Joystick Right
							Joystick[i].setXAxis(!currentJoyState);
							break;
						}
					}
				}
				//Save the last state for each axis for next time
				lastJoyState[i][jb[i]] = currentJoyState;

				++jb[i];
			} while (jb[i] < 4);

			//Iterate through the buttons (0-8) assigning the current state of the pin for each button, HIGH(0b00000001) or LOW(0b00000000), to the currentState variable
			button[i] = 0;

			do
			{
				if (i == 0) // For first set of joystick button inputs, read states from Arduino Pins
				{
					switch (button[i])
					{
					case 0: //on digital pin 4, PD4 - Arcade Button 1
						currentButtonState = (PIND & 0b00010000) >> 4; //logical AND the 8-bit pin reading with a mask to isolate the specific bit we're interested in and then shift it to the end of the byte
						break;
					case 1: //on digital pin 5, PC6 - Arcade Button 2
						currentButtonState = (PINC & 0b01000000) >> 6;
						break;
					case 2: //on digital pin 6, PD7 - Arcade Button 3
						currentButtonState = (PIND & 0b10000000) >> 7;
						break;
					case 3: //on digital pin 7, PE6 - Arcade Button 4
						currentButtonState = (PINE & 0b01000000) >> 6;
						break;
					case 4: //on digital pin 8, PB4 - Arcade Button 5
						currentButtonState = (PINB & 0b00010000) >> 4;
						break;
					case 5: //on digital pin 9, PB5 - Arcade Button 6
						currentButtonState = (PINB & 0b00100000) >> 5;
						break;
					case 6: //on digital pin 10, PB6 - Arcade Button 7
						currentButtonState = (PINB & 0b01000000) >> 6;
						break;
					case 7: //on digital pin 11, PB7 - Arcade Button 8
						currentButtonState = (PINB & 0b10000000) >> 7;
						break;
					case 8: //on digital pin 12, PD6 - Arcade Button 9
						currentButtonState = (PIND & 0b01000000) >> 6;
						break;
					case 9: //on digital pin MISO, PB3 - Arcade Button 10
							currentButtonState = (PINB & 0b00001000) >> 3; // MISO
							break;
					default: //should never happen
						currentButtonState = 0b00000000;
						break;
					}
				}
				else // For second set of joystick button inputs, read states from 74HC165 pair
				{
					switch (button[i])
					{
					case 0: //74HC165(2) Input (A) - Arcade Button 1
						currentButtonState = (ShiftReg & 0b0000000100000000) >> 8; //logical AND the 16-bit pin reading with a mask to isolate the specific bit we're interested in and then shift it to the end of the byte
						break;
					case 1: //74HC165(2) Input (B) - Arcade Button 2
						currentButtonState = (ShiftReg & 0b0000001000000000) >> 9;
						break;
					case 2: //74HC165(2) Input (C) - Arcade Button 3
						currentButtonState = (ShiftReg & 0b0000010000000000) >> 10;
						break;
					case 3: //74HC165(2) Input (D) - Arcade Button 4
						currentButtonState = (ShiftReg & 0b0000100000000000) >> 11;
						break;
					case 4: //74HC165(2) Input (E)  - Arcade Button 5
						currentButtonState = (ShiftReg & 0b0001000000000000) >> 12;
						break;
					case 5: //74HC165(2) Input (F) - Arcade Button 6
						currentButtonState = (ShiftReg & 0b0010000000000000) >> 13;
						break;
					case 6: //74HC165(2) Input (G) - Arcade Button 7
						currentButtonState = (ShiftReg & 0b0100000000000000) >> 14;
						break;
					case 7: //74HC165(2) Input (H) - Arcade Button 8
						currentButtonState = (ShiftReg & 0b1000000000000000) >> 15;
						break;
					case 8: //74HC165(1) Input (A) - Arcade Button 9
						currentButtonState = (ShiftReg & 0b0000000000000001) >> 0;
						break;
					case 9: //74HC165(1) Input (B) - Arcade Button 10
							currentButtonState = (ShiftReg & 0b0000000000000010) >> 1;
							break;
					default: //should never happen
						currentButtonState = 0b0000000000000000;
						break;
					}
				}

				//If the current state of the pin for each button is different than last time, update the joystick button state
				if (currentButtonState != lastButtonState[i][button[i]])
					Joystick[i].setButton(button[i], !currentButtonState);

				//Save the last button state for each button for next time
				lastButtonState[i][button[i]] = currentButtonState;

				++button[i];
			} while (button[i] < maxBut);
		}
		break;
	}
}