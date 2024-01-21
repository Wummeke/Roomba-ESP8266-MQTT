# Roomba-ESP8266-MQTT

Code, hints, tips, tricks and general documentation used as basis and lots of inspiration (in non particular order):
* https://github.com/thehookup/MQTT-Roomba-ESP01
* https://github.com/Mjrovai/Roomba_BT_Ctrl
* https://github.com/johnboiles/esp-roomba-mqtt
* https://www.bakke.online/index.php/2017/06/02/self-updating-ota-firmware-for-esp8266/
* https://forum.mysensors.org/post/52800
* http://anrg.usc.edu/ee579/spring2016/Roomba/iRobot_Roomba_600_Open_Interface_Spec.pdf

Uses Roomba library by Mike Macauley:
* http://www.airspayce.com/mikem/arduino/Roomba

# wiring:

```
Roomba  ESP
-----------
RX      TX
VCC     VCC
GND     GND
BRC     GPIO0
TX      RX
```
Note, I amplified  the TX signal from the Roomba with PNP transistor. See documentation in the Roomba library mentioned above and/or wiring scheme in the Hook Up's repository. This is different from John Boiles solution, who uses a voltage devider to lower the voltage of the TX signal. Not sure which approach is better, but using the transistor works fine for me.

# known issue:
My Roomba 650 won't wake up from sleep when docked. Tried pulsing BRC-pin low every 28 secs, but  Roomba resets itself after an hour or so. After that it won't wakeup unless I press the Clean button. However, it still responds with status information every so much minutes. 

If anyone has a sollution to this issue... Please let me know!
