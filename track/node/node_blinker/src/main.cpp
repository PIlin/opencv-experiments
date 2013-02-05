
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


/////////



//////////

static SimpleCommand command;
static SimpleAnswer answer;


/////////

int ledPin = 13;

static uint8_t slCmd[] = {'S','L'};
static uint8_t shCmd[] = {'S','H'};


XBee xbee = XBee();

XBeeAddress64 addr64 = XBeeAddress64(0x00, 0xffff);
ZBTxRequest zbTx = ZBTxRequest(addr64, NULL, 0);
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

static XBeeResponse response = XBeeResponse();

static ZBRxResponse rx = ZBRxResponse();
static ModemStatusResponse msr = ModemStatusResponse();

// static AtCommandRequest atRequest = AtCommandRequest();
static AtCommandResponse atResponse = AtCommandResponse();

///

struct NodeID
{
  uint32_t msb;
  uint32_t lsb;
};

static NodeID  this_node;


///

// struct point_t
// {
//   uint8_t x;
//   uint8_t y;
// };

// static const uint8_t children_count = 4;
// struct child
// {
//   uint32_t addr;
//   uint8_t p;
// };
// static child children[children_count] =
// {
//   {0xE9795D40, 0},
//   {0xDC5C2D40, 1},
//   {0xD95C2D40, 2},
//   {0xDA795D40, 3}
// };

// static uint32_t addr = 0xE9795D40;
// static const uint8_t positions_count = 4;
// static point_t positions[positions_count] = {
//   {10, 10},
//   {15, 169},
//   {200, 215},
//   {140, 25}
// };

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
  DEBUG_PRINTLN2("packet_xb_writer", (int)size);
  zbTx.setPayload(data);
  zbTx.setPayloadLength(size);
  zbTx.setAddress16(0xFFFE);

  addr64.setMsb(0x0013a200);
  addr64.setLsb(0x402d5cd9);
  zbTx.setAddress64(addr64);

  xbee.send(zbTx);

  DEBUG_PRINTLN("packet_xb_writer done");

  return true;
}
/////////


#define DBGP(x) debug_print(x, packet_stream_writer)


////////////////////////


void send_beacon()
{
  DEBUG_PRINTLN("send_beacon");
  answer.node_id.lsb = this_node.lsb;
  answer.node_id.msb = this_node.msb;
  answer.command = ECommand_BEACON;
  answer.answer = 0;



  send_package(SimpleAnswer_fields, &answer, packet_xb_writer);
}

//

void process_command()
{
  DEBUG_PRINTLN("process_command");
  DEBUG_PRINTLN2("command ", command.command);
  DEBUG_PRINTLN2H("com.lsb  ", command.node_id.lsb);
  DEBUG_PRINTLN2H("this.lsb ", this_node.lsb);
  DEBUG_PRINTLN2H("com.msb  ", command.node_id.msb);
  DEBUG_PRINTLN2H("this.msb ", this_node.msb);

  answer.node_id.lsb = this_node.lsb;
  answer.node_id.msb = this_node.msb;
  answer.command = command.command;
  answer.number = command.number;
  answer.answer = 0;

  if (command.node_id.lsb == this_node.lsb
    && command.node_id.msb == this_node.msb)
  {

    switch(command.command)
    {
      case ECommand_LIGHT_ON:
      {
        DEBUG_PRINTLN("ECommand_LIGHT_ON");
        digitalWrite(ledPin, HIGH);

        answer.answer = 1;

        break;
      }
      case ECommand_LIGHT_OFF:
      {
        DEBUG_PRINTLN("ECommand_LIGHT_OFF");
        digitalWrite(ledPin, LOW);

        answer.answer = 2;

        break;
      }
      case ECommand_BEACON:
      {
        DEBUG_PRINTLN("ECommand_BEACON");
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

  if (!status)
  {
    DEBUG_PRINTLN2("process_incoming_packet error ", PB_GET_ERROR(isrt));
  }

  return status;
}

void process_ZB_RX_RESPONSE(ZBRxResponse& rx)
{
  uint8_t* data = rx.getData();
  uint8_t const length = rx.getDataLength();

  DEBUG_PRINTLN2("process_ZB_RX_RESPONSE length = ", length);
  for (int i =0;i < length; ++i)
  {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  DEBUG_PRINTLN(' ');


  pb_istream_t istream = pb_istream_from_buffer(data, length);


  process_incoming_packet(&istream);
}

void process_ZB_TX_STATUS_RESPONSE()
{
  static ZBTxStatusResponse txsr;
  xbee.getResponse().getZBTxStatusResponse(txsr);

  DEBUG_PRINTLN2H("rem adr ", txsr.getRemoteAddress());
  DEBUG_PRINTLN2H("del st  ", txsr.getDeliveryStatus());
  DEBUG_PRINTLN2H("disc st ", txsr.getDiscoveryStatus());
  DEBUG_PRINTLN2H("ret cnt ", txsr.getTxRetryCount());
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
        DEBUG_PRINTLN("AT_COMMAND_RESPONSE");
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
    case ZB_TX_STATUS_RESPONSE:
      {
        DEBUG_PRINTLN("ZB_TX_STATUS_RESPONSE");
        process_ZB_TX_STATUS_RESPONSE();
        break;
      }
    case MODEM_STATUS_RESPONSE:
      {
        DEBUG_PRINTLN("MODEM_STATUS_RESPONSE");
        static ModemStatusResponse msr;
        xbee.getResponse().getModemStatusResponse(msr);

        DEBUG_PRINTLN2H("status ", msr.getStatus());

        break;
      }
    default:
      {
        DEBUG_PRINTLN2H("Got packet with apiId ", xbee.getResponse().getApiId());
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

bool at_command(char const * c,
  uint8_t const* val = NULL, uint8_t val_size = 0,
  int timeout = 5000)
{
  bool status = false;
  uint8_t* p = (uint8_t*)c;

  static AtCommandRequest r;
  r.setCommand(p);
  if (val)
  {
    r.setCommandValue((uint8_t*)val);
    r.setCommandValueLength(val_size);
  }
  else
  {
    r.clearCommandValue();
  }

  xbee.send(r);

  unsigned long now = millis();
  unsigned long last = now;

  while (1)
  {
    if (AT_COMMAND_RESPONSE == process_xbee_packets(timeout))
    {
      status = (atResponse.getStatus() == 0);
      DEBUG_PRINTLN2("got AT_COMMAND_RESPONSE status ", atResponse.getStatus());
      break;
    }
    else
    {
      now = millis();
      if (now - last >= timeout)
      {
        DEBUG_PRINTLN("AT_COMMAND_RESPONSE timeout");
        break;
      }
    }
  }

  return status;
}

void get_our_address()
{
  DEBUG_PRINTLN("get_our_address");

  while (!at_command("SL"))
  { }
  //memcpy(&this_node.lsb, atResponse.getValue(), 4);
  //DEBUG_PRINTLN2("value len ", atResponse.getValueLength());

  for (int i = 0; i < 4; ++i)
    ((uint8_t*)(&this_node.lsb))[i] = atResponse.getValue()[3 - i];

  while (!at_command("SH"))
  { }
  //memcpy(&this_node.msb, atResponse.getValue(), 4);
  //DEBUG_PRINTLN2("value len ", atResponse.getValueLength());
  for (int i = 0; i < 4; ++i)
    ((uint8_t*)(&this_node.msb))[i] = atResponse.getValue()[3 - i];

  DEBUG_PRINTLN2H("this_node.msb", this_node.msb);
  DEBUG_PRINTLN2H("this_node.lsb", this_node.lsb);
}

void initial_xb_setup()
{
  {
    uint8_t p[] = {0x28, 0x42};
    at_command("ID", p, sizeof(p));
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

  DEBUG_PRINTLN("Greetings!");

  get_our_address();

  initial_xb_setup();

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

  // static unsigned long last_beacon = 0;
  // unsigned long now = millis();
  // if (now - last_beacon >= 30000)
  // {
  //   last_beacon = now;
  //   send_beacon();
  // }


  process_xbee_packets();
}
