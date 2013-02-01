
#include "Arduino.h"


#include "simple_command.pb.h"
#include <pb_decode.h>
#include <pb_encode.h>

const int ledPin = 13; // the pin that the LED is attached to
int incomingByte;      // a variable to read incoming serial data into

static void blink(uint8_t count, unsigned long t)
{
  while (count--)
  {
    digitalWrite(ledPin, HIGH);
    delay(t);
    digitalWrite(ledPin, LOW);
    delay(t);
  }
}

#define ERROR_BLINK(count) blink(count, 100)





static simple_command command;
static simple_answer answer;

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
    {
      if (Serial.readBytes(&c, 1) < 1)
      {
        return false;
      }
    }
  }

  return true;
}

#define PB_WRITE_BUF_SIZE 32
static uint8_t pb_write_buf[PB_WRITE_BUF_SIZE];


pb_istream_t istream = {&read_callback, NULL, 65535u};

void process_command()
{

  answer.command = command.command;
  answer.node_id = command.node_id;
  answer.answer = 0;

  switch(command.command)
  {
    case ECommand_LIGHT_ON:
    {
      digitalWrite(ledPin, HIGH);

      answer.answer = 1;

      break;
    }
    case ECommand_LIGHT_OFF:
    {
      digitalWrite(ledPin, LOW);

      answer.answer = 1;

      break;
    }
  }

  pb_ostream_t ostream = pb_ostream_from_buffer(pb_write_buf, PB_WRITE_BUF_SIZE);
  for (int i = 0; i < 1; i++)
  {
    if (!pb_encode(&ostream, simple_answer_fields, &answer))
    {
      ERROR_BLINK(3);
    }
    else
    {
      if (Serial.write((uint8_t)ostream.bytes_written) != 1 ||
        Serial.write(pb_write_buf, ostream.bytes_written) != ostream.bytes_written)
      {
        ERROR_BLINK(6);
      }
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

      bool res = pb_decode(&istream, simple_command_fields, &command);

      if (res)
        process_command();
      else
      {
        ERROR_BLINK(9);
      }

      in_package = false;
    }
    else
    {
      if (Serial.available() > 0)
      {
        if (Serial.readBytes((char*)&size, 1) == 1)
        {
          in_package = true;
        }
      }
    }
  }

}


void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  read_package();
}

