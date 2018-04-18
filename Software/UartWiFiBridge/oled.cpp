#include "oled.h"
#include "SSD1306.h"

SSD1306  OLEDDisplay(0x3c, 4, 5);

void OLED::initialize(){
  OLEDDisplay.init();
  //OLEDDisplay.flipScreenVertically();
  OLEDDisplay.setFont(ArialMT_Plain_24);

}

void OLED::clear(){
  // clear the display
  OLEDDisplay.clear();
  // write the buffer to the display
  OLEDDisplay.display();  
}

void OLED::drawDisplayWithScrolling( String firstColumn, String secondColumn, bool blinkFirstColumnFlg ) {
  static unsigned long timeSinceLastDraw = 0;
  static int firstXPosiSum = 0;
  static int secondXPosiSum = 0;
  static int seqNum = 0;
  
  unsigned long currentMillis = millis();
  if ((currentMillis - timeSinceLastDraw) < 200) {
    return;

  }

  if( blinkFirstColumnFlg == true ) {
    seqNum++;
    if( seqNum >= 2 ) {
      seqNum = 0;
      firstColumn = "";
    }
  }

  int firstTotalPixel = OLEDDisplay.getStringWidth(firstColumn);
  int secondTotalPixel = OLEDDisplay.getStringWidth(secondColumn);
  int firstXPosition = 10;
  int secondXPosition = 10;
  
  timeSinceLastDraw = currentMillis;

  // clear the display
  OLEDDisplay.clear();
  
  if(firstTotalPixel > 110){
    firstXPosition -= firstXPosiSum;
    firstTotalPixel -= firstXPosiSum;

    if(firstTotalPixel < 32 ){
      firstXPosiSum = 0;
      firstXPosition = 10;
    }
  }

  if(secondTotalPixel > 110){
    secondXPosition -= secondXPosiSum;
    secondTotalPixel -= secondXPosiSum;

    if(secondTotalPixel < 32 ){
      secondXPosiSum = 0;
      secondXPosition = 10;
    }
  }

  OLEDDisplay.setFont(ArialMT_Plain_24);
  OLEDDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
  OLEDDisplay.drawString(firstXPosition, 0, firstColumn);
  OLEDDisplay.drawString(secondXPosition, 32, secondColumn);
  
  firstXPosiSum += 5;
  secondXPosiSum += 5;

  // write the buffer to the display
  OLEDDisplay.display();
  
}


void OLED::drawDisplayCenterAligned( String firstColumn, String secondColumn ) {
  static unsigned long timeSinceLastDraw = 0;

  unsigned long currentMillis = millis();
  if ((currentMillis - timeSinceLastDraw) < 200) {
    return;

  }

  timeSinceLastDraw = currentMillis;

  // clear the display
  OLEDDisplay.clear();
  
  OLEDDisplay.setFont(ArialMT_Plain_24);
  OLEDDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
  OLEDDisplay.drawString(64, 0, firstColumn);
  OLEDDisplay.drawString(64, 32, secondColumn);
  
  // write the buffer to the display
  OLEDDisplay.display();
  
}  
