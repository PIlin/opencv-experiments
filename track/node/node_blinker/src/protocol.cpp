

#include <Arduino.h>



#include "protocol.hpp"

#include "simple_command.pb.h"
#include <pb_decode.h>
#include <pb_encode.h>


extern int ledPin;
extern void blink(uint8_t count, unsigned long t);
#define ERROR_BLINK(count) blink(count, 100)


/////////////////////////////////////////////


static bool read_callback(pb_istream_t* stream, uint8_t* buf, size_t count);




#define PB_WRITE_BUF_SIZE 64
static uint8_t pb_write_buf[PB_WRITE_BUF_SIZE];

DebugPrint debug_print_message;

/////////////////////////////////////////////


static bool write_string_callback(pb_ostream_t *stream, const pb_field_t *field, const void *arg)
{
  uint8_t const* s = static_cast<uint8_t const*>(arg);
  char const* sc = static_cast<char const*>(arg);

  if (!pb_encode_tag_for_field(stream, field))
    return false;

  size_t len = strlen(sc);


  return pb_encode_string(stream, s, len);
}



//////////////////////////////////////////////////////

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


  while (pb_decode_tag(stream, &wire_type, &tag, &eof))
  {

    if (wire_type == PB_WT_STRING)
    {

      const pb_field_t *field;
      for (field = uniontype; field->tag != 0; field++)
      {
        if (field->tag == tag && (field->type & PB_LTYPE_SUBMESSAGE))
        {
          /* Found our field. */
          return reinterpret_cast<const pb_field_t*>(field->ptr);
        }
      }
    }

    /* Wasn't our field.. */
    pb_skip_field(stream, wire_type);
  }


  return NULL;
}

bool decode_unionmessage_contents(pb_istream_t *stream, const pb_field_t fields[], void *dest_struct)
{

  pb_istream_t substream;
  bool status;
  if (!pb_make_string_substream(stream, &substream))
  {


    return false;
  }



  status = pb_decode(&substream, fields, dest_struct);

  if (!status)
  {
  }

  pb_close_string_substream(stream, &substream);
  return status;
}

bool send_package(const pb_field_t messagetype[], const void *message, writer_func func)
{
  pb_ostream_t ostream = pb_ostream_from_buffer(pb_write_buf, PB_WRITE_BUF_SIZE);


  if (!encode_unionmessage(&ostream, MessagePackage_fields, messagetype, message))
  {
    // ERROR_BLINK(3);
    return false;
  }
  else
  {
    // if (stream.write((uint8_t)ostream.bytes_written) != 1 ||
    //   stream.write(pb_write_buf, ostream.bytes_written) != ostream.bytes_written)
    // {
    //   // ERROR_BLINK(6);
    //   return false;
    // }

    return func(pb_write_buf, ostream.bytes_written);
  }

  return true;
}


/////////////////////////////


void debug_print(char const* c, writer_func func)
{
  debug_print_message.what.funcs.encode = write_string_callback;
  debug_print_message.what.arg = const_cast<char*>(c);


  send_package(DebugPrint_fields, &debug_print_message, func);
}

/////////////////////////////
