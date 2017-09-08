# NodeVent
Control ventilation using a NodeMCU

Todo: HW schema, code cleanup

In short: 12V input. A NodeMCU ESP8266 module is powered by a 3.3V regulator, and a MCP4822 DAC takes 10V input through a voltage divider.
Output from the DAC is 0-10V to control the speed of the HVAC fan. The fan has a tachymeter which gives sinking pulses for the NodeMCU to count the RPM.
Communicating with Openhab using MQTT.
