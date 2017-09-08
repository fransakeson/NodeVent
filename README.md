# NodeVent
Control ventilation using a NodeMCU

Todo:
- HW schema
- Code cleanup
- Hard code less, make more configurable

In short: 12V input. A NodeMCU ESP8266 module is powered by a 3.3V regulator, and a MCP4822 DAC takes 10V input through a voltage divider.
Output from the DAC is 0-10V to control the speed of the HVAC fan. The fan has a tachymeter which gives sinking pulses for the NodeMCU to count the RPM.  
Communicating with Openhab using MQTT.

The fan is a Systemair box fan placed in the attic of my house. To be clear, it is not driven by the 10V, it has an input which takes 0-10V to control the speed. Normally, a potentiometer would be placed between a 10V output and the speed control input pin.
