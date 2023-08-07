/*
 * Note-Started using template Jan 2016 for compiler V 1.6.7
 * @TODO-always remember to update version history and programed compiled with below
 *
 * Circuit details below:
 *
 *
 */

//*****library inclusions below
// needed for platforIO, not needed if using arduino IDE
#include <Arduino.h>
// needed for radiohead library
#include <SPI.h>
// radiohead library. Others availabe. This one works with Semtech SX1276/77/78/79, Modtronix inAir4 and inAir9, and HopeRF RFM95/96/97/98 and other similar LoRa capable radios. (http://www.airspayce.com/mikem/arduino/RadioHead/index.html?utm_source=platformio&utm_medium=piohome)
#include <RH_RF95.h>

//*****global variables below
// general global variables
float firmwareV;       //@TODO-change to float
boolean debug = false; // enable verbose debug messages to serial
// pin to show recepit of message
#define LEDPIN 9

// for radio
// multiple instances possible but each instance must have its own interrupt and slave select pin. @TODO-check these pins are correct for moteino,
#define RFM95_CS 10 // chip select for SPI
#define RFM95_INT 2 // interupt pin
RH_RF95 rf95(RFM95_CS, RFM95_INT);
unsigned long nodeID = 100002; // up to 2 million for lorawan?
//@TODO-move other parameters up here for easy setting
float frequency = 904.0; // Specify the desired frequency in MHz
//@TODO-is this the right data type??
uint8_t spreadingFactor = 12;  // 6 to 12. A higher spreading factor increases the Signal to Noise Ratio (SNR), and thus sensitivity and range, but also increases the airtime of the packet. The number of chips per symbol is calculated as 2SF . For example, with an SF of 12 (SF12) 4096 chips/symbol are used. Each increase in SF halves the transmission rate and, hence, doubles transmission duration and ultimately energy consumption.
long signalBandwidth = 125000; // 7.8 to 500 kHz. typical values are 125000 ,250000 or 500000 Hz. Higher BW gives a higher data rate (thus shorter time on air), but a lower sensitivity.  Lower BW also requires more accurate crystals (less ppm).  Set bandwidth to the lowest (125 kHz). July 2023- 62500 Hz works with 1/2 dipole but unreliably. 41700 Hz does not work (tried 1/4 monopole, helical and 1/2 dipole for tx and 1/2 dipole for rx)

//****************functions- if declared before used, don't need Function Prototypes (.h) file? @TODO-is this preferred format for C++?

void fcn()
{
  // this is a description of this function
  // var1-defintion
}

void setup()
{
  firmwareV = 1.4;
  /*
   * Version history:
   * V1.0-initial
   * V1.1-added ability to not send ACKs to other ACKs and print signal infromation
   * V1.2-changed the way messages are sent to serial slightly
   * V1.3-changed message format
   * V1.4-added custom delimiter varaiable
   *
   * @TODO:
   * Only send ACK if ACK received from host computer
   * If not receiving ACKs from host computer, reboot it using a pin
   * change from a boolean debug variable to a log level byte variable
   * allow log level set from serial
   * allow frequency set from serial
   * allow set nodeID from serial
   * store nodeID in EEPROM
   * make it send firmware and nodeID in response to serial data
   */
  Serial.begin(9600);
  Serial.println();
  Serial.println();
  Serial.println(F("SETUP-Compiled with VScode and PlatformIO")); //@TODO-update this whenever compiling or get platformIO to do it automatically
  Serial.println(F("SETUP-and librarys:mikem/RadioHead, SPI.h"));
  Serial.print(F("SETUP-RFM95 receiver V"));
  Serial.println(firmwareV);
  Serial.println(F("SETUP-This program prints the lora messages received to the serial monitor."));

  // set pin(s) to output mode and do a test
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  delay(500);
  digitalWrite(LEDPIN, LOW);

  // start RFM95 radio
  if (!rf95.init())
  {
    Serial.println("ERROR-RFM95 initialization failed, can't do anything else! Reset me to try again.");
    digitalWrite(LEDPIN, HIGH);
    while (1)
      ;
  }

  // Set RFM95 transmission parameters
  // Note- some set functions do not return anything so don't need to check for sucess.
  // These 5 paramenter settings results in max range. The allowed values also come from this source (source:M. Bor and U. Roedig, “LoRa Transmission Parameter Selection,” in 2017 13th International Conference on Distributed Computing in Sensor Systems (DCOSS), Jun. 2017, pp. 27–34. doi: 10.1109/DCOSS.2017.10.)
  //  Set transmit power to the maximum (20 dBm), @TODO-check 2nd value. false is if PA_BOOST pin is used and true is RFO pin is used on RFM95 module
  rf95.setTxPower(20, false); //-4 to 20 in 1 dB steps
  // Set coding rate to the highest (4/8)
  rf95.setCodingRate4(8); // 5 to 8. offers protection against bursts of interference, A higher CR offers more protection, but increases time on air.
  // Set spreading factor to the highest (SF12)
  rf95.setSpreadingFactor(spreadingFactor);
  // Set bandwidth to the lowest (125 kHz). July 2023- 62500 Hz works with 1/2 dipole but unreliably. 41700 Hz does not work (tried 1/4 monopole, helical and 1/2 dipole for tx and 1/2 dipole for rx)
  rf95.setSignalBandwidth(signalBandwidth);
  // Set the desired frequency @TODO-how to set for optimal range?
  if (!rf95.setFrequency(frequency))
  {
    Serial.println("ERROR-Failed to set frequency!");
    digitalWrite(LEDPIN, HIGH);
    while (1)
      ;
  }

  //@TODO-consider:
  //rf95.setPromiscuous(true);
//rf95.setThisAddress(uint8_t thisAddress);

  Serial.println(F("INFO-RFM95 initialized."));
  if (debug)
  {
    Serial.println(F("DEBUG-All device registers:"));
    rf95.printRegisters();
    Serial.println();
  }
  Serial.print(F("INFO-Device version from register 42: "));
  Serial.println(rf95.getDeviceVersion());
  Serial.print(F("INFO-nodeID set to: "));
  Serial.println(nodeID);
  //@TODO-consider printing other things like maxMessageLength() (http://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html#ab273e242758e3cc2ed2679ef795a7196)

} // end setup fcn

void loop()
{

  if (rf95.available()) // note- do not need to delay because .available() returns true only if there is a new, complete, error free message
  {
    digitalWrite(LEDPIN, HIGH);

    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) // recv wants two pointers. You don't need to use &buf because C++ returns the address to the first element &len returns memory address of len
    {
      static unsigned long messageCounter = 1;

      const char delim = '\'';
      if (messageCounter == 1) // print header if this is the first message
      {
        //@TODO-get this to update with the delim
        // Serial.println(F("'S1'rssi'snr'frequencyError'bytes received'SpreadingFactor'SignalBandwidth'Frequency'M1'[message contents]'"));
        Serial.print(delim);
        Serial.print(F("N1"));
        Serial.print(delim);
        Serial.print(F("message counter (local to this device)"));
        Serial.print(delim);
        // extra space for another field later
        Serial.print(delim);
        // extra space for another field later
        Serial.print(delim);
        Serial.print(F("S1"));
        Serial.print(delim);
        Serial.print(F("rssi"));
        Serial.print(delim);
        Serial.print(F("snr"));
        Serial.print(delim);
        Serial.print(F("frequencyError"));
        Serial.print(delim);
        Serial.print(F("bytes received"));
        Serial.print(delim);
        Serial.print(F("SpreadingFactor"));
        Serial.print(delim);
        Serial.print(F("SignalBandwidth"));
        Serial.print(delim);
        Serial.print(F("Frequency"));
        Serial.print(delim);
        // extra space for another field later
        Serial.print(delim);
        // extra space for another field later
        Serial.print(delim);
        // extra space for another field later
        Serial.print(delim);
        // extra space for another field later
        Serial.print(delim);
        Serial.print(F("M1"));
        Serial.print(delim);
        Serial.print(F("[message contents]"));
        Serial.println(delim);
      }
      // print the signal info first since this will always be the same
      int16_t rssi = rf95.lastRssi();
      int snr = rf95.lastSNR();
      int freqError = rf95.frequencyError();

      //@TODO-consider adding these:
      //rf95.headerFlags();
      //rf95.headerFrom();
      //rf95.headerId();
      //rf95.headerTo();
      //rf95.rxBad();//may not be accurate
      //rf95.rxGood();
      Serial.print(delim);
      Serial.print(F("N1"));
      Serial.print(delim);
      Serial.print(messageCounter);
      Serial.print(delim);
      // extra space for another field later
      Serial.print(delim);
      // extra space for another field later
      Serial.print(delim);
      Serial.print(F("S1"));
      Serial.print(delim);
      Serial.print(rssi);
      Serial.print(delim);
      Serial.print(snr);
      Serial.print(delim);
      Serial.print(freqError);
      Serial.print(delim);
      Serial.print(len);
      Serial.print(delim);
      Serial.print(spreadingFactor);
      Serial.print(delim);
      Serial.print(signalBandwidth);
      Serial.print(delim);
      Serial.print(frequency);
      Serial.print(delim);
      // extra space for another field later
      Serial.print(delim);
      // extra space for another field later
      Serial.print(delim);
      // extra space for another field later
      Serial.print(delim);
      // extra space for another field later
      Serial.print(delim);
      buf[len] = '\0'; // Null-terminate the received message to treat it as a string
      Serial.print(F("M1"));
      Serial.print(delim);
      Serial.print((char *)buf); // casts this as a chracter array?? @TODO-check
      Serial.println(delim);

      // return ACK message, if needed
      //@TODO-add support for requesting an ack message or not
      //@TODO-add support to not print ACKs to serial
      static boolean ackReq = false;
      const char *ackMessage = "ACK";                                                   // Acknowledgment message, careful changing this as firmware update of all nodes will be needed. All receive nodes set to not send ACKs to this specific message
      boolean messageIsACK = strncmp((char *)buf, ackMessage, strlen(ackMessage)) == 0; // compaire the first characters of buffer to see if this is ACK message

      delay(100);                  //@TODO-add method to increase this if we keep getting the same message
      if (ackReq && !messageIsACK) // if ack requested and this is not an ACK message itself
      {
        // Send an acknowledgment back to the sender
        rf95.send((uint8_t *)ackMessage, strlen(ackMessage));
        rf95.waitPacketSent();
        if (debug)
        {
          Serial.println("INFO-Sent ACK message.");
        }
      }
      else
      {
        if (debug)
        {
          Serial.println("INFO-not sending ACK message.");
        }
      }

      messageCounter++;
      digitalWrite(LEDPIN, LOW);
    }
  }

  static unsigned long previousMillis = 0;
  // Get the current time
  unsigned long currentMillis = millis();
  if (debug && (currentMillis - previousMillis >= 300000))
  { // print a test message to serial, even if no transmission, 300000=5 min
    digitalWrite(LEDPIN, HIGH);
    // Reset the timer
    previousMillis = currentMillis;
    Serial.println(F("DEBUG-test serial message. Set to send at repeatable time interval."));
    delay(100);
    digitalWrite(LEDPIN, LOW);
  }

} // end loop fcn
