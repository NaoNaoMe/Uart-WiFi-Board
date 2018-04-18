Uart WiFi Board
=========

![Uart WiFi Board](mdContents/Uart WiFi Board_1.png)  
The Uart WiFi Board designed as the transparent WiFi-serial bridge.  
There are some updates from the former Uart WiFi Board.  
 - The 128x32 pixels SSD1306 OLED display
 - The 3 user switches(a switch used as power switch too)

Thanks to add the 128x32 pixels OLED display, we can show IP address, SSID and Serial BaudRate.  
You don't need remember IP address and Baud Rate, you can see them in the display.
The Uart WiFi Board can be programmed using the Arduino IDE.


### Some highlights of the Uart WiFi Board:
 1. Main core: ESP-WROOM-02(ESP8266).
 2. Voltage regulator(3.3V) with control pin.
 3. Opto-isolated UART interface(Uart interface circuit accepts 3.3V and 5.0V).
 4. On board WiFi-controller can use following functions.
 	- LED1(IO2), SW1(IO12/POW_ON), SW2(IO16) and SW3(IO13)
 	- Control voltage regulator's control pin connected to "IO14" H:Enable L:Disable(normally)
 	- Voltage detection circuit(for CN1) connected to "TOUT"
 5. Measures 50mm x 60mm x 20mm


#### Some highlights of the Sample software:
 1. Totally transparent WiFi-serial bridge.
 2. IP address, SSID, Serial baud-rate and input-voltage can be displayed.
 3. Low voltage detection.
 
Sample software use the [esp8266-oled-ssd1306](https://github.com/ThingPulse/esp8266-oled-ssd1306) library.


Repository Contents
-------------------
* **/Hardware** - All Eagle design files (.brd, .sch)
* **/Software** - Software files for this hardware


Description(Pin Interfaces)
-------------------
![TopView](mdContents/Uart WiFi Board_2.png)

### CN1(Primarily) Pin Descriptions:
This connector is the supply voltage.

| Pin | Name | Description                                                  |
| --- | ---- | ------------------------------------------------------------ |
| 1   | VBAT | Up to 12.0VDC convert to 3.3VDC                              |
| 2   | GND  | GND	                                                        |



### CN3(Primarily) Pin Descriptions:
This connector is for uploading software to WiFi-controller.  
To be selected BOOT MODE before supply voltage when you upload software.

| Pin | Name | Description                                                  |
| --- | ---- | ------------------------------------------------------------ |
| 1   | VCC  | 3.3VDC                                                       |
| 2   | TX   | UART TX	                                                    |
| 3   | RX   | UART RX	                                                    |
| 4   | GND  | GND	                                                        |
| 5   | BOOT | Connected to "IO0" H:Flash boot(normally) L:UART download    |



### CN2(Secondary) Pin Descriptions:
This connector is for your target board UART-interface.

| Pin | Name | Description                                                  |
| --- | ---- | ------------------------------------------------------------ |
| 1   | VIN  | 2.7VDC to 5.5VDC                                             |
| 2   | TX   | UART TX(Open collector output)	                            |
| 3   | RX   | UART RX	                                                    |
| 4   | GND  | GND	                                                        |



How to upload program
-------------------
See [this page](UploadProgram.md).


How to use the Uart WiFi Board
-------------------
An example is [here](Instructions.md).


License
-------------------
The repository is released under [Creative Commons ShareAlike 4.0 International](https://creativecommons.org/licenses/by-sa/4.0/).

Distributed as-is; no warranty is given.
