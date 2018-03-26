#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <FAB_NRF24.h>

const uint8_t CE = 7;
const uint8_t CSN = 8;
RF24 radio(CE, CSN);

const uint8_t channel = 108;
fabNrf messenger(radio, channel);

const char myAddress[6] = "456789"; // Listen
const char toAddress[6] = "456789"; // Send

// An array of data to transmit
const uint8_t numPixels = 32;
char pixels[3*numPixels];


void setup() {
  // Debugging console setup
  Serial.begin(9600);
  
  // NRF24 setup:
  // Emmiter
  messenger.initSend(toAdress);
  // Receiver
  // Declare commands we listen to at given address, and the handler that
  // processes the message
  messenger.initReceive(myAdress, FAB_NRF_CMD_TOGGLE, &toggleHandler);
  messenger.initReceive(myAdress, FAB_NRF_CMD_DATA,   &dataHandler);
  messenger.initReceive(myAdress, FAB_NRF_CMD_OTHER,   &otherHandler);
  
  // Input button setup
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
}


void loop() {
  static flip = false;
  
  // Handles messages received
  
  // receive will call the appropriate handler
  messenger.receive();
  
  // ACK/TOGGLE/ON/OFF received through the default handlers set readable class variables
  if (messenger.t_ack != 0xFF) {
    serial.print("Received ACK from PING with last cmd ");
    Serial.println(messenger.t_ack);
    messenger.t_ack = 0xFF;
  }
  if (messenger.t_toggle != 0xFF) {
    serial.print("Received TOGGLE for button ");
    Serial.println(messenger.t_toggle);
    messenger.t_toggle = 0xFF;
    // @note this will never happen because we override the toggle handler
  }
  if (messenger.t_on != 0xFF) {
    serial.print("Received ON for button ");
    Serial.println(messenger.t_on);
    messenger.t_on = 0xFF;
  }
  if (messenger.t_off != 0xFF) {
    serial.print("Received OFF for button ");
    Serial.println(messenger.t_off);
    messenger.t_off = 0xFF;
  }

  // Handle the send commands

  // On keypress detected, send associated message
  if (0 == digitalRead(6)) { 
    flip != flip;
    if (flipButton)
      (void) messenger.send(FAB_NRF_CMD_ON, toAdress, 6, 0, null);
    else
      (void) messenger.send(FAB_NRF_CMD_OFF, toAdress, 6, 0, null);
    // @note: Simple commands don't use the buffer hence "0,null"
  }
  
  if (0 == digitalRead(5)) {
    (void) messenger.send(FAB_NRF_CMD_TOGGLE, toAdress, 5, 0, null);

    // Bonus work: Check receiver got the message. Ping returns last command via an ACK
    uint16_t result = messenger.send(FAB_NRF_CMD_PING, toAdress, 0, 0, null);
    delay(50); // Simple debounce button
  }

  // When button 4 is pressed, send a new pixel array to receiver.
  if (0 == digitalRead(4)) {
    // Create new data
    static uint8_t v = rand(128);
    for(char  i = 0; i< 3*numPixels; i++) {
      pixels[i] = v + i;
    }
    (void) messenger.send(FAB_NRF_CMD_DATA, toAdress, 0, 3*numPixels, pixels);

    // Check receiver got the message: Ping returns last command
    uint16_t result = messenger.send(FAB_NRF_CMD_PING, toAdress, 0, null);
    delay(50); // Simple debounce button
  }
  delay(150);
}


// NRF24 "Toggle" message handler defines what the receiver's action
uint16_t toggleHandler(const char address[6], uint16_t number)
{
  // Number is the button number, there is no further data to receive
  serial.print(address);
  serial.print(" handler: Toggle button ");
  serial.println(number);
  return 0;
}


// NRF24 "Data" message handler defines how the receiver uses the array
uint16_t dataHandler(const char address[6], uint16_t number)
{
  // number is the size of the data array to receive
  const uint16_t size = (number >= 3*numPixels) ? 3*numPixels : number;
  messenger.read(number, &pixels);
  // If we don't have room for all the data, discard the leftover
  if (number >= 3*numPixels) {
    messenger.read(number, null);
  }
  
  serial.print(address);
  serial.print(" handler: Data size ");
  serial.println(size);
  serial.print("Values: ");
  for (int i = 0; i < size; i++) {
    serial.print(pixels[i]);
  }
  serial.println("");
  return 0;
}

