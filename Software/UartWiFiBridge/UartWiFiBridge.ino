//******************************************************************************
// UartWiFiBridge
// Note
// Totally transparent UART-WiFi bridge.
// This sketch adapts to UartWiFiBoard project.
//******************************************************************************

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <EEPROM.h>
#include "oled.h"

#define POW_BUTTON_PIN 12   //SW1
#define LEFT_BUTTON_PIN 13  //SW3
#define RIGHT_BUTTON_PIN 16 //SW2

#define STATUS_LED_PIN 2  //LED1
#define REG_CTRL_PIN 14   //U1

#define SERIAL_ARRAY_SIZE 5

enum BoardCondition{
  CHECK_OLED_DEVICE = 0,
  BOARD_OPENING,
  WIFI_CONNECTION_WAITING,
  WIFI_CONNECTION_SUCCESS,
  BOARD_LV_DETECTED,
  BOARD_CLOSING
};

enum BoardProperty{
  SHOW_NOTHING = 0,
  SHOW_CONNECTED_SSID,
  SHOW_ASSIGNED_IP,
  SHOW_BAUDRATE,
  SHOW_BATTERY_VOLTAGE,
  SHOW_BLANK
};

enum SwitchState{
  SW_NOTHING = 0,
  SW_SHORT_PUSH,
  SW_LONG_PUSH
};

enum LvState{
  LV_NOTHING = 0,
  LV_DETECTED,
  LV_QUIT
};

#define RESISTOR_VALUE_R1  (float)10000  //10.0k ohm
#define RESISTOR_VALUE_R2  (float) 1000  // 1.0k ohm
#define IMPEDANCE_VALUE_AD (float)20000  //20.0k ohm

#define RESISTOR_VALUE_H    RESISTOR_VALUE_R1
#define RESISTOR_VALUE_L    (1/((1/RESISTOR_VALUE_R2) + (1/IMPEDANCE_VALUE_AD)))

#define BATT_GAIN_MAX       (float)100
#define BATT_GAIN_DEFAULT   (float)((RESISTOR_VALUE_L * 1024) / (RESISTOR_VALUE_H + RESISTOR_VALUE_L))
#define BATT_GAIN_MIN       (float)80

#define LV_DETECT_VOLTAGE     3.5f    // Low voltage value(3.5V)
#define LV_QUIT_VOLTAGE       3.3f    // Low voltage value(3.3V)

#define CALIBRATION_REF       5.0f    // Battery voltage value(5.0V)

//#define STATIC_IP_ADDR

const String SerialSpeeds[SERIAL_ARRAY_SIZE] = {
  "9600",
  "19200",
  "38400",
  "57600",
  "115200"
};

OLED OLEDModule;

const char VersionName[] = "UWB-Sample";
const char VersionNumber[] = "1.0.0";

const char* ssid = "TestSSID0123";
const char* password = "TestEncryptionKey0123";

#ifdef STATIC_IP_ADDR
IPAddress staticIP(192,168,0,25);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
#endif

WiFiServer localServer(23); //Port number must be set
WiFiClient localClient;

int serialSpeedIndex = 0;
float BatteryCalcGain = BATT_GAIN_DEFAULT;
int   BattryVoltAD  = 0;
float BattryVoltage = 0;

LvState detectLowVoltage(){
  static unsigned long previousMillis = 0;
  static int lvDetectCnt = 0;
  static int lvQuitCnt = 0;
  static LvState lowVoltageState = LV_NOTHING;
  
  unsigned long currentMillis = millis();

  if ((currentMillis - previousMillis) >= 100) {
    previousMillis = currentMillis;

    //check Battery voltage
    BattryVoltAD = analogRead(A0);
    BattryVoltage = (float)BattryVoltAD / BatteryCalcGain;
    
    if( BattryVoltage < LV_DETECT_VOLTAGE ) {
      lvDetectCnt++;
    }
    else {
      lvDetectCnt = 0;      
    }
  
    if( lvDetectCnt > 10 ) {
      lvDetectCnt = 10;
      lowVoltageState = LV_DETECTED;
    }
    
    if( BattryVoltage < LV_QUIT_VOLTAGE ) {
      lvQuitCnt++;
    }
    else {
      lvQuitCnt = 0;      
    }
  
    if( lvQuitCnt > 10 ) {
      lvQuitCnt = 10;
      lowVoltageState = LV_QUIT;
    }
  }
  
  return lowVoltageState;
}

bool IntervalTiming(int intervalCount){
  static unsigned long previousMillis = 0;

  bool timeupFlg = false;
  
  unsigned long currentMillis = millis();
  
  if ((currentMillis - previousMillis) >= intervalCount) {
    previousMillis = currentMillis;
    timeupFlg = true;

  }
  
  return timeupFlg;
}


SwitchState detectLeftButton(){
  static unsigned long previousMillis = 0;
  static SwitchState swState = SW_NOTHING;
  SwitchState retState = SW_NOTHING;
  unsigned long currentMillis = millis();
  
  int val = digitalRead(LEFT_BUTTON_PIN);
  if(val == LOW)
  {
    if ((currentMillis - previousMillis) > 1000) {
      if( swState != SW_LONG_PUSH ){
        swState = SW_LONG_PUSH;
        retState = SW_LONG_PUSH;
        
      }

    }
    else if ((currentMillis - previousMillis) > 100) {
      swState = SW_SHORT_PUSH;

    }
  }
  else
  {
    if(swState == SW_SHORT_PUSH) {
      retState = SW_SHORT_PUSH;
      swState = SW_NOTHING;
      
    }

    previousMillis = currentMillis;
    
  }

  return retState;
}


SwitchState detectRightButton(){
  static unsigned long previousMillis = 0;
  static SwitchState swState = SW_NOTHING;
  SwitchState retState = SW_NOTHING;
  unsigned long currentMillis = millis();
  
  int val = digitalRead(RIGHT_BUTTON_PIN);
  if(val == LOW)
  {
    if ((currentMillis - previousMillis) > 1000) {
      if( swState != SW_LONG_PUSH ){
        swState = SW_LONG_PUSH;
        retState = SW_LONG_PUSH;
        
      }

    }
    else if ((currentMillis - previousMillis) > 100) {
      swState = SW_SHORT_PUSH;

    }
  }
  else
  {
    if(swState == SW_SHORT_PUSH) {
      retState = SW_SHORT_PUSH;
      swState = SW_NOTHING;
      
    }

    previousMillis = currentMillis;
    
  }

  return retState;
}

bool detectPowButton(){
  static unsigned long previousMillis = 0;
  bool retFlg = false;
  unsigned long currentMillis = millis();
  
  int val = digitalRead(POW_BUTTON_PIN);
  if(val == LOW)
  {
    if (currentMillis - previousMillis > 3000) {
      previousMillis = currentMillis;
      retFlg = true;
    }
  }
  else
  {
    previousMillis = currentMillis;
    
  }

  return retFlg;
}

String appendString(String stringOne, String stringTwo ) {
  String retString;
  retString = stringOne.concat(stringTwo);
  return stringOne;
}

void dataExchange() {
  static unsigned long previousMillis = 0;
  
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) >= 50) {
    previousMillis = currentMillis;
    size_t bufLength = 0;
    
    //check if there are any new clients
    if (localServer.hasClient()){
      if (!localClient.connected()){
        if(localClient){
          localClient.stop();
        }
        localClient = localServer.available();
      }
    }
    
    //check a client to bridge UART
    if (localClient && localClient.connected()){
      if(localClient.available()){
        bufLength = localClient.available();
        uint8_t sbuf[bufLength];
        localClient.readBytes(sbuf, bufLength);
        Serial.write(sbuf, bufLength);
      }
      digitalWrite(STATUS_LED_PIN, LOW);
    }
    else{
      digitalWrite(STATUS_LED_PIN, HIGH);
      
    }

    //check UART to bridge WiFi
    if(Serial.available()){
      bufLength = Serial.available();
      uint8_t sbuf[bufLength];
      Serial.readBytes(sbuf, bufLength);
      if (localClient && localClient.connected()){
        localClient.write(sbuf, bufLength);
      }
    }
  }
}

bool ScanI2CDevices(byte address) {
  bool retflg =false;
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  if (error == 0) {
    retflg = true;
    
  }
  return retflg;
}

float ReadCalibration(float input) {
  float output = input;
  float gain = 0;
  byte tmp = 0;
  uint32_t value = 0;
  
  tmp = EEPROM.read(3);
  value |= (uint32_t)tmp;
  value = (value << 8) & 0xFFFFFF00;
  tmp = EEPROM.read(2);
  value |= (uint32_t)tmp;
  value = (value << 8) & 0xFFFFFF00;
  tmp = EEPROM.read(1);
  value |= (uint32_t)tmp;
  value = (value << 8) & 0xFFFFFF00;
  tmp = EEPROM.read(0);
  value |= (uint32_t)tmp;

  if(value != 0xFFFFFFFF) {
    gain = *(float *)&value;
    if( (gain <  BATT_GAIN_MAX) &&
        (gain >= BATT_GAIN_MIN) ) {
      output = gain;
    }
  }
  
  return output;
}

float WriteCalibration(float input) {
  float output = input;
  byte tmp = 0;
  uint32_t value = 0;

  if( (input >= BATT_GAIN_MAX) ||
      (input <  BATT_GAIN_MIN) ) {
    return output;
  }
  
  value = *(uint32_t *)&input;
  tmp = (byte)(value & 0x000000FF); 
  value = value >> 8;
  EEPROM.write(0, tmp);
  tmp = (byte)(value & 0x000000FF); 
  value = value >> 8;
  EEPROM.write(1, tmp);
  tmp = (byte)(value & 0x000000FF); 
  value = value >> 8;
  EEPROM.write(2, tmp);
  tmp = (byte)(value & 0x000000FF); 
  EEPROM.write(3, tmp);

  EEPROM.commit();
  
  return output;
}

void setup() {
  digitalWrite(REG_CTRL_PIN, HIGH);   // Enable to external voltage regulator.
  digitalWrite(STATUS_LED_PIN, HIGH);

  pinMode(REG_CTRL_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  
  pinMode(POW_BUTTON_PIN, INPUT);
  pinMode(LEFT_BUTTON_PIN, INPUT);
  pinMode(RIGHT_BUTTON_PIN, INPUT);

  Wire.begin();

  Serial.begin(SerialSpeeds[serialSpeedIndex].toInt());
  
  WiFi.begin(ssid, password);
#ifdef STATIC_IP_ADDR
  WiFi.config(staticIP, gateway, subnet);
#endif

  EEPROM.begin(4);

  BatteryCalcGain = ReadCalibration(BatteryCalcGain);

  IntervalTiming(0);

}

void loop() {
  static BoardCondition wifiState = CHECK_OLED_DEVICE;
  String firstColumn;
  String secondColumn;
  
  switch(wifiState){
    case CHECK_OLED_DEVICE:
      if(IntervalTiming(100)) {
        if(ScanI2CDevices(0x3c)) {
          wifiState = BOARD_OPENING;
          OLEDModule.initialize();
          IntervalTiming(0);
          //Serial.println("OLED device found");
        }
        else {
          //Serial.println("There are no OLED devices");          
        }
      }

    break;
    
    case BOARD_OPENING:
      firstColumn = String(VersionName);
      secondColumn = String(VersionNumber);
      OLEDModule.drawDisplayWithScrolling(firstColumn, secondColumn, false);

      if(IntervalTiming(2000)){
        wifiState = WIFI_CONNECTION_WAITING;
      }
      
    break;

    case WIFI_CONNECTION_WAITING:
      firstColumn = String(ssid);
      secondColumn = "Connecting";
      OLEDModule.drawDisplayWithScrolling(firstColumn, secondColumn, false);
      
      if(WiFi.status() == WL_CONNECTED){
        wifiState = WIFI_CONNECTION_SUCCESS;
        
        localServer.begin();
        localServer.setNoDelay(true);
        
        IntervalTiming(0);
        
      }
      
    break;

    case WIFI_CONNECTION_SUCCESS:

      displaySequence();
      
      if(WiFi.status() == WL_CONNECTED){
        dataExchange();
      }
      else{
        wifiState = WIFI_CONNECTION_WAITING;
      }

      {
        LvState tmpState = detectLowVoltage();
        if(tmpState == LV_DETECTED){
          wifiState = BOARD_LV_DETECTED;
          WiFi.disconnect();
          Serial.end();
        }
        else if(tmpState == LV_QUIT){
          wifiState = BOARD_CLOSING;
        }
      }
	  
    break;

    case BOARD_LV_DETECTED:
      firstColumn = "LV Detected";
      char buff[10];
      dtostrf(BattryVoltage, 4, 2, buff);  //4 is mininum width, 2 is precision
      secondColumn = appendString(String(buff),String("V"));
      OLEDModule.drawDisplayCenterAligned(firstColumn, secondColumn );
      
      if(detectLowVoltage() == LV_QUIT){
        wifiState = BOARD_CLOSING;
      }
    break;
    
    case BOARD_CLOSING:
      firstColumn = "POWER";
      secondColumn = "OFF";
      OLEDModule.drawDisplayCenterAligned(firstColumn, secondColumn);
      WiFi.disconnect();
      
      if(IntervalTiming(2000)){
        digitalWrite(REG_CTRL_PIN, LOW);   // External voltage regulator will be stopped.
      }
      
    break;

    default:
      
    break;
    
  }

  if(detectPowButton()){
    wifiState = BOARD_CLOSING;
  }
  
}

void displaySequence(void){
  static BoardProperty currentProperty = SHOW_CONNECTED_SSID;
  static BoardProperty previousProperty = SHOW_CONNECTED_SSID;
  static bool blinkFlg = false;
  String firstColumn;
  String secondColumn;

  switch(currentProperty){
    case SHOW_CONNECTED_SSID:
      firstColumn = String(ssid);
      secondColumn = "Connected";
      OLEDModule.drawDisplayWithScrolling(firstColumn, secondColumn, false);

      if(IntervalTiming(5000)){
        previousProperty = currentProperty;
        currentProperty = SHOW_BLANK;
      }

      if( detectLeftButton() == SW_SHORT_PUSH){
        currentProperty = SHOW_ASSIGNED_IP;
      }
      
    break;

    case SHOW_ASSIGNED_IP:
      firstColumn = WiFi.localIP().toString();
      secondColumn = "Assigned";
      OLEDModule.drawDisplayWithScrolling(firstColumn, secondColumn, false);
      
      if(IntervalTiming(5000)){
        previousProperty = currentProperty;
        currentProperty = SHOW_BLANK;
      }

      if( detectLeftButton() == SW_SHORT_PUSH){
        currentProperty = SHOW_BAUDRATE;
        IntervalTiming(0);
        blinkFlg = false;
      }
      
    break;
      
    case SHOW_BAUDRATE:
      firstColumn = appendString(SerialSpeeds[serialSpeedIndex],String("bps"));
      secondColumn = "Serial Data rate";

      if( blinkFlg == false ){
        OLEDModule.drawDisplayWithScrolling(firstColumn, secondColumn, false);

        SwitchState tmpState = detectLeftButton();
        detectRightButton();  //Dummy call
        
        if( tmpState == SW_LONG_PUSH){
          blinkFlg = true;
          IntervalTiming(0);
        }
        
        if( tmpState == SW_SHORT_PUSH){
          currentProperty = SHOW_BATTERY_VOLTAGE;
          IntervalTiming(0);
        }
        
      }
      else {
        OLEDModule.drawDisplayWithScrolling(firstColumn, secondColumn, true);
        
        if( detectRightButton() == SW_SHORT_PUSH){
          serialSpeedIndex++;
          if( serialSpeedIndex >= SERIAL_ARRAY_SIZE){
            serialSpeedIndex = 0;
          }
          IntervalTiming(0);
        }
        
        if( detectLeftButton() == SW_LONG_PUSH){
          blinkFlg = false;
          IntervalTiming(0);

          Serial.end();
          delay(200);
          Serial.begin(SerialSpeeds[serialSpeedIndex].toInt());
          
        }
        
      }

      if(IntervalTiming(5000)){
        previousProperty = currentProperty;
        currentProperty = SHOW_BLANK;
        blinkFlg = false;
      }

    break;

    case SHOW_BATTERY_VOLTAGE:
      firstColumn = "BATT Volt";
      char buff[10];
      dtostrf(BattryVoltage, 4, 2, buff);  //4 is mininum width, 2 is precision
      secondColumn = appendString(String(buff),String("V"));
      OLEDModule.drawDisplayCenterAligned(firstColumn, secondColumn );
      
      if(IntervalTiming(5000)){
        previousProperty = currentProperty;
        currentProperty = SHOW_BLANK;
      }

      if( detectLeftButton() == SW_SHORT_PUSH){
        currentProperty = SHOW_CONNECTED_SSID;
        IntervalTiming(0);
      }
      
      if( detectRightButton() == SW_LONG_PUSH){
        //VBAT(CN1) Voltage Calibration
        BatteryCalcGain = (float)BattryVoltAD / CALIBRATION_REF;
        BatteryCalcGain = WriteCalibration(BatteryCalcGain);
        IntervalTiming(0);
      }
      
    break;

    case SHOW_BLANK:
      OLEDModule.clear();
      currentProperty = SHOW_NOTHING;
    break;

    default:
      if( detectLeftButton() == SW_SHORT_PUSH){
        currentProperty = previousProperty;
        IntervalTiming(0);
      }
      
    break;
    
  }

}

