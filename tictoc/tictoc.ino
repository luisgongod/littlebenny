
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

String clk_status[4] = { "PLAY", "PAUSE", "STOP", "ERR!"};

int bpm = 120; //initial beats per minute
byte ppb = 4; //pulses per beat
int gate_lenght = 10; //ms (5 to 50ms) max of 62ms for 16ppb @ 240 BPM
// bool play = true; //1 for play, 0 for pause or stop
bool focus_state = false; //1 for focus ON, 0 for focus OFF

byte enum_focus_items = 3;
enum enum_focus {
    FOCUS_BPM,
    FOCUS_PPB,
    FOCUS_GATE
};

byte enum_clock_items = 3;
enum enum_clock_state {
    CLK_PLAY,
    CLK_PAUSE,
    CLK_STOP
};

byte clk_item = CLK_PLAY;

#define startTriggerPin 3 // Start trigger pin
#define stopTriggerPin 2 // Stop trigger pin

// 74HC595 pins 
byte outputLatchPin = 5; 
byte outputClockPin = 6; 
byte outputDataPin = 4;  

// Current en previous states of rotary pins
byte currentStateSW; 
byte previousStateSW;
byte currentStateCLK;
byte previousStateCLK; 

// Rotary Encoder Inputs pins
#define inputCLK 9
#define inputDT 8
#define inputSW 7

#define max_ppb 4
#define min_ppb 1
#define max_bpm 240
#define min_bpm 60
#define max_gate_lenght 50
#define min_gate_lenght 10
// #define max_beats_count 16

// Current menu item
byte focusItem = 0;
byte beat_counter = 0;

unsigned long eventTime_1_beat ;
const unsigned long eventTime_2_screen = 500;//half the interval in ms

unsigned long previousTime_1 = 0;
unsigned long previousTime_2 = 0;
bool gate_up = false;

void setup() {
    Serial.begin(9600);

    // Set all the pins of 74HC595 as OUTPUT
    pinMode(outputLatchPin, OUTPUT);
    pinMode(outputDataPin, OUTPUT);  
    pinMode(outputClockPin, OUTPUT);

    // Rotary inputs
    pinMode (inputCLK,INPUT_PULLUP);
    pinMode (inputDT,INPUT_PULLUP);
    pinMode (inputSW,INPUT); 

    attachInterrupt(digitalPinToInterrupt(stopTriggerPin), stopTrigger, FALLING );
    attachInterrupt(digitalPinToInterrupt(startTriggerPin), playTrigger, FALLING );

    eventTime_1_beat = GetBeatInterval(); // interval in ms

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
    }

    display.display();
    display.clearDisplay();
    display.setRotation(2);
    display.setTextSize(2);   

    display.setRotation(2);
    display.setTextColor(SSD1306_WHITE);
    display.dim(true);

    display.setCursor(0,0);    
    display.setTextSize(2);      
    display.print(F("BigBen")); 
    display.display();    
    delay(200);
    display.print(F("."));   
    display.display();
    delay(200);
    display.print(F("."));   
    display.display();
    delay(200);
    display.print(F("."));   
    display.display();
    delay(200);

    printScreen();
}

void loop() {
    CheckRotary();
    CheckRotarySwitch();    
        
    unsigned long currentTime = millis();

    if(clk_item == CLK_PLAY) {

    if ( currentTime - previousTime_1 >= eventTime_1_beat ) {
        beat_counter++;
        gate_up = true;
        previousTime_1 = currentTime;
        previousTime_2 = currentTime;

        // Serial.print(eventTime_1_beat);    
        // Serial.print("\t");    
        // Serial.print(beat_counter);
        // Serial.print("\t");    
        // Serial.println(beat_counter,BIN);

        
        outputClock(beat_counter);
    }

    
    if ( gate_up ) {
        if ( currentTime - previousTime_2 >= gate_lenght ) {            
        outputClock(0b00000000);
        }
    }

    }

 
}


// Check if rotary switch has been pushed
// Set the menuState
void CheckRotarySwitch() {
    
   currentStateSW = digitalRead(inputSW);
   if (currentStateSW != previousStateSW && currentStateSW == 0) {
        delay(50);//debounce
        if(focusItem == enum_focus_items-1) {
            focusItem = 0;
        } else {
            focusItem++;
        }
    //   displayMenuItem();
        printScreen();
   }
   previousStateSW = currentStateSW; 
}



void CheckRotary() {
   currentStateCLK = digitalRead(inputCLK);
   
   if (currentStateCLK != previousStateCLK && currentStateCLK == 0){  

    
      switch(focusItem) {
        case FOCUS_BPM:     
          UpdateBPM();
          break;
        case FOCUS_PPB:
          Updateppb();
          break;
        case FOCUS_GATE:
          UpdateGateLenght();
          break;


        
     }
    //updating beat interval
    eventTime_1_beat = GetBeatInterval();    
    printScreen();  
   }   
   previousStateCLK = currentStateCLK;
}


// Rotary updates BPM 
void UpdateBPM() {
  if (digitalRead(inputDT) != currentStateCLK) { 
     bpm++; 
   } else {
     bpm--;
   }

    if(bpm > max_bpm) {
      bpm = max_bpm;
    }
    if(bpm < min_bpm) {
      bpm = min_bpm;
    }


//    displayBPMBasedOnStateClock();
//    updateTimer();
}

// Rotary updates BPM 
void UpdateGateLenght() {
  if (digitalRead(inputDT) != currentStateCLK) { 
     gate_lenght+=5; 
   } else {
     gate_lenght-=5;
   }

   if(gate_lenght > max_gate_lenght) {
     gate_lenght = max_gate_lenght;
   }
   if(gate_lenght < min_gate_lenght) {
     gate_lenght = min_gate_lenght;
   }

}

// Rotary updates BPM 
void Updateppb() {
  if (digitalRead(inputDT) != currentStateCLK) { 
     ppb*=2; 
   } 
   else {
     ppb/=2;
   }   

   if (ppb> max_ppb){
    ppb = max_ppb;
   }
    if (ppb< min_ppb){
     ppb = min_ppb;
    }   
}
void printScreen(void) {
//bpm,ppb,clk_status

    display.clearDisplay();
    display.setCursor(0,0);    
    display.setTextSize(2);      
    
    display.print(clk_status[clk_item]); 

    //BPM
    display.setCursor(60,0);
    display.setTextSize(2);   
    if(focusItem == FOCUS_BPM ) {
        display.print(F(">"));
    }
    display.setCursor(70,0);
    display.setTextSize(3);  
    if(bpm<100){
        display.print(F(" "));
    } 
    display.print(bpm);

    //PPB
    display.setCursor(0,20);    
    display.setTextSize(1);                  
    if(focusItem == FOCUS_PPB ) {
        display.print(F(">"));
    }
    display.setCursor(5,20); 
    display.print(ppb);     
    display.print(F("ppb"));   
    
    //GATE
    display.setTextSize(1);              
    display.setCursor(75,25);    
    if(focusItem == FOCUS_GATE ) {
        display.print(F(">"));
    }
    display.setCursor(80,25);    
    display.print(gate_lenght);     
    display.print(F("Gate"));   
    
    display.display();
}

//Stop button
void stopTrigger() {
    delay(10);
    clk_item= CLK_STOP;
    // clk_item= 2;
    beat_counter = 0;
    gate_up = false;
    // outputClock(0b00000000);
    
    // printScreen();
}

// Play Pause the clock
void playTrigger() {   
    delay(10);
        // clk_item = 1;
    if(clk_item == CLK_PLAY) {
        clk_item = CLK_PAUSE;
    }
    else {
        clk_item = CLK_PLAY;
        // clk_item = 0;
    }
    // printScreen();
}

// Output the clock/reset
void outputClock(byte pins){
   digitalWrite(outputLatchPin, LOW);
   shiftOut(outputDataPin, outputClockPin, LSBFIRST, pins);
   digitalWrite(outputLatchPin, HIGH);
}

long GetBeatInterval() {
    //ms per beat, based on BPM and ppb (pulses per beat)
  return 60L * 1000 / (bpm * ppb*2);
}