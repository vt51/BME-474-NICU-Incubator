// record analog readings from temperature probes at certain time intervals, synced with LabVIEW's thermometer

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

int readIndex[3] = {0, 0, 0}; // index of the current reading
float total[3] = {0.0, 0.0, 0.0}; // running total
float average[3] = {0.0, 0.0, 0.0}; // average of analog readings

void setup() {
  Serial.begin(9600);

  // initiating analog pins for temp probes
  pinMode(analogPin0, INPUT);  
  pinMode(analogPin1, INPUT);
  pinMode(analogPin2, INPUT);
}

void loop() {
  // sense temperature
  analogSense(0);
  analogSense(1);
  analogSense(2);
  
  Serial.println();
  delay(10000);
}

void analogSense(int k){ // where k is the probe number
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

  String color = "";

  if (k == 0){
    color = "B";
  }
  else if (k == 1){
    color = "R";
  }
  else{
    color = "O";
  }
  

  Serial.print(average[k]); 
  Serial.print(",");
}
