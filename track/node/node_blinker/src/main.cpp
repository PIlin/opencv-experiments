
#include "Arduino.h"


#include "XBee/XBee.h"

#define DEBUG 1

#include "debug.h"

#include "protocol.hpp"
#include "simple_command.pb.h"
#include <pb_decode.h>
#include <pb_encode.h>

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

static uint8_t slCmd[] = {'S','L'};


XBee xbee = XBee();

XBeeAddress64 addr64 = XBeeAddress64(0x00, 0xffff);
ZBTxRequest zbTx = ZBTxRequest(addr64, NULL, 0);
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

static XBeeResponse response = XBeeResponse();

static ZBRxResponse rx = ZBRxResponse();
static ModemStatusResponse msr = ModemStatusResponse();

static AtCommandRequest atRequest = AtCommandRequest();
static AtCommandResponse atResponse = AtCommandResponse();

///

static uint32_t sl = 0;


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


void send_beacon()
{
  DEBUG_PRINT("send_beacon");
  answer.node_id = sl;
  answer.command = ECommand_BEACON;
  answer.answer = 0;

  send_package(SimpleAnswer_fields, &answer, packet_xb_writer);
}

//

void process_command()
{
  answer.node_id = sl;
  answer.command = command.command;
  answer.answer = 0;

  if (command.node_id == sl)
  {

    switch(command.command)
    {
      case ECommand_LIGHT_ON:
      {
        DEBUG_PRINT("ECommand_LIGHT_ON");
        digitalWrite(ledPin, HIGH);

        answer.answer = 1;

        break;
      }
      case ECommand_LIGHT_OFF:
      {
        DEBUG_PRINT("ECommand_LIGHT_OFF");
        digitalWrite(ledPin, LOW);

        answer.answer = 2;

        break;
      }
      case ECommand_BEACON:
      {
        DEBUG_PRINT("ECommand_BEACON");
        send_beacon();
        break;
      }
    }

  }

  send_package(SimpleAnswer_fields, &answer, packet_xb_writer);
}

void process_answer()
{
  DEBUG_PRINTLN("don't want answers");
}

void process_debug_print_package()
{
  DEBUG_PRINTLN("don't want debugs");
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
    case AT_COMMAND_RESPONSE:
      {
        xbee.getResponse().getAtCommandResponse(atResponse);
        break;
      }
    case ZB_RX_RESPONSE:
      {
        DEBUG_PRINTLN("ZB_RX_RESPONSE");

        xbee.getResponse().getZBRxResponse(rx);
        process_ZB_RX_RESPONSE(rx);
        break;
      }
    default:
      {
        DEBUG_PRINTLN2("Got packet with apiId", xbee.getResponse().getApiId());
        break;
      }
    } // swtich
  }
  else if (xbee.getResponse().isError())
  {
    DEBUG_PRINTLN2("Error reading packet.  Error code: ", xbee.getResponse().getErrorCode());
  }

  return api_id;
}

void get_our_sl()
{
  // DEBUG_PRINT("get_our_sl");

  atRequest.setCommand(slCmd);

  while (true)
  {
    xbee.send(atRequest);
    if (AT_COMMAND_RESPONSE == process_xbee_packets(5000))
    {
      if (atResponse.isOk())
        break;
    }
  }

  char * const begin = reinterpret_cast<char*>(atResponse.getValue());
  char * end = begin + atResponse.getValueLength();


  // DEBUG_PRINTLN2("result len = ", atResponse.getValueLength());

  // for (int i = 0; i < atResponse.getValueLength(); ++i)
  // {
  //   Serial.print((uint8_t)begin[i], HEX);
  // }
  // DEBUG_PRINTLN();

  sl = *reinterpret_cast<uint32_t*>(begin);

  DEBUG_PRINT("sl = ");
  Serial.println(sl, HEX);
}


///

void setup()
{
  pinMode(ledPin, OUTPUT);

  Serial.begin(9600);

  XBEE_SERIAL.begin(9600);
  xbee.begin(XBEE_SERIAL);

  //while (!Serial);

  get_our_sl();

  // DBGP("Greetings!");
}

void loop()
{

  // if (Serial.available() > 0)
  // {
  //   char r = Serial.read();
  //   // DEBUG_PRINTLN2("serial available", r);

  //   change_pattern();
  // }

  //process_serial_package();

  //perform_blink();

  static unsigned long last_beacon = 0;
  unsigned long now = millis();
  if (now - last_beacon >= 30000)
  {
    last_beacon = now;
    send_beacon();
  }


  process_xbee_packets();
}
