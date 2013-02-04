
#include "Arduino.h"


#include "XBee/XBee.h"

#define DEBUG 1

#include "debug.h"

#include "protocol.hpp"
#include "simple_command.pb.h"

///////

#ifdef __AVR_ATmega32U4__
# define XBEE_SERIAL Serial1
#else
# include <SoftwareSerial.h>
SoftwareSerial soft_serial(10, 11);
# define XBEE_SERIAL soft_serial
#endif



//////////

static SimpleCommand command;
static SimpleAnswer answer;


/////////

int ledPin = 13;

void blink(uint8_t count, unsigned long t)
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


////

XBee xbee = XBee();

XBeeAddress64 addr64 = XBeeAddress64(0x00, 0xffff);
ZBTxRequest zbTx = ZBTxRequest(addr64, NULL, 0);
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

static ZBRxResponse rx = ZBRxResponse();
static ModemStatusResponse msr = ModemStatusResponse();

///

struct point_t
{
  uint8_t x;
  uint8_t y;
};

static const uint8_t children_count = 4;
struct child
{
  uint32_t addr;
  uint8_t p;
};
static child children[children_count] =
{
  {0xE9795D40, 0},
  {0xDC5C2D40, 1},
  {0xD95C2D40, 2},
  {0xDA795D40, 3}
};

static uint32_t addr = 0xE9795D40;
static const uint8_t positions_count = 4;
static point_t positions[positions_count] = {
  {10, 10},
  {15, 169},
  {200, 215},
  {140, 25}
};

////////////////////////

static bool packet_stream_reader(pb_istream_t* stream, uint8_t* buf, size_t count)
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

pb_istream_t istream_serial = {&packet_stream_reader, NULL, 65535u};

bool packet_stream_writer(uint8_t* data, uint8_t size)
{
  // on serial size + message


  if (Serial.write(size) != 1 ||
    Serial.write(data, size) != size)
  {

    return false;
  }

  return true;
}


bool packet_xb_writer(uint8_t* data, uint8_t size)
{
  zbTx.setPayload(data);
  zbTx.setPayloadLength(size);

  xbee.send(zbTx);

  return true;
}
/////////


#define DBGP(x) debug_print(x, packet_stream_writer)


////////////////////////


void change_pattern()
{
  // static uint8_t cur = 4;

  // ++cur;
  // cur = cur % positions_count;

  // DEBUG_PRINTLN2("next positions = ", cur);

  struct packet {
    uint8_t magic;
    uint8_t updates;
    struct info_t{
      uint32_t addr;
      point_t pos;
    } info[];
  };


  uint8_t buf[2 + sizeof(packet::info_t) * children_count];
  packet& p = *(packet*)buf;

  p.magic = 0x42;
  p.updates = children_count;

  for (int i = 0; i < children_count; ++i)
  {
    packet::info_t& info = p.info[i];

    info.addr = children[i].addr;

    children[i].p++;
    children[i].p %= positions_count;

    info.pos = positions[children[i].p];
  }


  zbTx.setPayload(buf);
  zbTx.setPayloadLength(sizeof(buf));


  // DEBUG_PRINTLN("payload");
  // for (int i = 0; i < zbTx.getPayloadLength(); ++i)
  // {
  //   Serial.print(zbTx.getPayload()[i], HEX);
  // }
  // DEBUG_PRINTLN();

  xbee.send(zbTx);

  // DEBUG_PRINTLN("send done");
}

void process_command()
{
  send_package(SimpleCommand_fields, &command, packet_xb_writer);
}

void process_answer()
{
  send_package(SimpleAnswer_fields, &answer, packet_stream_writer);
}

void process_debug_print_package()
{
  send_package(SimpleAnswer_fields, &debug_print_message, packet_stream_writer);
}

bool process_incoming_packet(pb_istream_t* isrt)
{
  bool status = false;

  const pb_field_t *type = decode_unionmessage_type(isrt, MessagePackage_fields);

  if (SimpleCommand_fields == type)
  {
    status = decode_unionmessage_contents(isrt, SimpleCommand_fields, &command);
    if (status)
      process_command();
  }
  else if (SimpleAnswer_fields == type)
  {
    status = decode_unionmessage_contents(isrt, SimpleAnswer_fields, &answer);
    if (status)
      process_answer();
  }
  else if (DebugPrint_fields == type)
  {
    status = decode_unionmessage_contents(isrt, DebugPrint_fields, &debug_print_message);
    if (status)
      process_debug_print_package();
  }
  else
  {
    // Serial.println("Unknown package");
    // ERROR_BLINK(9);
  }

  return status;
}


///////////////////

void process_ZB_RX_RESPONSE(ZBRxResponse& rx)
{
  // const uint8_t magic = 0x42;
  uint8_t* data = rx.getData();
  uint8_t const length = rx.getDataLength();


  pb_istream_t istream = pb_istream_from_buffer(data, length);


  process_incoming_packet(&istream);
}

uint8_t process_xbee_packets(int timeout = 0)
{
  uint8_t api_id = 0xff;


  if (timeout)
  {
    if (!xbee.readPacket(timeout))
    {
      return api_id;
    }
  }
  else
    xbee.readPacket();


  if (xbee.getResponse().isAvailable())
  {
    api_id = xbee.getResponse().getApiId();
    switch (api_id)
    {
    // case AT_COMMAND_RESPONSE:
    //   {
    //     xbee.getResponse().getAtCommandResponse(atResponse);
    //     break;
    //   }
    case ZB_RX_RESPONSE:
      {
        // DEBUG_PRINTLN("ZB_RX_RESPONSE");

        xbee.getResponse().getZBRxResponse(rx);
        process_ZB_RX_RESPONSE(rx);
        break;
      }
    default:
      {
        DBGP("xbee got unknown packet");
        // DEBUG_PRINTLN2("Got packet with apiId", xbee.getResponse().getApiId());
        break;
      }
    } // swtich
  }
  else if (xbee.getResponse().isError())
  {
    // DEBUG_PRINTLN2("Error reading packet.  Error code: ", xbee.getResponse().getErrorCode());
    DBGP("error reading packet");
  }

  return api_id;
}

///




void process_serial_package()
{
  static uint8_t size = 0;
  static bool in_package = false;

  while (Serial.available() > 0)
  {
    if (in_package)
    {
      istream_serial.bytes_left = size;


      process_incoming_packet(&istream_serial);


      if (istream_serial.bytes_left > 0)
      {
        // Serial.println("Bytes left");
        while (istream_serial.bytes_left--)
          Serial.read();
      }

      in_package = false;
      ERROR_BLINK(1);
    }
    else
    {
      if (Serial.available() > 0)
      {
        if (Serial.readBytes((char*)&size, 1) == 1)
        {
          DBGP("in package");

          in_package = true;
        }
      }
    }
  }
}


///

void setup()
{
  pinMode(ledPin, OUTPUT);

  Serial.begin(9600);

  XBEE_SERIAL.begin(9600);
  xbee.begin(XBEE_SERIAL);


  while (!Serial);

  // ERROR_BLINK(5);

  DBGP("Greetings!");
}

void loop()
{

  // if (Serial.available() > 0)
  // {
  //   char r = Serial.read();
  //   // DEBUG_PRINTLN2("serial available", r);

  //   change_pattern();
  // }

  process_serial_package();

  process_xbee_packets();
}
