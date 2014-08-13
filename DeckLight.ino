#include <SPI.h>
#include <Ethernet.h>
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>

// NeoPixel strip
#define PIN 7
#define PIXEL_COUNT = 14;

// Onboard LED
int led = 9;
int switchInput = 2;
int pirInput = 3;
int switchLight = 5;
int pirControl = 6;
volatile bool lightsOn = true;
volatile bool updateLights = false;
volatile bool switchLightOn = true;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(26, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

// Update these with values suitable for your network.
// POE unit.
byte mac[]    = {  0x90, 0xA2, 0xDA, 0x00, 0x71, 0xDD };

// Tinamous setup
int port = 1883;
char server[] = "[AccountName].Tinamous.com";
char userName[] = "[UserName].[AccountName]";
char password[] = "";

// Fall back values if DHCP fails.
byte ip[] = { 10, 0, 0, 158 };
byte dnss[] = { 10, 0, 0, 99 };
byte gateway[] = {10, 0, 0, 138};
byte subnet[] = {255,255,127,0};

// The current color scheme
byte red = 0;
byte green = 0;
byte blue = 255;

// Buffer to hold the requested color settings.
char colorBuffer[2];

// buffer for the payload characters received from MQTT subscription callback.
// Allocate once to help with memory usage.
// callback messages must not be over 127 characters!
char payloadChars[127];

int loopCounter = 0;

// Callback needs to be defined before pub sub
// but implemented after so it can use PubSubClient
void callback(char* topic, byte* payload, unsigned int length);

EthernetClient ethClient;
PubSubClient client(server, port, callback, ethClient);

// Gets the colour byte value from 2 charaters in the payload
byte GetColorByte(int offset, byte* payload) {
  
  colorBuffer[0] = (char)payload[offset];
  colorBuffer[1] = (char)payload[offset+1];      

  // Convert the hex string (e.g. #ff) to a number using base 16
  byte number = (byte)strtol(colorBuffer, NULL, 16);  

  return number;
}

// Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  // Look at the payload.
  // Look for the #
  // should be 6 Hex digits
 
  // Flash the switch light to see what's happening.
  digitalWrite(switchLight, LOW);
  delay(250);
  digitalWrite(switchLight, HIGH);
    
  // Convert the payload from byte array pointer to char array pointer.
  // Note no \0 at the end of the message.
  //char *payloadChars = (char *) payload;
  
  strcpy(payloadChars, (char*)payload);
 
  // Will this need the null terminator?
  char* character = strchr(payloadChars, '#');
  
  int offset = findHash(payloadChars, length, '#');
  
  // TODO: Rather than forcing the # at the start allow it to be anywhere.
  if (offset >= 0) { 
    // offset is the position of # so move one after.
    offset++;
    red = GetColorByte(offset, payload);
    green = GetColorByte(offset + 2, payload);
    blue = GetColorByte(offset + 4, payload);
    updateLights = true;
    
    // Don't update in actual callback, 
    // let the loop handle the update.
  } 

  // Republish what was received on the "outTopic"
  // client.publish("outTopic", topic);
}

int findHash(char* source, unsigned int length, char toFind) {    
   int i=0;
   // NB: source is UTF8 from MQTT. May include \0 as part of the character stream 
   // which could be mistaken for end of string.
   for (int i = 0; i< length; i++)
   {         
      if (source[i] == '#') {
        return i;
     }
   }
   return -1;
}

void setup() {
 
  // Setup the Neopixel strip
  strip.begin();
  // Setup pixels 0-3 to be blue whilst DHCP/MQTT is initializing.
  strip.setPixelColor(0, strip.Color(0, 0, 127));
  strip.setPixelColor(1, strip.Color(0, 0, 127));
  strip.setPixelColor(2, strip.Color(0, 0, 127));
  strip.setPixelColor(3, strip.Color(0, 0, 127));
  strip.show();

  // Onboard LED
  pinMode(led, OUTPUT);
  
  // Switch
  pinMode(switchInput, INPUT_PULLUP);
  pinMode(switchLight, OUTPUT);
  attachInterrupt(0, SwitchPressed, CHANGE);
  
  // PIR
  pinMode(pirInput, INPUT_PULLUP);
  pinMode(pirControl, OUTPUT);
  attachInterrupt(1, PirInput, CHANGE);
    
  InitializeEthernet();
  InitializeMQTT();
  
  // Set the LED pin high to show network setup running
  digitalWrite(led, HIGH);
  digitalWrite(switchLight, HIGH);
  digitalWrite(pirControl, HIGH);
}

// Initialize the ethernet using DHCP, if that fails then fall back to hardcoded values.
// Setting:
// Pixel 0 goes red to show it failed, or green for DHCP success.
void InitializeEthernet() {
  if (Ethernet.begin(mac) == 0) {
    // Failed to configure ethernet for DHCP.
    // Set the first pixel to red.
    strip.setPixelColor(0, strip.Color(255, 0, 0));
    Ethernet.begin(mac, ip, dnss, gateway, subnet);
  } else {
    // If we connected set the first pixel to green.
    strip.setPixelColor(0, strip.Color(0,255, 0));
  }
  
  strip.show();
}

// Initialize MQTT connection
// Setting:
// Pixel 1 goes red to show failed MQTT connection, green for connected.
void InitializeMQTT() {
  if (client.connect("kitchenLightsClient", userName, password)) {
    // Initialize second Pixel to Green to show we've connected to the MQTT service.
    strip.setPixelColor(1, strip.Color(0, 255, 0));
    
    // Publish status to let the world know the lights are connected.
    client.publish("/Tinamous/V1/Status","Kitchen Lights Initialized");
    
    // Subscribe to ... user 
    if (client.subscribe("/Tinamous/V1/Status.To/KitchenLights")) {
      // Set the onboard LED to network/MQTT initialization completed.
      digitalWrite(led, LOW); 
    }
  } else {
    // Set red on second pixel to show MQTT connection failed.
    strip.setPixelColor(1, strip.Color(255, 0, 0));
  }
  strip.show();
}

// Main loop
void loop() {

  // Needs to be called regularly to allow processing of messages and maintain connection to the server.
  client.loop();
    // if client is not connected then try and reconnet.
//    InitializeMQTT();
  
  // Every now and then maintain the ethernet connection to keep DHCP alive.
  // and publish a set of measurements.
  if (loopCounter > 1000) {
    // Maintain the DHCP.
    Ethernet.maintain();
    PublishMeasurements();
    loopCounter = 0;
  }
  
  if (updateLights) {
    setAllPixels();
    digitalWrite(switchLight, HIGH);
  }
  
  // Combined with updateLights makes a bit of debounce for the switch.
  delay(500);
}

// ISR from switch pin
void SwitchPressed() {
  digitalWrite(led, true);
  
  // Lights already queued for an update so don't do another.
  // this is likley due to switch bounce.
  if (updateLights) {
    return;
  }
    
  // If the switch is pressed then toggle the lights on/off
  // holding the switch will caluse the lights to flash.
  // Switch input is pulled high with resitor. low = pressed
  int switchValue = digitalRead(switchInput);
   
  // If switch pressed
  if (!switchValue) {   
    lightsOn = !lightsOn;
    updateLights = true;    
  } 
  
  digitalWrite(led, false);
}

// ISR from PIR pin
void PirInput() {
  // TODO: Handle PIR
}

void PublishMeasurements() {
  // Read and publish light and temperature levels (every 10 seconds)
  //client.publish("/Tinamous/V1/Measurements/0/Field8","257");
  client.publish("/Tinamous/V1/Status","Kitchen Lights Alive :-)");
}


void setAllPixels() { 
  // Off whilst processing
  digitalWrite(switchLight, LOW);
  
  uint32_t color;
  String message = "Kitchen lights ";
  
  if (lightsOn) {  
    color =  strip.Color(red, green, blue);
    message+= "set to  ";
    message+= "Red: ";
    message+= (int)red;
    message+=", Green: ";
    message+= (int)green;
    message+=", Blue: ";
    message+= (int)blue;
    message+= ".";
  } else {
        // Override with master lightsOn -> off setting.
    color = strip.Color(0, 0, 0);
    message+="off";
  }
  
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, color);
  }

  strip.show();
  publishLightsStatus(message);
  updateLights = false;
  digitalWrite(switchLight, HIGH);
}

void publishLightsStatus(String message) {
  // TODO: Figure out the lights status in here.
  
  // Now publish the message.  
  char charBuf[message.length()+1];
  message.toCharArray(charBuf, message.length()+1);
  client.publish("/Tinamous/V1/Status",charBuf);
}

void flash(uint32_t color, uint8_t wait, uint8_t times) {
  for (int j=0; j < times; j++) {
    setAllColorAndWait(color, wait);
    // All off.
    setAllColorAndWait(0, wait);
  }
}

void setAllColorAndWait(uint32_t color, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
    
  delay(wait);
}
