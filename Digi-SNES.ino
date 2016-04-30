////////////////////////////////////////////////////////////////////////////////
//  ___  _      _     ___             
// |   \(_)__ _(_)___/ __|_ _  ___ ___                  )
// | |) | / _` | |___\__ \ ' \/ -_|_-<                 ) \
// |___/|_\__, |_|   |___/_||_\___/__/                / ) (
//        |___/                                       \(_)/ Wny
//
//  DIGI SNES - SNES to USB adapter using Digispark 
//  Danjovic, 2015-2016 danjovic@hotmail.com        
//  
//	V2 - April 29, 2016 - Added auto-fire control
//                                                  
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//    _    _ _                 _        
//   | |  (_) |__ _ _ __ _ _ _(_)___ ___
//   | |__| | '_ \ '_/ _` | '_| / -_|_-<
//   |____|_|_.__/_| \__,_|_| |_\___/__/
//                                      
////////////////////////////////////////////////////////////////////////////////
#include "DigiJoystick.h"


////////////////////////////////////////////////////////////////////////////////
//    ___       __ _      _ _   _             
//   |   \ ___ / _(_)_ _ (_) |_(_)___ _ _  ___
//   | |) / -_)  _| | ' \| |  _| / _ \ ' \(_-<
//   |___/\___|_| |_|_||_|_|\__|_\___/_||_/__/
//                                            
////////////////////////////////////////////////////////////////////////////////


// Pin Assignment         SNES                                       NES
                    //   /---\                        
                    //   | O | GND   (Brown)          
                    //   | O |                                     +----\
                    //   | O |                      (Brown)   GND  | O   \
                    //   +---+                       (Red)   CLOCK | O  O |
#define dataPin  0  //   | O | DATA  (Red)          (Orange)  DATA | O  O |
#define latchPin 1  //   | O | LATCH (Yellow)       (Yellow) LATCH | O  O |
#define clockPin 2  //   | O | CLOCK (Blue)                        +------+
                    //   | O | +5V   (White)          
                    //   +---+

//
// USB Joystick Mapping
//  
//   USB       == SNES ==     == NES ==
//  Y Axis   -  Up/Down    -  Up/Down  
//  X Axis   - Left/Right  - Left/Right 
//  Button 1 -     X       -     A
//  Button 2 -     A       -     B
//  Button 3 -     B       -   Select  
//  Button 4 -     Y       -   Start
//  Button 5 -   Top L     - 
//  Button 6     Top R     - 
//  Button 7 -   Select    - 
//  Button 8 -   Start     - 
//  
// See details about the SNES controller protocol at the ending remarks


					
////////////////////////////////////////////////////////////////////////////////					
//   __   __        _      _    _        
//   \ \ / /_ _ _ _(_)__ _| |__| |___ ___
//    \ V / _` | '_| / _` | '_ \ / -_|_-<
//     \_/\__,_|_| |_\__,_|_.__/_\___/__/
//                                       
////////////////////////////////////////////////////////////////////////////////
boolean pinState = false; 
uint8_t Buttons_low;
uint8_t Buttons_high;
uint8_t X_Axis;
uint8_t Y_Axis;
uint8_t Autofire_modulation,flip_flop=0;


////////////////////////////////////////////////////////////////////////////////
//    ___             _   _             
//   | __|  _ _ _  __| |_(_)___ _ _  ___
//   | _| || | ' \/ _|  _| / _ \ ' \(_-<
//   |_| \_,_|_||_\__|\__|_\___/_||_/__/
//                                      
////////////////////////////////////////////////////////////////////////////////

// Pulse clock line. 
inline void Pulse_clock() {
	delayMicroseconds(6);
	digitalWrite(clockPin, HIGH);
	delayMicroseconds(6);
	digitalWrite(clockPin, LOW);
}

// Get buttons state. 
uint16_t get_buttons(void) {
    //   shift in bits from controller and return them using the following format
    //   11  10   9   8   7   6   5   4   3   2   1   0
    //   Up  Dw   Lf  Rt  St  Sl  Tr  Tl  Y   B   A   X
    //
	uint16_t buttons=0;

	//latch buttons state. After latching, button B state is ready at data output
	digitalWrite(latchPin, HIGH);
	delayMicroseconds(12);
	digitalWrite(latchPin, LOW);
	digitalWrite(clockPin,LOW);

	// read the buttons	and store their values
	if (!digitalRead(dataPin)) buttons |= (1<<2); // B
	Pulse_clock();
	
	if (!digitalRead(dataPin)) buttons |= (1<<3); // Y
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<7); // Select 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<6); // Start 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<11); // Up 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<10); // Down 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<9); // Left 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<8); // Right 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<1); // A  
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<0); // X 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<4); // Top Left 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<5); // Top Right 
	Pulse_clock();

	// Return Clock signal to idle level.
	digitalWrite(clockPin,HIGH);
	
	return buttons;
}	

////////////////////////////////////////////////////////////////////////////////
//    ___      _             
//   / __| ___| |_ _  _ _ __ 
//   \__ \/ -_)  _| || | '_ \
//   |___/\___|\__|\_,_| .__/
//                     |_|   
////////////////////////////////////////////////////////////////////////////////
void setup(){
	uint16_t Buttons_on_at_startup;
	
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
	
	// Now sample buttons and initialize autofire mask
	Buttons_on_at_startup = get_buttons();
	Autofire_modulation = (uint8_t)(Buttons_on_at_startup & 0xff);
	Autofire_modulation &= 0x3f; // Isolate Start and Select
	Autofire_modulation = ~Autofire_modulation; // invert bits.
	flip_flop = 0;
}


////////////////////////////////////////////////////////////////////////////////
//    __  __      _        ___             _   _          
//   |  \/  |__ _(_)_ _   | __|  _ _ _  __| |_(_)___ _ _  
//   | |\/| / _` | | ' \  | _| || | ' \/ _|  _| / _ \ ' \ 
//   |_|  |_\__,_|_|_||_| |_| \_,_|_||_\__|\__|_\___/_||_|
//                                                        
////////////////////////////////////////////////////////////////////////////////
void loop(){
	//Grab the buttons state. At this time B value is already at Data output
	uint16_t Buttons = get_buttons();

	// fill in buttons                             7   6   5   4   3   2   1   0  bit
	Buttons_low=(uint8_t)(Buttons & 0xff);    //   St  Sl  Tr  Tl  Y   B   A   X  button  
	Buttons_high=0;                       // SNES controller uses only 8 out of 16 buttons                     

	// Now modulate buttons with autofire 
	if (++flip_flop & (1<<1)) {  // do it once at each 2 runs of 50ms (100ms turn rate or 10Hz)
		Buttons_low &= Autofire_modulation; // Clear the bits set to autofire
	}
	
	// Initialize Axes (centered) and buttons (none pressed)
	X_Axis = 128;     // 0 = left, 128 = center, 255 = right
	Y_Axis = 128;     // 0 = up  , 128 = center, 255 = down

	// Now test for the directional buttons
	if (Buttons & (1<<11)) Y_Axis-=128; // Up
	if (Buttons & (1<<10)) Y_Axis+=127; // Down
	if (Buttons & (1<<9))  X_Axis-=128; // Left
	if (Buttons & (1<<8))  X_Axis+=127; // Right


	DigiJoystick.setX((byte) X_Axis);
	DigiJoystick.setY((byte) Y_Axis);
	DigiJoystick.setButtons((char) Buttons_low , (char) Buttons_high );
	DigiJoystick.delay(25);
}


////////////////////////////////////////////////////////////////////////////////
//    ___         _               _   ___      _        _ _    
//   | _ \_ _ ___| |_ ___  __ ___| | |   \ ___| |_ __ _(_) |___
//   |  _/ '_/ _ \  _/ _ \/ _/ _ \ | | |) / -_)  _/ _` | | (_-<
//   |_| |_| \___/\__\___/\__\___/_| |___/\___|\__\__,_|_|_/__/
//                                                             
////////////////////////////////////////////////////////////////////////////////

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
	
	

