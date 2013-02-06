
#include "Arduino.h"


#include "XBee/XBee.h"

#define DEBUG 2

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

////////////


size_t BufferPrint::write(uint8_t c)
{
  if (s > 1)
  {
    buf[written] = c;
    ++written;
    buf[written] = 0;
    --s;
    return 1;
  }
  return 0;
}


#define DEBUG_PRINT_BUF_SIZE 64
uint8_t debug_print_buf[DEBUG_PRINT_BUF_SIZE];
BufferPrint bufferPrint;
void reset_buffer_print(void)
{
  debug_print_buf[0] = 0;
  bufferPrint = BufferPrint(debug_print_buf, DEBUG_PRINT_BUF_SIZE);
}





//////////

static SimpleCommand command;
static SimpleAnswer answer;
static PositionNotify pos_notify;

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
static AtCommandResponse atResponse = AtCommandResponse();
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


void DBGP(char const* x)
{
  debug_print(x, packet_stream_writer);
}

bool packet_xb_writer(uint8_t* data, uint8_t size)
{
/*
  reset_buffer_print();
  bufferPrint.print("packet_xb_writer size "); bufferPrint.print(size);
  DBGP((char const*)bufferPrint.buf);

  reset_buffer_print();
  for (int i = 0; i < size; ++i)
  {
    bufferPrint.print(data[i], HEX); bufferPrint.print(" ");
  }
  DBGP((char const*)bufferPrint.buf);
*/
/*
  Serial.print("packet_xb_writer size "); Serial.println(size);
  Serial.print("packet_xb_writer data "); Serial.println((size_t)data, HEX);
  for (int i = 0; i < size; ++i)
  {
    Serial.print(data[i], HEX); Serial.print(" ");
  }
  Serial.println(' ');
*/
  zbTx.setPayload(data);
  zbTx.setPayloadLength(size);
  zbTx.setAddress16(0xFFFE);

  xbee.send(zbTx);

  ERROR_BLINK(1);

  return true;
}
/////////





////////////////////////

void process_command()
{
  // ERROR_BLINK(3);

  if (command.command == ECommand_BEACON)
  {
    addr64.setMsb(0);
    addr64.setLsb(0xFFFF);
  }
  else
  {

    addr64.setLsb(command.node_id.lsb);
    addr64.setMsb(command.node_id.msb);
  }

  zbTx.setAddress64(addr64);

/*
  reset_buffer_print();
  bufferPrint.print("PC: ");
  bufferPrint.print("C "); bufferPrint.print(command.command);
  bufferPrint.print(", N "); bufferPrint.print(command.number);
  bufferPrint.print(", lsb "); bufferPrint.print(command.node_id.lsb);
  bufferPrint.print(", msb "); bufferPrint.print(command.node_id.msb);
  DBGP((char const*)bufferPrint.buf);
*/

  send_package(SimpleCommand_fields, &command, packet_xb_writer);
  // send_package(SimpleCommand_fields, &command, packet_stream_writer);
}

void process_position_notify()
{
  addr64.setLsb(pos_notify.node_id.lsb);
  addr64.setMsb(pos_notify.node_id.msb);
  zbTx.setAddress64(addr64);
  send_package(PositionNotify_fields, &pos_notify, packet_xb_writer);
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
  else if  (PositionNotify_fields == type)
  {
    status = decode_unionmessage_contents(isrt, PositionNotify_fields, &pos_notify);
    if (status)
      process_position_notify();
  }
  else
  {
    // Serial.println("Unknown package");
    //ERROR_BLINK(9);
  }

  if (!status)
  {
    reset_buffer_print();
    bufferPrint.print("proc_inc_pkg err "); bufferPrint.println(PB_GET_ERROR(isrt));
    DBGP((char const*)bufferPrint.buf);
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

void process_ZB_TX_STATUS_RESPONSE()
{
  ZBTxStatusResponse txsr;
  xbee.getResponse().getZBTxStatusResponse(txsr);

  reset_buffer_print();
  bufferPrint.print("ra "); bufferPrint.println(txsr.getRemoteAddress(), HEX);
  bufferPrint.print("ds "); bufferPrint.println(txsr.getDeliveryStatus(), HEX);
  bufferPrint.print("di "); bufferPrint.println(txsr.getDiscoveryStatus(), HEX);
  bufferPrint.print("rc "); bufferPrint.println(txsr.getTxRetryCount(), HEX);

  DBGP((char const*)bufferPrint.buf);
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
      // ERROR_BLINK(1);
    }
    else
    {
      if (Serial.available() > 0)
      {
        if (Serial.readBytes((char*)&size, 1) == 1)
        {
          // DBGP("in package");

          in_package = true;
        }
      }
    }
  }
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

// void get_our_address()
// {
//   DEBUG_PRINTLN("get_our_address");

//   while (!at_command("SL"))
//   { }
//   //memcpy(&this_node.lsb, atResponse.getValue(), 4);
//   //DEBUG_PRINTLN2("value len ", atResponse.getValueLength());

//   for (int i = 0; i < 4; ++i)
//     ((uint8_t*)(&this_node.lsb))[i] = atResponse.getValue()[3 - i];

//   while (!at_command("SH"))
//   { }
//   //memcpy(&this_node.msb, atResponse.getValue(), 4);
//   //DEBUG_PRINTLN2("value len ", atResponse.getValueLength());
//   for (int i = 0; i < 4; ++i)
//     ((uint8_t*)(&this_node.msb))[i] = atResponse.getValue()[3 - i];

//   DEBUG_PRINTLN2H("this_node.msb", this_node.msb);
//   DEBUG_PRINTLN2H("this_node.lsb", this_node.lsb);
// }

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

  Serial.begin(57600);

  XBEE_SERIAL.begin(9600);
  xbee.begin(XBEE_SERIAL);


  while (!Serial);

  // ERROR_BLINK(5);

  DBGP("Greetings!");

  initial_xb_setup();

  // get_our_address();
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
