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
//	V2 - April 29, 2016 
//     - Added auto-fire control
//  v2.1 - February 26, 2023
//     - Fixed ghost autofired buttons with original NES controllers
//     - Added Pullup on DATA pin
//     - Unified NES/SNES PC Button mapping
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
//  Y Axis   -  Up/Down    -   Up/Down  
//  X Axis   - Left/Right  -  Left/Right
//  Button 1 -     A       -      A     
//  Button 2 -     B       -      B     
//  Button 3 -     x       -      -     
//  Button 4 -     Y       -      -     
//  Button 5 -   Top L     -      -   
//  Button 6     Top R     -      -   
//  Button 7 -   Select    -    Select  
//  Button 8 -   Start     -    Start  
//  
// See details about the SNES controller protocol at the ending remarks

enum controllerType {
  SNES = 0,
  NES,
  NONE = 0xff
};
					
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
  uint16_t combinedButtons=0;
  
  uint8_t i, controllerType;
  uint16_t dataIn;

  //latch buttons state. After latching, first button is ready at data output
  digitalWrite(latchPin, HIGH);
  delayMicroseconds(12);
  digitalWrite(latchPin, LOW);
  digitalWrite(clockPin, LOW);

  // read 16 bits from the controller
  dataIn = 0;
  for (i = 0; i < 16; i++) {
    dataIn >>= 1;
    if (digitalRead(dataPin) == 0) dataIn |= (1 << 15) ; // shift in one bit
    Pulse_clock();
  }

  if (digitalRead(dataPin)) { // 17th bit received should be a zero if an original controller is attached
    controllerType = NONE;
  } else {
    if ( (dataIn & 0xf000) == 0x0000)
      controllerType = SNES;
    else  if ( (dataIn & 0xff00) == 0xff00)
      controllerType = NES;
    else
      controllerType = NONE;
  }

  // Return Clock signal to idle level.
  digitalWrite(clockPin, HIGH);


    //   11  10   9   8   7   6   5   4   3   2   1   0
    //   Up  Dw   Lf  Rt  St  Sl  Tr  Tl  Y   X   B   A 

  combinedButtons = 0;

  // Common buttons
  if (dataIn & (1 << 2 )) combinedButtons |= (1 << 6 ); // Select
  if (dataIn & (1 << 3 )) combinedButtons |= (1 << 7 ); // Start
  if (dataIn & (1 << 4 )) combinedButtons |= (1 << 11); // Up
  if (dataIn & (1 << 5 )) combinedButtons |= (1 << 10); // Down
  if (dataIn & (1 << 6 )) combinedButtons |= (1 << 9 ); // Left
  if (dataIn & (1 << 7 )) combinedButtons |= (1 << 8 ); // Right


  if (controllerType == SNES)  {
    if (dataIn & (1 << 8 )) combinedButtons |= (1 << 0 ); // A
    if (dataIn & (1 << 0 )) combinedButtons |= (1 << 1 ); // B
    if (dataIn & (1 << 9 )) combinedButtons |= (1 << 2 ); // X
    if (dataIn & (1 << 1 )) combinedButtons |= (1 << 3 ); // Y
    if (dataIn & (1 << 10)) combinedButtons |= (1 << 4 ); // L
    if (dataIn & (1 << 11)) combinedButtons |= (1 << 5 ); // R

  } else { //  NES / Knockoff
    if (dataIn & (1 << 0 )) combinedButtons |= (1 << 0 ); // A
    if (dataIn & (1 << 1 )) combinedButtons |= (1 << 1 ); // B
  }



  return combinedButtons;
}

//

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
	pinMode(dataPin,INPUT_PULLUP);
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
	Buttons_low=(uint8_t)(Buttons & 0xff);    //   St  Sl  Tr  Tl  Y   X   B   A  button  
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
/*  Controller wavefrorms
                ____
    Latch  ____/    \_____________________________________________________________________________________________________
           ____________    __    __    __    __    __    __    __    __    __    __    __    __    __    __    ___    ____ 
    Clock              \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/   \__/         

    SNES   _________                                                                                                      
    Data            [  B ][  Y ][ Sl ][ St ][ UP ][DOWN][LEFT][RIGH][  A ][  X ][TopL][TopR][  1 ][  1 ][  1 ][  1 ][  0 ]

    NES    _________                                                
    Data            [  A ][  B ][ Sl ][ St ][ UP ][DOWN][LEFT][RIGH][  0 ][  0 ][  0 ][  0 ][  0 ][  0 ][  0 ][  0 ][  0 ]
    
    Clone NES ______                                                
    Data            [  A ][  B ][ Sl ][ St ][ UP ][DOWN][LEFT][RIGH][  1 ][  1 ][  1 ][  1 ][  1 ][  1 ][  1 ][  1 ][  1 ]  
    
    Nth bit         <  1 ><  2 ><  3 ><  4 ><  5 ><  6 ><  7 ><  8 ><  9 >< 10 >< 11 >< 12 >< 13 >< 14 >< 15 >< 16 >< 17 >

    Bits 14 to 17 can be used to differentiate between controllers -------------------------------|______________________|

*/
	
	
