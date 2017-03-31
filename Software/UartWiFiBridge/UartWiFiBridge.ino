//******************************************************************************
// UartWiFiBridge
// Note
// Totally transparent UART-WiFi bridge.
// This sketch adapts to UartWiFiBoard project.
//******************************************************************************

#include <ESP8266WiFi.h>

#define WIFI_STAT_LED_PIN 0
#define UART_STAT_LED_PIN 2
#define REG_CTRL_PIN 14

enum WiFiCondtion{
  WIFI_CONNECTION_WAITING,
  WIFI_CONNECTION_SUCCESS,
  WIFI_CONNECTION_FAILURE
};

#define LV_VOLTAGE_AD 409   // Low voltage ad value

#define BAUDRATE 9600   //Sets the baud rate for serial data.

const char versionName[] = "1.0.0";

const char* ssid = "YOUR-SSID";
const char* password = "YOUR-SSID-PASSWORD";

WiFiServer localServer(23); //Port number must be set
WiFiClient localClient;


WiFiCondtion connectToWiFi(){
  static unsigned long previousMillis = 0;
  static int timeoutCnt = 0;
  
  WiFiCondtion state = WIFI_CONNECTION_WAITING;
  
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) >= 100) {
    previousMillis = currentMillis;

    if( timeoutCnt == 200 ){
      state = WIFI_CONNECTION_FAILURE;
    }
    else if( timeoutCnt == 400 ){
      state = WIFI_CONNECTION_SUCCESS;
    }
    else if( timeoutCnt > 100 ){
      state = WIFI_CONNECTION_FAILURE;
      timeoutCnt = 200;
      Serial.print("Could not connect to ");
      Serial.println(ssid);
    }
    else{
      if( WiFi.status() == WL_CONNECTED ){
        state = WIFI_CONNECTION_SUCCESS;
        timeoutCnt = 400;
        
        localServer.begin();
        localServer.setNoDelay(true);
      
        Serial.print("Ready! Use 'Uart-WiFi Bridge ");
        Serial.print(WiFi.localIP());
        Serial.println(" to connect");
      }
      else{
        timeoutCnt++;
      }
    }
  }

  return state;
}

bool UartWiFiBridge( void ) {
  size_t bufLength = 0;
  bool ret = false;
  
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

  if(bufLength > 0){
    ret = true;
  }

  return ret;
}

void controlWiFiStatusLed(int blinkOnTimingCnt, int blinkPeriodCnt){
  static unsigned long previousMillis = 0;
  static int blinkCount = 0;

  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) >= 10) {
    previousMillis = currentMillis;

    //control blinking led
    blinkCount++;
    
    if( blinkCount > blinkPeriodCnt ) {
      blinkCount = 0;
      digitalWrite(WIFI_STAT_LED_PIN, LOW);
    }
    else if( blinkCount > blinkOnTimingCnt ) {
      digitalWrite(WIFI_STAT_LED_PIN, HIGH);
    }
    else {
      digitalWrite(WIFI_STAT_LED_PIN, LOW);
    }
  }
  
}

void controlUARTStatusLed(bool state){
  if(state){
    digitalWrite(UART_STAT_LED_PIN, LOW);
  }
  else{
    digitalWrite(UART_STAT_LED_PIN, HIGH);
  }
  
}

void detectLowVoltage(void){
  static unsigned long previousMillis = 0;
  static int adcStableCnt = 0;

  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) >= 10) {
    previousMillis = currentMillis;

    //check Battery voltage
    int value = analogRead(A0);
    if( value < LV_VOLTAGE_AD ) {
      adcStableCnt++;
    }
    else {
      adcStableCnt = 0;      
    }
  
    if( adcStableCnt > 100 ) {
      adcStableCnt = 100;
      digitalWrite(REG_CTRL_PIN, LOW);   // External voltage regulator will be stopped.
    }
  }

}

void setup() {
  digitalWrite(REG_CTRL_PIN, HIGH);   // Enable to external voltage regulator.
  digitalWrite(WIFI_STAT_LED_PIN, HIGH);
  digitalWrite(UART_STAT_LED_PIN, HIGH);

  pinMode(WIFI_STAT_LED_PIN, OUTPUT);
  pinMode(UART_STAT_LED_PIN, OUTPUT);
  pinMode(REG_CTRL_PIN, OUTPUT);
  
  Serial.begin(BAUDRATE);
  WiFi.begin(ssid, password);
  Serial.print("\nFirmware Version "); Serial.println(versionName);
  Serial.print("Connecting to "); Serial.println(ssid);

}

void loop() {
  static WiFiCondtion state = WIFI_CONNECTION_WAITING;
  
  bool ret = false;
  int blinkOnTimingCnt = 50;

  switch(state){
    case WIFI_CONNECTION_WAITING:
      state = connectToWiFi();
      
    break;

    case WIFI_CONNECTION_SUCCESS:
      ret = UartWiFiBridge();
      blinkOnTimingCnt = 100;
    break;

    default:
      blinkOnTimingCnt = 10;
    break;
    
  }

  controlWiFiStatusLed(blinkOnTimingCnt, 100);
  controlUARTStatusLed(ret);
  detectLowVoltage();
    
}
