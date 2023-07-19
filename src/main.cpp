/*
 * Note-Started using template Jan 2016 for compiler V 1.6.7
 * @TODO-always remember to update version history and programed compiled with below
 * 
 * Circuit details below:
 * 
 * 
 */

//*****library inclusions below
//needed for platforIO, not needed if using arduino IDE
#include <Arduino.h>
//needed for radiohead library
#include <SPI.h>
//radiohead library. Others availabe. This one works with Semtech SX1276/77/78/79, Modtronix inAir4 and inAir9, and HopeRF RFM95/96/97/98 and other similar LoRa capable radios. (http://www.airspayce.com/mikem/arduino/RadioHead/index.html?utm_source=platformio&utm_medium=piohome)
#include <RH_RF95.h>


//*****global variables below
//general global variables
boolean serialON=true;//print messages to serial, not needed if running on own
float firmwareV; //@TODO-change to float
boolean debug = true; //enable verbose debug messages to serial

//for radio
//multiple instances possible but each instance must have its own interrupt and slave select pin. @TODO-check these pins are correct for moteino,  
#define RFM95_CS 10 //chip select for SPI
#define RFM95_INT 2 //interupt pin
RH_RF95 rf95(RFM95_CS, RFM95_INT);
unsigned long nodeID = 3; //up to 2 million for lorawan?


//******functions- if declared before used, don't need Function Prototypes (.h) file? @TODO-is this preferred format for C++?
long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  //https://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

void fcn(){
  //this is a description of this function
  //var1-defintion
  
}


void setup(){
  Serial.begin(9600);
  Serial.println();
  Serial.println();
  Serial.println(F("Compiled with VScode and PlatformIO")); //@TODO-update this whenever compiling or get platformIO to do it automatically
  Serial.println(F("and librarys:mikem/RadioHead@^1.120, SPI.h"));
  Serial.println(F("RFM9595TEST-SEND V 1.0"));
  Serial.println(F("This program sends dummy packets to test the transmission ability of the REM95 module."));
  if(!serialON){
    Serial.end();
  }
  /*
   * Version history:
   * V1.0-initial
   * 
   */
  firmwareV = 1.0;
  

//start RFM95 radio
  if (!rf95.init()) {
    if(serialON){
    Serial.println("RFM95 initialization failed");
    }
    while (1);
  }

//Set RFM95 transmission parameters
//Note- some set functions do not return anything so don't need to check for sucess.
  //These 5 paramenter settings results in max range. The allowed values also come from this source (source:M. Bor and U. Roedig, “LoRa Transmission Parameter Selection,” in 2017 13th International Conference on Distributed Computing in Sensor Systems (DCOSS), Jun. 2017, pp. 27–34. doi: 10.1109/DCOSS.2017.10.)
  // Set transmit power to the maximum (20 dBm), @TODO-check 2nd value. false is if PA_BOOST pin is used and true is RFO pin is used on RFM95 module
  rf95.setTxPower(20, false);//-4 to 20 in 1 dB steps
  // Set spreading factor to the highest (SF12)
  rf95.setSpreadingFactor(12);//6 to 12. A higher spreading factor increases the Signal to Noise Ratio (SNR), and thus sensitivity and range, but also increases the airtime of the packet. The number of chips per symbol is calculated as 2SF . For example, with an SF of 12 (SF12) 4096 chips/symbol are used. Each increase in SF halves the transmission rate and, hence, doubles transmission duration and ultimately energy consumption.
  // Set bandwidth to the lowest (125 kHz)
  rf95.setSignalBandwidth(125000); //7.8 to 500 kHz. typical 125,250 or 500 kHz. Higher BW gives a higher data rate (thus shorter time on air), but a lower sensitivity.  Lower BW also requires more accurate crystals (less ppm).
  // Set coding rate to the highest (4/8)
  rf95.setCodingRate4(8);//5 to 8. offers protection against bursts of interference, A higher CR offers more protection, but increases time on air.
// Set the desired frequency @TODO-how to set for optimal range?
  float frequency = 915.0; // Specify the desired frequency in MHz
  if (!rf95.setFrequency(frequency)) {
    Serial.println("Failed to set frequency!");
    while (1);
  }

  if(serialON){
  Serial.println(F("RFM95 initialized."));
  if(debug){
    Serial.println(F("All device registers:"));
    rf95.printRegisters();
    Serial.println();
  }
  Serial.print(F("Device version from register 42:"));
  Serial.println(rf95.getDeviceVersion());
  Serial.println(F("nodeID set to:"));
  Serial.println(nodeID);
  //@TODO-consider printing other things like maxMessageLength() (http://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html#ab273e242758e3cc2ed2679ef795a7196)
  }
  
}//end setup fcn

void loop(){
  static unsigned long counter = 1;

  long supplyV = readVcc();
  char message[47]; //@TODO-make this shorter to save memory. unsigned long:4,294,967,295. long: -2,147,483,648 (approximate assume all fields are 11 chars and one for comma: 11+1+11+1+11+1+11=47)
  //@TODO-get this to dynamically update the firmware version number
  snprintf(message, sizeof(message)/sizeof(char), "1.0,%lu,%ld,%lu",nodeID,supplyV,counter); //snprintf should add the \0 at the end

  if(serialON){
  Serial.println(F("Sending data:"));
  Serial.println(message);
  }

  rf95.send((uint8_t*)message, strlen(message));
  rf95.waitPacketSent();

if(serialON){
  Serial.println("Packet sent");
  }

  delay(100);
  counter++;
 
}//end loop fcn