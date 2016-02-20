/*
   DIGI SNES - SNES to USB adapter using Digispark                 )
   Danjovic, 2015 - danjovic@hotmail.com                          ) \
                                                                 / ) (
                                                                 \(_)/ Wny
*/

/*  SNES Controller Protocol
                ____
    Latch  ____/    \____________________________________________________________________________________________________ 
           ____________    __    __    __    __    __    __    __    __    __    __    __    __    __    __    __________  
    Clock              \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/             
           _________                                                                                                _____
    Data            [  B ][  Y ][ Sl ][ St ][ UP ][DOWN][LEFT][RIGH][  A ][  X ][TopL][TopR][  1 ][  1 ][  1 ][  1 ]

	*/

/*  NES Controller Protocol
                ____
    Latch  ____/    \_____________________________________________________ 
           ____________    __    __    __    __    __    __    __    _____  
    Clock              \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/        
           _________                                                ______
    Data            [  A ][  B ][ Sl ][ St ][ UP ][DOWN][LEFT][RIGH]

	*/	
	
	
	
	
// USB Joystick Mapping
/*
 USB       == SNES ==     == NES ==
Y Axis   -  Up/Down    -  Up/Down  
X Axis   - Left/Right  - Left/Right 
Button 1 -     X       -     A
Button 2 -     A       -     B
Button 3 -     B       -   Select  
Button 4 -     Y       -   Start
Button 5 -   Top L     - 
Button 6     Top R     - 
Button 7 -   Select    - 
Button 8 -   Start     - 
*/


#include "DigiJoystick.h"

// Pin Assignment        SNES                                       NES
                    //   /---\                        
                    //   | O | GND   (Brown)          
     SNES           //   | O |                                     +----\
                    //   | O |                      (Brown)   GND  | O   \
                    //   +---+                       (Red)   CLOCK | O  O |
int dataPin  = 0;   //   | O | DATA  (Red)          (Orange)  DATA | O  O |
int latchPin = 1;   //   | O | LATCH (Yellow)       (Yellow) LATCH | O  O |
int clockPin = 2;   //   | O | CLOCK (Blue)                        +------+
                    //   | O | +5V   (White)          
                    //   +---+

// Variables
boolean pinState = false; 
uint8_t Buttons_low;
uint8_t Buttons_high;
uint8_t X_Axis;
uint8_t Y_Axis;

// Pulse clock line. 
inline void Pulse_clock() {
	delayMicroseconds(6);
	digitalWrite(clockPin, HIGH);
	delayMicroseconds(6);
	digitalWrite(clockPin, LOW);
}


// Setup Hardware
void setup(){
	pinMode(clockPin,OUTPUT);
	pinMode(latchPin,OUTPUT);
	pinMode(dataPin,INPUT);
	digitalWrite(clockPin,HIGH);
	digitalWrite(latchPin,LOW);


	DigiJoystick.setX((byte) 0x80);
	DigiJoystick.setY((byte) 0x80);

	// Initialize non used axes	
	DigiJoystick.setXROT((byte) 0x80);
	DigiJoystick.setYROT((byte) 0x80);
	DigiJoystick.setZROT((byte) 0x80);
	DigiJoystick.setSLIDER((byte) 0x80);
}


// Main Loop
void loop(){
	//Grab the buttons state. At this time B value is already at Data output
	digitalWrite(latchPin, HIGH);
	delayMicroseconds(12);
	digitalWrite(latchPin, LOW);
	digitalWrite(clockPin,LOW);
	
	// Initialize Axes (centered) and buttons (none pressed)
	X_Axis = 128;     // 0 = left, 128 = center, 255 = right
	Y_Axis = 128;     // 0 = up  , 128 = center, 255 = down

	Buttons_high=0;   //   7   6   5   4   3   2   1   0
	Buttons_low=0;    //   St  Sl  Tr  Tl  Y   B   A   X    


	// Now read 12 bits and convert it to the report values

	// B
	if (!digitalRead(dataPin)) Buttons_low |= (1<<2); 
	Pulse_clock();

	// Y
	if (!digitalRead(dataPin)) Buttons_low |= (1<<3); 
	Pulse_clock();

	// Select
	if (!digitalRead(dataPin)) Buttons_low |= (1<<6); 
	Pulse_clock();

	// Start
	if (!digitalRead(dataPin)) Buttons_low |= (1<<7); 
	Pulse_clock();

	// Up
	if (!digitalRead(dataPin)) Y_Axis-=128; 
	Pulse_clock();

	// Down
	if (!digitalRead(dataPin)) Y_Axis+=127;
	Pulse_clock();

	// Left
	if (!digitalRead(dataPin)) X_Axis-=128;
	Pulse_clock();

	// Right
	if (!digitalRead(dataPin)) X_Axis+=127;
	Pulse_clock();

	// A 
	if (!digitalRead(dataPin)) Buttons_low |= (1<<1); 
	Pulse_clock();

	// X
	if (!digitalRead(dataPin)) Buttons_low |= (1<<0); 
	Pulse_clock();

	// Top Left
	if (!digitalRead(dataPin)) Buttons_low |= (1<<4); 
	Pulse_clock();

	// Top Right
	if (!digitalRead(dataPin)) Buttons_low |= (1<<5); 
	Pulse_clock();

	// Return Clock signal to idle level.
	digitalWrite(clockPin,HIGH);

	//Set the Axes and Buttons on the USB-HID-Controler	

/*
 X_Axis 
 0 -> left
 255 -> right
Y_Axis
 0-> UP
 255-> down
 
*/
  
	DigiJoystick.setX((byte) X_Axis);
	DigiJoystick.setY((byte) Y_Axis);
	DigiJoystick.setButtons((char) Buttons_low , (char) Buttons_high );
	DigiJoystick.delay(25);
}


