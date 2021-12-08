
// importing libraries OLED libraries
#include <Adafruit_SSD1306.h>
#include <splash.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>

// defining OLED specifications
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// long term plan: non-volatile storage with reset button

// Define the number of samples to keep track of. The higher the number, the
// more the readings will be smoothed, but the slower the output will respond to
// the input. Using a constant rather than a normal variable lets us use this
// value to determine the size of the readings array.

//initiate variables that will hold the temperature readings
float temp[3];

//initiate arrays that will hold the incoming temperature values for the last two hours for each sensor
float tempArray[3][120];
int c = 0; // counter for shuffling items in tempArray

#define numReadings 10 // number of readings smoothed
#define threshAnalog 400 // threshold of analog reading to make sure only reasonable data values are stored

//initiate three temperature probe sensors
#define analogPin0 A2 // black
#define analogPin1 A3 // red
#define analogPin2 A6 // orange
int analogPins[3] = {A2, A3, A6};

float analogReadings[3][numReadings]; // what the analog pins are reading in; floats are fine bc we use the readings in math later

// temperature calibration curves for each probe (0, 1, 2) in the form (y = mx + b); easily adjusted for new calibrations
// calibrated on breadboard
#define m0 0.1021
#define b0 -27.994
#define m1 0.1019
#define b1 -27.999
#define m2 0.1021
#define b2 -28.016

// calibrated on PCB
//#define m0 0.0251
//#define b0 41.213
//#define m1 0.0251
//#define b1 41.217
//#define m2 0.0248
//#define b2 41.228
float m[3] = {m0, m1, m2};
float b[3] = {b0, b1, b2};

int readIndex[3] = {0, 0, 0}; // index of the current reading
float total[3] = {0.0, 0.0, 0.0}; // running total
float average[3] = {0.0, 0.0, 0.0}; // average of analog readings

// variables for averaging temp in 30 min intervals
float sum[3][4];
float averageT[3][4];
#define numReadings2 30

//initiation of variables for LED, buttons, and buzzer
#define LED 2
#define buttonR 5 // turn alarm off (red button)
#define Buzzer 10

// using interrupts 
#define buttonG 3 // toggle between 2 display modes (black button)
int BP = 1; // button press counter (corresponds to screens 1-4)

// alarm stuff
bool snooze = false; // has someone attempted to turn alarm off?
int alarmBP = 0; // button press counter for alarm snoozing

void setup() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){ // check initialization of OLED
  Serial.println("SSD1306 allocation failed");
    for (;;);
    }
  
  // set up OLED display
  display.clearDisplay();
  display.setTextSize(1.75);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0); 

  Serial.begin(9600);

  // initiating analog pins for temp probes
  pinMode(analogPin0, INPUT);  
  pinMode(analogPin1, INPUT);
  pinMode(analogPin2, INPUT);

  // initiate pinMode for LED, buttons, and buzzers
  pinMode(LED,OUTPUT);
  pinMode(buttonR,INPUT);
  pinMode(Buzzer,OUTPUT); 

  attachInterrupt(digitalPinToInterrupt(3),buttonPressed1,RISING); 
}

void loop() { 
  
  // sense temperature
  tempSense(0);
  tempSense(1);
  tempSense(2);

  // calculate temperature running average
  tempAvgCalc(0);
  tempAvgCalc(1);
  tempAvgCalc(2);

  if(BP == 1){ // display default screen
   dispDefault();
   attachInterrupt(digitalPinToInterrupt(buttonG),buttonPressed1,RISING);
  }
  
  else if(BP == 2){ // display baby 1 temps
   disp2();
   attachInterrupt(digitalPinToInterrupt(buttonG),buttonPressed1,RISING);
  }

  else if(BP == 3){ // display baby 2 temps
   disp3();
   attachInterrupt(digitalPinToInterrupt(buttonG),buttonPressed1,RISING);
  }

  else if(BP == 4){ // display baby 3 temps
   disp4();
   attachInterrupt(digitalPinToInterrupt(buttonG),buttonPressed1,RISING);
  }

  if (snooze == false){
    alarmSys(); // activate alarm system
  }

  snoozeFn(); // turn off alarm
  
  delay(1000);
}

//////////////////////////////////////////////////////////////////////// FUNCTIONS ////////////////////////////////////////////////////////////////////////

void tempSense(int k){ // where k is the probe number
//  Serial.println(k);

  // smoothing analog input readings with running average //
  // subtract the last reading:
  total[k] = total[k] - analogReadings[k][readIndex[k]];

  // read from the temp sensor:
  analogReadings[k][readIndex[k]] = float(analogRead(analogPins[k]));
//  Serial.println(analogReadings[k][readIndex[k]]);

  // add the analog reading to running total:
  total[k] = total[k] + analogReadings[k][readIndex[k]];
//  Serial.print("Total ");
//  Serial.println(total[k]);

  // advance to the next position in the temp data storing array:
  readIndex[k] = readIndex[k] + 1;
//  Serial.print("Read Index ");
//  Serial.println(readIndex[k]);

  // if we're at the end of the array... loop back to beginning
  if (readIndex[k] == (numReadings-1)){
    readIndex[k] = 0;
  }

  average[k] = total[k] / float(numReadings); // take average of latest 10 analog readings
//  Serial.print(k);
//  Serial.println(" ANALOG: ");
//  Serial.println(average[k]);

  temp[k] = (m[k] * average[k]) + b[k]; // convert analog reading into temp in C
  Serial.print(k);
  Serial.println(" AVG TEMP ");
  Serial.println(temp[k]);

  // storing newest temp values at front of tempArray and shuffling older values to back
  for(int i = c; i >= 0; i--){
    if(average[k] > 40){
//      Serial.print("I am BIG");
      tempArray[k][i+1] = tempArray[k][i];
    }
  }

  tempArray[k][0] = temp[k]; // putting latest temp val at beginning of array

//  Serial.print("TEMP ARRAY BABY ");
//  Serial.println(k+1);
//  for (int i = 0; i <= 5; i++){
//    Serial.print(i);
//    Serial.print("     ");
//    Serial.println(tempArray[k][i]);
//  }

  c = c + 1; // counter for shuffling array values
//  Serial.println(c);

  if(c == 119){ // reset counter bc we only care about the most recent 120 values
    c = 0;
//    Serial.print("120 VALS BBY: ");
//    for (int i = 0; i <= 119; i++){
//      Serial.print(i);
//      Serial.print("     ");
//      Serial.println(tempArray[k][i]);
//    }
  }
}

void avgCalc(int k, int low, int high, int col){ // k = probe #, low = lower bound, high = upper bound, col = first, second, third, fourth interval avg
  sum[k][col] = 0;
  for(int i = low; i <= high; i++){
    sum[k][col] = sum[k][col] + tempArray[k][i-1];
  }
}

void tempAvgCalc(int k){
  // currently averaging temperatures every minute
  avgCalc(k, 1, 30, 0);
  avgCalc(k, 31, 60, 1);
  avgCalc(k, 61, 90, 2);
  avgCalc(k, 91, 120, 3);

//  Serial.print("Baby ");
//  Serial.println(k+1);

  averageT[k][0] = sum[k][0] / float(10);
//  Serial.print("Avg1: ");
//  Serial.println(averageT[k][0]);

  averageT[k][1] = sum[k][1] / float(10);
//  Serial.print("Avg2: ");
//  Serial.println(averageT[k][1]);

  averageT[k][2] = sum[k][2] / float(10);
//  Serial.print("Avg3: ");
//  Serial.println(averageT[k][2]);

  averageT[k][3] = sum[k][3] / float(10);
//  Serial.print("Avg4: ");
//  Serial.println(averageT[k][3]);
}

void buttonPressed1()           //ISR function excutes when push button at pinD3 is pressed
{                    
   ++BP;
   if(BP == 5){ // reset to default screen
    BP = 1;
   }
                                                                                         
   Serial.print("BUTTON PUSHED -- SCREEN: ");
   Serial.println(BP);
   detachInterrupt(digitalPinToInterrupt(buttonG)); 
}

void dispDefault(){ // default screen
    display.display();
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Live Temps (C)");

    display.setCursor(0,20);
    display.print("Baby 1: ");
    display.println(temp[0]);
    display.setCursor(0,30);
    display.print("Baby 2: ");
    display.println(temp[1]);
    display.setCursor(0,40);
    display.print("Baby 3: ");
    display.println(temp[2]);
    delay(1000);

//    Serial.println("Screen 1");
}

void disp2(){ // baby 1
    display.display();
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Baby 1 Avg T (C)");
    display.println("(past 2 hours)");

    for (int i = 0; i < 4; i++) {
      display.println(averageT[0][i]);
      Serial.println(averageT[0][i]);
    }

    Serial.println("Screen 2");
    delay(1000);
}

void disp3(){ // baby 2
    display.display();
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Baby 2 Avg T (C)");
    display.println("(past 2 hours)");

    for (int i = 0; i < 4; i++) {
      display.println(averageT[1][i]);
      Serial.println(averageT[1][i]);
    }

    Serial.println("Screen 3");
    delay(1000);
}

void disp4(){ // baby 3
    display.display();
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Baby 3 Avg T (C)");
    display.println("(past 2 hours)");

    for (int i = 0; i < 4; i++) {
      display.println(averageT[2][i]);
      Serial.println(averageT[2][i]);
    }

    Serial.println("Screen 4");
    delay(1000);
}

void alarmSys(){ // alarm system code
  // ring alarm if there is greater than a 0.7 difference between any of the baby temperatures and turn LED on  
  if(abs(temp[0] - temp[1]) >= 0.7 || abs(temp[0] - temp[2]) >= 0.7 || abs(temp[1] - temp[2]) >= 0.7) {
//    tone(Buzzer,1000);
    digitalWrite(LED,HIGH);
    
    if(abs(temp[0] - temp[1]) >= 0.7) {
      display.display();
      display.clearDisplay();
      display.setCursor(0,0);
      
      display.print("Baby 1 & 2 DEV"); 
      display.display();
    }
    else if(abs(temp[0] - temp[2]) >= 0.7) {
      display.display();
      display.clearDisplay();
      display.setCursor(0,10);
      
      display.print("Baby 1 & 3 DEV");
      display.display();
    }
    else if(abs(temp[1] - temp[2]) >= 0.7) {
      display.display();
      display.clearDisplay();
      display.setCursor(0,20);
      
      display.print("Baby 2 & 3 DEV");
      display.display();
    }   
  }

//  // CHANGE FOR DEMO
//  if((16 >= temp[0] || temp [0] >= 20) || (16 >= temp[1] || temp [1] >= 20) || (16 >= temp[2] || temp [2] >= 20)) {
////    tone(Buzzer,2000);
//    digitalWrite(LED,HIGH);
//
//    display.display();
//    display.clearDisplay();
//    
//    //how do we stop reading temperature at this point and restart if this is not the case
//    if(16 >= temp[0] || temp [0] >= 20) {
//      display.setCursor(0,0);
//      
//      display.print("Baby 1 OOR: ");
//      display.println(temp[0]);
//      display.display();
//
//      Serial.print("Baby 1 OOR: ");
//      Serial.println(temp[0]);
//    }
//    if(16 >= temp[1] || temp [1] >= 20) {
////      display.display();
////      display.clearDisplay();
//      display.setCursor(0,10);
//      
//      display.print("Baby 2 OOR: ");
//      display.println(temp[1]);
//      display.display();
//
//      Serial.print("Baby 2 OOR: ");
//      Serial.println(temp[1]);
//    } 
//    if(16 >= temp[2] || temp [2] >= 20) {
////      display.display();
////      display.clearDisplay();
//      display.setCursor(0,20);
//      
//      display.print("Baby 3 OOR: ");
//      display.println(temp[2]);
//      display.display();
//
//      Serial.print("Baby 3 OOR: ");
//      Serial.println(temp[2]);
//    }
//  }
}

void snoozeFn(){
  //turn off alarms by pressing buttonR
  if((digitalRead(buttonR) == HIGH)){ 
    snooze = true;
    alarmBP = 1; // alarm has been snoozed once / button has been pressed once 
    Serial.println("snooze 1");
  }

  while(alarmBP == 1){ // freeze on this screen until nurses make adjustments; once babies have been treated, then when the alarm system reactivates, the alarm shouldn't go off
    digitalWrite(LED,LOW);
    noTone(Buzzer);

    display.display();
    display.clearDisplay();
    display.setCursor(0,10);
    
    display.print("Alarm Deactivated");

    display.setCursor(0,20);
    display.print("Push button to reactivate.");
    display.display();
    
    Serial.println("ALARM SNOOZED; PUSH BUTTON AGAIN TO REACTIVATE");

    delay(4000);

    if(digitalRead(buttonR) == HIGH){ // reactivate alarm system
      alarmBP = 0;
      snooze = false; 
      Serial.print("snooze 2");
    }
  }
}
