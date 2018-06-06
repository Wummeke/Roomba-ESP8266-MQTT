# Roomba-ESP8266-MQTT

Code, hints, tips and tricks used as basis and lots of inspiration (in non particular order):
* https://github.com/thehookup/MQTT-Roomba-ESP01
* https://github.com/Mjrovai/Roomba_BT_Ctrl
* https://github.com/johnboiles/esp-roomba-mqtt
* https://www.bakke.online/index.php/2017/06/02/self-updating-ota-firmware-for-esp8266/
* https://forum.mysensors.org/post/52800

Uses Roomba library by Mike Macauley:
* http://www.airspayce.com/mikem/arduino/Roomba

# known issue:
My Roomba 650 won't wake up from sleep when docked. Tried pulsing BRC-pin low every 28 secs, but  Roomba resets itself after an hour or so. After that it won't wakeup unless I press the Clean button. However, it still responds with status information every so much minutes. 

If anyone has a sollution to this issue... Please let me know!
