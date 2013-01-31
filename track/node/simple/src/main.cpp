
#include "Arduino.h"


#include "simple_command.pb.h"
#include <pb_decode.h>

const int ledPin = 13; // the pin that the LED is attached to
int incomingByte;      // a variable to read incoming serial data into


static simple_command command;

static bool read_callback(pb_istream_t* stream, uint8_t* buf, size_t count)
{
  if (buf)
  {
    if (Serial.readBytes((char*)buf, count) < count)
    {
      return false;
    }
  }
  else
  {
    char c;
    while (count--)
      if (Serial.readBytes(&c, 1) < 1)
        return false;
  }

  return true;
}

pb_istream_t istream = {&read_callback, NULL, 65535};


void process_command()
{
  switch(command.command)
  {
    case ECommand_LIGHT_ON:
    {
      digitalWrite(ledPin, HIGH);
      break;
    }
    case ECommand_LIGHT_OFF:
    {
      digitalWrite(ledPin, LOW);
      break;
    }
  }
}


void read_package()
{
  static uint8_t size = 0;
  static bool in_package = false;

  while (Serial.available() > 0)
  {
    if (in_package)
    {
      istream.bytes_left = size;

      pb_decode(&istream, simple_command_fields, &command);

      process_command();

      in_package = false;
    }
    else
    {
      if (Serial.available() > 0)
        if (Serial.readBytes((char*)&size, 1) == 1)
          in_package = true;
    }
  }

}


void setup() {
  // initialize serial communication:
  Serial.begin(9600);
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // see if there's incoming serial data:
  if (Serial.available() > 0) {
    // read the oldest byte in the serial buffer:
    incomingByte = Serial.read();
    // if it's a capital H (ASCII 72), turn on the LED:
    if (incomingByte == 'H') {
      digitalWrite(ledPin, HIGH);
    }
    // if it's an L (ASCII 76) turn off the LED:
    if (incomingByte == 'L') {
      digitalWrite(ledPin, LOW);
    }

    delay(30);
    Serial.write(incomingByte);



  }
}