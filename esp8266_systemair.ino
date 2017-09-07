 /*
 ESP8266 MQTT
 DAC output
 Pulse counter input
 Non-blocking reconnect

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MCP48x2.h>

// Update these with values suitable for your network.
const char* ssid = ""; // SSID of your wifi
const char* password = ""; // Password for your wifi
const char* mqtt_server = ""; // IP of the MQTT server

#define reportTime 5000 // Set milliseconds between mqtt reports
#define CSPIN D3 // Define the SPI chip select pin
#define isrPin D1 // Define interrupt pin for RPM counter
long dacDefault = 37; // Set default DAC output to 37% 
long MAX_VALUE = 2062; // Fine tune max output from DAC depending on the exact load and supply

// Variables for counting RPM
volatile unsigned long firstPulseTime;
volatile unsigned long lastPulseTime;
volatile unsigned long numPulses;

// Create wifi and mqtt objects
WiFiClient espClient;
PubSubClient client(espClient);

byte channelA = CHANNEL_A | GAIN_2 | MODE_ACTIVE; //define config byte for channel A with gain2 and active
MCP48x2 dac(MCP4822, CSPIN); //create the DAC object, this example uses the MCP4822 12 bit DAC

long lastReconnectAttempt = 0;
long timeBefore = 0;
char msg[50];

float payloadVal = dacDefault; // Just to send default value at startup
String payloadStr;

void setup() {
  pinMode(isrPin, INPUT); // Initialize isrPin as input
  pinMode(BUILTIN_LED, OUTPUT); // Initialize the BUILTIN_LED pin as an output

  //start the SPI
  SPI.begin();
  SPI.setFrequency(10000000); 
  dac.send(CHANNEL_B | MODE_SHUTDOWN, 0); // Shutdown channel B
  dac.send(channelA, (int) (MAX_VALUE/100.0)*dacDefault); //Set the output on channel A to default value

  // Setup serial, wifi and mqtt
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  lastReconnectAttempt = 0;
}

void setup_wifi() {

  // We start by connecting to a WiFi network
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  digitalWrite(BUILTIN_LED, LOW);  // Turn the internal status LED on by making the voltage LOW
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Make the payload into a string
  for (int i = 0; i < length; i++) { 
    //Serial.print((char)payload[i]);
    payloadStr = payloadStr + (char)payload[i];
  }
  // Convert string to a value and check for >0 and <100
  payloadVal=payloadStr.toFloat();
  if (payloadVal < 0)
    payloadVal=0;
  if (payloadVal > 100)
    payloadVal=100;
    
  Serial.println("Setting DAC to: " + payloadStr + "%");
  float payloadValB = (MAX_VALUE/100.0)*payloadVal; // Calculate setting from percentage
  Serial.print("New value: ");
  Serial.print((int)payloadValB);
  Serial.println("[end]");
  
  dac.send(channelA, (int) payloadValB); // Cast value to integer and tell dac what value to use
  //blinkLED(1); // Just blink to show something was received
  payloadStr=""; // Clear payload string to avoid residual characters
}

// Inteerrupt and set timers and increment rpm counter
void isr() {
  unsigned long now = micros();
  if (numPulses == 1) {
    firstPulseTime = now;
  }
  else {
    lastPulseTime = now;
  }
  //Serial.print("isr");
   ++numPulses;
}

void startCounter() {
  numPulses = 0; // prime the system to start a new reading
  attachInterrupt(isrPin, isr, FALLING); // enable the interrupt
}

float getRPM() {
  detachInterrupt(isrPin); // disable interrupt while calculating
  return (numPulses < 3) ? 0 : (float)(numPulses - 2.0)*60000000.0/(float)(lastPulseTime - firstPulseTime);
}

/*
// Blink internal LED set number of times
void blinkLED (int reps) {
    digitalWrite(BUILTIN_LED, LOW);
    delay (100);
    for (int i = 0; i < reps; i++) {
      digitalWrite(BUILTIN_LED, HIGH);
      delay (100);
      digitalWrite(BUILTIN_LED, LOW);
      delay (100);
    } 
}
*/
boolean reconnect() {
  if (client.connect("Systemair")) {
     client.subscribe("home/systemair/in");
  }
  return client.connected();
}

void loop()
{
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) { // Wait 5 s between reconnection attempts
      Serial.print("Attempting MQTT connection...");
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        Serial.println("connected");
        lastReconnectAttempt = 0;
        // Report the default dac level at startup
        Serial.print("Publish default DAC level:");
        Serial.println(payloadVal);
        snprintf (msg, 75, "%ld", (int) payloadVal);
        client.publish("home/systemair/outLevel", msg);
      } 
      else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
      }
    }
  } else {
    // Client connected
    client.loop();

    // Check if it is time to report
    unsigned long timeNow = millis();
    if (timeNow - timeBefore > reportTime) {
      timeBefore = timeNow; // Reset time

      // Get RPM and report via mqtt
      long rpmCount = getRPM(); 
      snprintf (msg, 75, "%ld", (int)rpmCount);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("home/systemair/outRpm", msg);

      // Report signal strength
      long rssi = WiFi.RSSI();
      snprintf (msg, 75, "%ld dBm", rssi);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("home/systemair/outSig", msg);

      // restart the rpm counter
      startCounter();
    }
  }
}
