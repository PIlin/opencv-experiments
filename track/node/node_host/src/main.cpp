
#include "Arduino.h"

#define DEBUG 2

#include "debug.h"

#include "protocol.hpp"
#include "simple_command.pb.h"

#include "mrf24j.h"


#define DEBUG_SHORT(x) do { DEBUG_PRINTLN2H(#x " ", mrf.read_short(x)); } while (0)
#define DEBUG_LONG(x) do { DEBUG_PRINTLN2H(#x " ", mrf.read_long(x)); } while (0)

///////
// wiring

#ifdef ARDUINO_BOARD_leonardo
static const int pin_reset = 5;
static const int pin_cs = 7;
static const int pin_interrupt = 3;

static const int ledPin = 13;

#else
static const int pin_reset = 5;
static const int pin_cs = 7;
static const int pin_interrupt = 2;

// cannot use ledPin beacuse it is SCK
// static const int ledPin = 13;

#endif


#define MAIN_SERIAL Serial
// #define MAIN_SERIAL Serial1

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

#ifdef ARDUINO_BOARD_leonardo

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

#else
void blink(uint8_t, unsigned long) {}
#endif

#define ERROR_BLINK(count) blink(count, 100)
////

#define PAN_ID 0xB41C
#define HOST_ADDR 0x0001
#define BROADCAST_ADDR 0xFFFF

#define THIS_NODE HOST_ADDR

static Mrf24j mrf(pin_reset, pin_cs, pin_interrupt);

static uint16_t send_dst_addr = 0;

static bool sent = false;
static bool sent_ok = false;

static void process_mrf_packets();

bool packet_stream_writer(uint8_t* data, uint8_t size);

////////////////////////

void DBGP(char const* x)
{
  debug_print(x, packet_stream_writer);
}

void DBGP_BUF(void const* x, uint8_t size)
{
  reset_buffer_print();
  uint8_t const* data = (uint8_t const*)x;
  for (int i = 0; i < size; ++i)
  {
    bufferPrint.print(data[i], HEX); bufferPrint.print(" ");
  }
  DBGP((char const*)bufferPrint.buf);

}


////////////////////////

static bool packet_stream_reader(pb_istream_t* stream, uint8_t* buf, size_t count)
{
  if (buf)
  {
    if (MAIN_SERIAL.readBytes((char*)buf, count) < count)
    {
      return false;
    }

    // DBGP_BUF(buf, count);
  }
  else
  {
    char c;
    while (count--)
    {
      if (MAIN_SERIAL.readBytes(&c, 1) < 1)
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


  if (MAIN_SERIAL.write(size) != 1 ||
    MAIN_SERIAL.write(data, size) != size)
  {

    return false;
  }

  return true;
}




bool packet_xb_writer(uint8_t* data, uint8_t size)
{

/*
  MAIN_SERIAL.print("packet_xb_writer size "); MAIN_SERIAL.println(size);
  MAIN_SERIAL.print("packet_xb_writer data "); MAIN_SERIAL.println((size_t)data, HEX);
  for (int i = 0; i < size; ++i)
  {
    MAIN_SERIAL.print(data[i], HEX); MAIN_SERIAL.print(" ");
  }
  MAIN_SERIAL.println(' ');
*/

  const char* pd = (char*)data;
  sent = false;
  sent_ok = false;
  mrf.send16(send_dst_addr, pd, size);

  while (!sent)
    process_mrf_packets();

  DEBUG_PRINT("packet_xb_writer done");

  return sent_ok;
}
/////////





////////////////////////

void process_command()
{
  // ERROR_BLINK(3);

  if (command.command == ECommand_BEACON)
  {
    // addr64.setMsb(0);
    // addr64.setLsb(0xFFFF);
    send_dst_addr = BROADCAST_ADDR;
    DBGP("got beacon");
  }
  else
  {
    // addr64.setLsb(command.node_id.lsb);
    // addr64.setMsb(command.node_id.msb);

    send_dst_addr = command.node_id.addr;
  }

  // zbTx.setAddress64(addr64);


  reset_buffer_print();
  bufferPrint.print("PC: ");
  bufferPrint.print("C "); bufferPrint.print(command.command);
  bufferPrint.print(", N "); bufferPrint.print(command.number);
  bufferPrint.print(", addr "); bufferPrint.print(command.node_id.addr);
  DBGP((char const*)bufferPrint.buf);


  send_package(SimpleCommand_fields, &command, packet_xb_writer);
  // send_package(SimpleCommand_fields, &command, packet_stream_writer);
}

void process_position_notify()
{
  // addr64.setLsb(pos_notify.node_id.lsb);
  // addr64.setMsb(pos_notify.node_id.msb);
  // zbTx.setAddress64(addr64);
  send_dst_addr = pos_notify.node_id.addr;
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
    // MAIN_SERIAL.println("Unknown package");
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

static void process_ZB_RX_RESPONSE()
{
  uint8_t* data = mrf.get_rxinfo()->rx_data;
  uint8_t const length = mrf.rx_datalength();

  pb_istream_t istream = pb_istream_from_buffer(data, length);

  process_incoming_packet(&istream);
}

static void process_ZB_TX_STATUS_RESPONSE()
{
  reset_buffer_print();

  sent = true;
  sent_ok = mrf.get_txinfo()->tx_ok;

  bufferPrint.print("tx "); bufferPrint.println(sent_ok ? "ok" : "err");
  bufferPrint.print("rc "); bufferPrint.println(mrf.get_txinfo()->retries);

  DBGP((char const*)bufferPrint.buf);
}


static void handle_rx(void)
{
  process_ZB_RX_RESPONSE();
}

static void handle_tx(void)
{
  process_ZB_TX_STATUS_RESPONSE();
}

static void process_mrf_packets()
{
  mrf.check_flags(&handle_rx, &handle_tx);
}

///




void process_serial_package()
{
  static uint8_t size = 0;
  static bool in_package = false;

  while (MAIN_SERIAL.available() > 0)
  {
    if (in_package)
    {
      istream_serial.bytes_left = size;

      process_incoming_packet(&istream_serial);

      if (istream_serial.bytes_left > 0)
      {
        // MAIN_SERIAL.println("Bytes left");
        while (istream_serial.bytes_left--)
          MAIN_SERIAL.read();
      }

      in_package = false;
      // ERROR_BLINK(1);
    }
    else
    {
      if (MAIN_SERIAL.available() > 0)
      {
        if (MAIN_SERIAL.readBytes((char*)&size, 1) == 1)
        {
          // DBGP("in package");

          in_package = true;
        }
      }
    }
  }
}

///

static void mrf_interrupt(void)
{
  mrf.interrupt_handler();
}

static void setup_mrf(void)
{
  // MAIN_SERIAL.println("setup mrf node " xstr(ARDUINO_NODE_ID));

  //SPI.setClockDivider(B00000001); // spi speed

  mrf.reset();
  mrf.init();

  mrf.set_pan(0xabba);
  mrf.address16_write(THIS_NODE);

  //mrf.set_promiscuous(true);

  //mrf.write_short(MRF_RXMCR, 0x02); // error mode

  attachInterrupt(0, mrf_interrupt, CHANGE);
  interrupts();

  delay(1000);

  DBGP("done setup mrf");
}

void check_mrf(void)
{
  DEBUG_PRINTLN("check_mrf");
  int pan = mrf.get_pan();
  DEBUG_PRINTLN2H("pan ", pan);

  int addr = mrf.address16_read();
  DEBUG_PRINTLN2H("addr ", addr);

  DEBUG_SHORT(MRF_PACON2);
  DEBUG_SHORT(MRF_TXSTBL);

  DEBUG_LONG(MRF_RFCON0);
  DEBUG_LONG(MRF_RFCON1);
  DEBUG_LONG(MRF_RFCON2);
  DEBUG_LONG(MRF_RFCON6);
  DEBUG_LONG(MRF_RFCON7);
  DEBUG_LONG(MRF_RFCON8);
  DEBUG_LONG(MRF_SLPCON1);

  DEBUG_SHORT(MRF_BBREG2);
  DEBUG_SHORT(MRF_CCAEDTH);
  DEBUG_SHORT(MRF_BBREG6);

  DEBUG_PRINTLN("done check_mrf");
}

void setup()
{
  pinMode(ledPin, OUTPUT);

  MAIN_SERIAL.begin(57600);

  setup_mrf();

  while (!MAIN_SERIAL);

  // ERROR_BLINK(5);

  DBGP("Greetings!");

  check_mrf();
}

void loop()
{
  process_serial_package();
  process_mrf_packets();

  static unsigned int prev = millis();
  unsigned int now = millis();
  if (now - prev >= 10000)
  {
    DBGP("alive");
    prev = now;
  }
}
