MQTT
====

A Wrapper around mqtt for Arduino to be used with esp8266 modules.

It wraps a slightly modified version of mqtt for esp8266 ported by Tuan PM.
Original code for esp: https://github.com/tuanpmt/esp_mqtt
Original code for contiki: https://github.com/esar/contiki-mqtt


====

**warning:**

Subscribe supports qos=0 and qos=1

Avoid Serial.setDebugOutput(true)!

It is currently not recommended to do blocking IO operations (network, serial, file) from callback functions. Instead, set a flag inside the callback and check for that flag inside the loop function.

Don't use other libraries that call system_os_task with priority = 1!!!

**secure libssl:**

If you want to use secure communication, please use the `secure`-branch!
