// Reads data from Current Cost CC128 Power Meter and feeds it to Arduino Mega which collates with other sensors 
// and uploads to Xively and Raspberry Pi which in turn logs to local SQL.
// http://www.dangertech.org

#include <SoftwareSerial.h>
#define rxPin 3
#define txPin 300 // again a non existing pin, compiler is OK with this...
SoftwareSerial currentcost(rxPin, txPin);

String readString, temp, sensor, watts;
const unsigned int MAX_INPUT = 164; //Serial Buffer

int wattsInt;
long int totalPower = 0;
long int totalHydro = 0;
int sensorInt;
int LP;
static char input_line [MAX_INPUT];
static unsigned int input_pos = 0;
int hydroCount = 0; //hydro counter
int totalCount = 0; //power counter
int lastHydro = 0; //Last Hydro result
int lastTotal = 0; //Last total power result

void setup() {
  Serial.begin(9600);
  currentcost.begin(57600);
}

void loop() {

  static char input_line [MAX_INPUT];
  static unsigned int input_pos = 0;

  if (currentcost.available()) { //Software serial have data?
    //  Serial.print((char)Serial1.read()); //debug to print line from currentcost
    {
      char inByte = currentcost.read (); //read  waiting serial char to inByte
      switch (inByte) {
      case '\n':   // end of text
        input_line [input_pos] = 0;  // terminating null byte
        // terminator reached! process input_line here ...
        process_data (input_line);
        // reset buffer for next time
        input_pos = 0;  
        break;

      case '\r':   // discard carriage return
        break;

      default:
        // keep adding if not full ... allow for terminating null byte
        if (input_pos < (MAX_INPUT - 1))
          input_line [input_pos++] = inByte;
        //     Serial.print(inByte);
        break;

      }  // end of switch

    }  // end of incoming data

  }
  //********************Send Recieve code**************************

  if(Serial.available()) {
    char ch = Serial.read();
    switch(ch) {
    case 'S': //Receive 'S' from Mega? Average Pending Results, transmit and reset counts.
      Serial.println("Received Send Request");
      totalPower = totalPower / totalCount;
      if(totalPower < 1) { //Stops rogue reading of negative or 0 values due to my Solar Exporting to Grid
        totalPower = 0;
      }
      totalHydro = totalHydro / hydroCount;
      if(totalHydro < 0) {
        totalHydro = 0;
      }
      LP = totalPower - totalHydro; // =Lights and Power usage. Can have weird values due to Solar Generation. Currentcost does not give - value when generating power
     
     //Sending data to be read by Mega
      Serial.println("Sending Requested Data...."); 
      Serial.print(totalPower,DEC); 
      Serial.print(",");  
      Serial.print(totalHydro,DEC); 
      Serial.print(",");
      Serial.print(LP,DEC);
      Serial.print(",");
      Serial.println("F"); //Finish marker
      
      //Reset Counts
      ch = 0;
      totalCount = 0;
      totalPower = 0;
      totalHydro = 0;
      hydroCount = 0;
      Serial.flush();
      break;
      
    default: //Any other char
      Serial.print(ch);  
      Serial.println(" was recieved....WTF do I do with this?! ");
      Serial.flush(); //Empty Serial buffer
      break;
    }
  }
}

void process_data (char * data)
{
  //Serial.println (data);
  readString = data; //makes the string readString
  temp = readString.substring(70, 74); //get the first four characters
  sensor = readString.substring(89, 90); //get the next four characters 
  watts = readString.substring(139, 144);
  wattsInt = watts.toInt();  //convert readString into a number
  sensorInt = sensor.toInt();

  if(sensorInt == 0) {
    lastTotal = wattsInt;
    if(lastTotal > 10) { // Value should never be below 10. Ignore if this is the case
      totalPower = totalPower + lastTotal;
      totalCount ++;
      //    Serial.print("Last POwer:");Serial.println (lastTotal);
      //    Serial.print("Total Count:");Serial.println (totalCount);
      //    Serial.print("Total Power:");Serial.println (totalPower);
    }
  }
  else if(sensorInt = 1) {
    lastHydro = wattsInt;
    if(lastHydro >= 0) { //Value can be 0
      totalHydro = totalHydro + lastHydro;
      hydroCount ++;
      //        Serial.print("Last hydro:");Serial.println (lastHydro);
      //        Serial.print("Hydro Count:");Serial.println (hydroCount);
      //        Serial.print("Hydro amount:");Serial.println (totalHydro);
    }
  }
}
