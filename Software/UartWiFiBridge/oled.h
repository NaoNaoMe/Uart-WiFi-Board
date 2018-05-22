#ifndef oled_h
#define oled_h
#include <ESP8266WiFi.h>

class OLED {
  public:
  	void initialize();
  	void clear();
		void drawDisplayWithScrolling( String firstColumn, String secondColumn, bool blinkFirstColumnFlg );
		void drawDisplayCenterAligned( String firstColumn, String secondColumn );
  private:
};

#endif
