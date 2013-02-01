
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



static bool write_string_callback(pb_ostream_t *stream, const pb_field_t *field, const void *arg)
{
  uint8_t const* s = static_cast<uint8_t const*>(arg);
  char const* sc = static_cast<char const*>(arg);

  if (!pb_encode_tag_for_field(stream, field))
    return false;

  return pb_encode_string(stream, s, strlen(sc));
}

static SimpleCommand command;
static SimpleAnswer answer;
static DebugPrint debug_print_message;



static bool read_callback(pb_istream_t* stream, uint8_t* buf, size_t count)
{
  // Serial.print("read_callback count = ");
  // Serial.println(count);
  if (buf)
  {
    // Serial.print("readBytes ");
    if (Serial.readBytes((char*)buf, count) < count)
    {
      // Serial.println("error");
      return false;
    }

    // while (count--)
    //   Serial.print(*buf++, HEX);
    // Serial.println();
  }
  else
  {
    // Serial.print("readBytes skipping");
    char c;
    while (count--)
    {
      if (Serial.readBytes(&c, 1) < 1)
      {
        // Serial.println("error");
        return false;
      }

      // Serial.print(c, HEX);
    }
    // Serial.println();
  }

  return true;
}

#define PB_WRITE_BUF_SIZE 32
static uint8_t pb_write_buf[PB_WRITE_BUF_SIZE];


pb_istream_t istream = {&read_callback, NULL, 65535u};

bool encode_unionmessage(pb_ostream_t *stream, const pb_field_t uniontype[], const pb_field_t messagetype[], const void *message)
{
    const pb_field_t *field;
    for (field = uniontype; field->tag != 0; field++)
    {
        if (field->ptr == messagetype)
        {
            /* This is our field, encode the message using it. */
            if (!pb_encode_tag_for_field(stream, field))
                return false;

            return pb_encode_submessage(stream, messagetype, message);
        }
    }

    /* Didn't find the field for messagetype */
    return false;
}

const pb_field_t* decode_unionmessage_type(pb_istream_t *stream,  const pb_field_t uniontype[])
{
  pb_wire_type_t wire_type;
  uint32_t tag;
  bool eof;

  // Serial.println("decode_unionmessage_type");

  while (pb_decode_tag(stream, &wire_type, &tag, &eof))
  {
    // Serial.print("wire_type = ");
    // Serial.println((int)wire_type);
    // Serial.print("tag = ");
    // Serial.println((int)tag);

    if (wire_type == PB_WT_STRING)
    {
      // Serial.println("wire_type == PB_WT_STRING");

      const pb_field_t *field;
      for (field = uniontype; field->tag != 0; field++)
      {
        if (field->tag == tag && (field->type & PB_LTYPE_SUBMESSAGE))
        {
          // Serial.println("/* Found our field. */");
          /* Found our field. */
          return reinterpret_cast<const pb_field_t*>(field->ptr);
        }
      }
    }

    // Serial.println("/* Wasn't our field.. */");
    /* Wasn't our field.. */
    pb_skip_field(stream, wire_type);
  }

  // Serial.print("last error ");
  // Serial.println(PB_GET_ERROR(stream));

  return NULL;
}

bool decode_unionmessage_contents(pb_istream_t *stream, const pb_field_t fields[], void *dest_struct)
{
  // Serial.println("decode_unionmessage_contents");

  pb_istream_t substream;
  bool status;
  if (!pb_make_string_substream(stream, &substream))
  {
    // Serial.print("last stream error ");
    // Serial.println(PB_GET_ERROR(stream));

    // Serial.print("last substream error ");
    // Serial.println(PB_GET_ERROR(&substream));

    return false;
  }

  // Serial.print("stream bytes left ");
  // Serial.println(stream->bytes_left);

  // Serial.print("substream bytes left ");
  // Serial.println(substream.bytes_left);

  // Serial.println("decoding submessage");
  status = pb_decode(&substream, fields, dest_struct);

  if (!status)
  {
    // Serial.print("last substream error ");
    // Serial.println(PB_GET_ERROR(&substream));
  }

  pb_close_string_substream(stream, &substream);
  return status;
}

static bool send_package(const pb_field_t messagetype[], const void *message)
{
  pb_ostream_t ostream = pb_ostream_from_buffer(pb_write_buf, PB_WRITE_BUF_SIZE);

  if (!encode_unionmessage(&ostream, MessagePackage_fields, messagetype, message))
  {
    // ERROR_BLINK(3);
    return false;
  }
  else
  {
    if (Serial.write((uint8_t)ostream.bytes_written) != 1 ||
      Serial.write(pb_write_buf, ostream.bytes_written) != ostream.bytes_written)
    {
      // ERROR_BLINK(6);
      return false;
    }
  }

  return true;
}


void debug_print(char const* c)
{
  debug_print_message.what.arg = const_cast<char*>(c);

  send_package(DebugPrint_fields, &debug_print_message);
}


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

  send_package(SimpleAnswer_fields, &answer);
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

      //bool res = pb_decode(&istream, simple_command_fields, &command);
      const pb_field_t *type = decode_unionmessage_type(&istream, MessagePackage_fields);
      bool status = false;

      if (type == SimpleCommand_fields)
      {
        status = decode_unionmessage_contents(&istream, SimpleCommand_fields, &command);
        if (status)
          process_command();
      }
      else
      {
        // Serial.println("Unknown package");
        ERROR_BLINK(9);
      }

      if (!status)
      {
        // Serial.println("Unable to decode package");
        ERROR_BLINK(12);
      }

      if (istream.bytes_left > 0)
      {
        // Serial.println("Bytes left");
        while (istream.bytes_left--)
          Serial.read();
      }

      in_package = false;
    }
    else
    {
      if (Serial.available() > 0)
      {
        if (Serial.readBytes((char*)&size, 1) == 1)
        {
          // Serial.print("in_package = true; size = ");
          // Serial.println(size);

          debug_print("in package");

          in_package = true;
        }
      }
    }
  }


}


void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);


  debug_print_message.what.funcs.encode = write_string_callback;

}

void loop() {
  read_package();
}

