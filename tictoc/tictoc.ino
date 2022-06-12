/**
 * Adapted sketch for the modified "LittleBen" module by Quinie
 * https://www.quinie.nl/
 * 
 * modified by: Luisgongod
 * https://github.com/luisgongod/littlebenny
 * */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Oled display settings
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 32 
#define OLED_RESET     4 
#define SCREEN_ADDRESS 0x3C

// Interrupt pin for the buttons
#define startTriggerPin 3 // Start trigger pin
#define stopTriggerPin 2 // Stop trigger pin

// Rotary Encoder Inputs pins
#define inputCLK 9
#define inputDT 8
#define inputSW 7

//  Max min values
#define max_ppb 4
#define min_ppb 1
#define max_bpm 240
#define min_bpm 60
#define gate_step 5
#define max_gate_lenght 50
#define min_gate_lenght 10
#define debouncetime 10 //for buttons, in ms


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//status definiton items
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

String clk_status[4] = { "PLAY", "PAUSE", "STOP", "ERR!"};

// 74HC595 pins 
byte outputLatchPin = 5; 
byte outputClockPin = 6; 
byte outputDataPin = 4;  

// Current en previous states of rotary pins
byte currentStateSW; 
byte previousStateSW;
byte currentStateCLK;
byte previousStateCLK; 

// Current menu item
byte focusItem = 0;
byte beat_counter = 0;
byte beat_counter_old = 0;

// Initial values
int bpm = 120; //initial beats per minute
byte ppb = 4; //initial pulses per beat
int gate_lenght = 10; //ms (10 to 50ms) max of 62ms for 16ppb @ 240 BPM
byte clk_item = CLK_PLAY; //initial clock state
bool ready2print = false; //true if the display needs to be updated

unsigned long eventTime_1_beat ;
unsigned long previousTime_1 = 0;
unsigned long previousTime_2 = 0;
bool gate_up = false; //true if the gate is up

void setup() {    

    // Serial.begin(9600); //for debugging

    // Set all the pins of 74HC595 as OUTPUT
    pinMode(outputLatchPin, OUTPUT);
    pinMode(outputDataPin, OUTPUT);  
    pinMode(outputClockPin, OUTPUT);

    // Rotary inputs
    pinMode (inputCLK,INPUT_PULLUP);
    pinMode (inputDT,INPUT_PULLUP);
    pinMode (inputSW,INPUT); 

    // Interrupt pin for the buttons
    attachInterrupt(digitalPinToInterrupt(stopTriggerPin), stopTrigger, FALLING );
    attachInterrupt(digitalPinToInterrupt(startTriggerPin), playTrigger, FALLING );

    // calculate the time of the beat
    eventTime_1_beat = GetBeatInterval(); // interval in ms

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        // Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
    }
    
    display.clearDisplay();
    display.setRotation(2); //set the display to rotate 180 degrees

    display.setTextSize(2);       
    display.setTextColor(SSD1306_WHITE);
    display.dim(true); //dim the display to 50%, better to avoid noise

    display.setCursor(0,0);       

    
    String title[2] = { "Little", "Benny"};

    for(byte i = 0; i < 2; i++){
        display.setCursor(i*16+10,i*16);
        for(int j = 0; j < title[i].length(); j++) {
            display.print(title[i][j]);
            display.display();
            delay(50);
        }
        delay(200);
    }
    delay(300);

    printScreen();
}

void loop() {
    CheckRotary();
    CheckRotarySwitch();    
        
    unsigned long currentTime = millis();

    if(clk_item == CLK_PLAY) {

        if ( currentTime - previousTime_1 >= eventTime_1_beat ) {
            beat_counter++;
            byte beat_counter_diff = (beat_counter ^ beat_counter_old) & beat_counter ;
            
            gate_up = true;

            previousTime_1 = currentTime;
            previousTime_2 = currentTime;
            
            outputClock(beat_counter_diff);
            beat_counter_old = beat_counter;
        }

        
        if ( gate_up ) {
            if ( currentTime - previousTime_2 >= gate_lenght ) {            
            outputClock(0b00000000); //reset the clock output
            }
        }

    }

    // update the display if needed
    if(ready2print) {
        printScreen();
        ready2print = false;
    }

 
}


// Check if rotary switch has been pushed
// Set the focus item to the next item
void CheckRotarySwitch() {
    
   currentStateSW = digitalRead(inputSW);
   if (currentStateSW != previousStateSW && currentStateSW == 0) {
        delay(debouncetime);//debounce
        if(focusItem == enum_focus_items-1) {
            focusItem = 0;
        } else {
            focusItem++;
        }
    ready2print = true;
   }
   previousStateSW = currentStateSW; 
}

// Check if rotary encoder has been turned, and update the value
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
    //updating beat interval (depended on BPM and ppb)
    eventTime_1_beat = GetBeatInterval();    
    ready2print = true;
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
}

// Rotary updates Gate Lenght in steps 
void UpdateGateLenght() {
    if (digitalRead(inputDT) != currentStateCLK) { 
        gate_lenght+=gate_step; 
    } else {
        gate_lenght-=gate_step ;
    }

    if(gate_lenght > max_gate_lenght) {
        gate_lenght = max_gate_lenght;
    }
    if(gate_lenght < min_gate_lenght) {
        gate_lenght = min_gate_lenght;
    }
}

// Rotary updates ppb
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

// Print the display bpm,ppb,gate, clk_status
void printScreen(void) {

    display.clearDisplay();
    display.setCursor(0,0);    
    display.setTextSize(2);      
    
    //status
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

//Stop button, will reset outputs and counter
void stopTrigger() {
    delay(debouncetime);
    clk_item= CLK_STOP;    
    beat_counter = 0;
    beat_counter_old = 0;
    gate_up = false;
    ready2print = true;
}

// Play Pause the clock
void playTrigger() {   
    delay(debouncetime);        
    if(clk_item == CLK_PLAY) {
        clk_item = CLK_PAUSE;
    }
    else {
        clk_item = CLK_PLAY;        
    }
    ready2print = true;
}

// Output the clock/reset to the 74HC595
void outputClock(byte pins){
   digitalWrite(outputLatchPin, LOW);
   shiftOut(outputDataPin, outputClockPin, LSBFIRST, pins);
   digitalWrite(outputLatchPin, HIGH);
}

long GetBeatInterval() {
    //ms per beat, based on BPM and ppb (pulses per beat)
  return 60L * 1000 / (bpm * ppb * 2);
}