
#include "Arduino.h"

#define DEBUG 1

#include "debug.h"

#include "protocol.hpp"
#include "simple_command.pb.h"
#include <pb_decode.h>
#include <pb_encode.h>

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

#define RED_LED 8
#define GREEN_LED 9
#define BLUE_LED 10

#else
static const int pin_reset = 5;
static const int pin_cs = 7;
static const int pin_interrupt = 2;

// cannot use ledPin beacuse it is SCK
// static const int ledPin = 13;

#define RED_LED 8
#define GREEN_LED 9
#define BLUE_LED 10

#endif


/////////

struct ColorRect
{
  uint8_t l;
  uint8_t r;
  uint8_t t;
  uint8_t b;
  uint8_t color;
};

static ColorRect colors[] = {
  {0, 85, 0, 127, 4},
  {85, 170, 0, 127, 7},
  {170, 255, 0, 127, 1},
  {0, 85, 128, 255, 3},
  {85, 170, 128, 255, 2},
  {170, 255, 128, 255, 6},
};

static bool current_enabled = false;
static uint8_t current_color = 0;



static void set_led(bool on)
{
#define ON(color) ((current_color & color) ? LOW : HIGH)
#define OFF(color) HIGH
#define SWITCH(on, color) (on ? ON(color) : OFF(color))

  DEBUG_PRINTLN2("set_led ", on);
  DEBUG_PRINTLN2("color ", current_color);

  current_enabled = on;
  digitalWrite(RED_LED, SWITCH(on, 4));
  digitalWrite(GREEN_LED, SWITCH(on, 2));
  digitalWrite(BLUE_LED, SWITCH(on, 1));


#undef SWITCH
#undef OFF
#undef ON
}

//////////

static SimpleCommand command;
static SimpleAnswer answer;
static PositionNotify pos_notify;

/////////


#define PAN_ID 0xB41C
#define HOST_ADDR 0x0001
#define BROADCAST_ADDR 0xFFFF

#define THIS_NODE (ARDUINO_NODE_ID + 0x09)

static Mrf24j mrf(pin_reset, pin_cs, pin_interrupt);

static uint16_t send_dst_addr = 0;

static bool sent = false;
static bool sent_ok = false;

static void process_mrf_packets();

///

// struct NodeID
// {
//   uint32_t msb;
//   uint32_t lsb;
// };

// static NodeID  this_node;


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

  sent = false;
  sent_ok = false;

  const char* pd = (char*)data;
  mrf.send16(send_dst_addr, pd, size);

  unsigned long now = millis();
  bool timeout = false;
  do
  {
    DEBUG_PRINTLN("packet_xb_writer waiting");
    process_mrf_packets();
  } while (!(sent || (timeout = (millis() - now > 3000))));

  DEBUG_PRINTLN("packet_xb_writer done");

  if (timeout)
  {
    DEBUG_PRINTLN("was timeout");
  }

  return sent_ok;
}
/////////


#define DBGP(x) debug_print(x, packet_stream_writer)


////////////////////////


void send_beacon()
{
  DEBUG_PRINTLN("send_beacon");
  answer.node_id.addr = THIS_NODE;
  answer.command = ECommand_BEACON;
  answer.answer = 0;
  answer.number = command.number;



  send_package(SimpleAnswer_fields, &answer, packet_xb_writer);
}

//

void process_command()
{
  DEBUG_PRINTLN("process_command");
  DEBUG_PRINTLN2("command ", command.command);
  DEBUG_PRINTLN2H("com.addr  ", command.node_id.addr);
  DEBUG_PRINTLN2H("this.addr ", THIS_NODE);

  answer.node_id.addr = THIS_NODE;
  answer.command = command.command;
  answer.number = command.number;
  answer.answer = 0;

  if (command.node_id.addr == THIS_NODE ||
    command.node_id.addr == 0)
  {

    switch(command.command)
    {
      case ECommand_LIGHT_ON:
      {
        DEBUG_PRINTLN("ECommand_LIGHT_ON");
        //digitalWrite(ledPin, HIGH);
        // digitalWrite(ledPin, LOW);
        current_color = 7;
        set_led(true);

        answer.answer = 1;

        break;
      }
      case ECommand_LIGHT_OFF:
      {
        DEBUG_PRINTLN("ECommand_LIGHT_OFF");
        // digitalWrite(ledPin, LOW);
        // digitalWrite(ledPin, HIGH);
        set_led(false);


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

void process_position_notify()
{
  DEBUG_PRINTLN("process_position_notify");
  DEBUG_PRINTLN2H("pos_notify.addr ", pos_notify.node_id.addr);
  DEBUG_PRINTLN2("pos_notify.x ", pos_notify.x);
  DEBUG_PRINTLN2("pos_notify.y ", pos_notify.y);

  uint8_t x = pos_notify.x;
  uint8_t y = pos_notify.y;

  for (uint8_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i)
  {
    ColorRect& r = colors[i];
    if (r.l <= x && x <= r.r &&
        r.t <= y && y <= r.b)
    {
      current_color = r.color;
      set_led(current_enabled);
      DEBUG_PRINTLN2("set color ", current_color);
      break;
    }
  }
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
  else if  (PositionNotify_fields == type)
  {
    status = decode_unionmessage_contents(isrt, PositionNotify_fields, &pos_notify);
    if (status)
      process_position_notify();
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

void process_ZB_RX_RESPONSE()
{
  uint8_t* data = mrf.get_rxinfo()->rx_data;
  uint8_t const length = mrf.rx_datalength();

  DEBUG_PRINTLN2("process_ZB_RX_RESPONSE length = ", length);
  for (int i =0;i < length; ++i)
  {
    Serial.print(data[i], HEX);
    Serial.print(' ');
  }
  DEBUG_PRINTLN(' ');

  send_dst_addr = mrf.get_rxinfo()->src_addr16;

  pb_istream_t istream = pb_istream_from_buffer(data, length);

  process_incoming_packet(&istream);
}

void process_ZB_TX_STATUS_RESPONSE()
{
  DEBUG_PRINTLN("process_ZB_TX_STATUS_RESPONSE");

  sent = true;
  sent_ok = mrf.get_txinfo()->tx_ok;

  DEBUG_PRINTLN2("tx ", sent_ok ? "ok" : "err");
  DEBUG_PRINTLN2H("rc ", mrf.get_txinfo()->retries);
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

static void mrf_interrupt(void)
{
  mrf.interrupt_handler();
}

static void setup_mrf(void)
{
  // Serial.println("setup mrf node " xstr(ARDUINO_NODE_ID));

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

  Serial.println("done setup mrf");
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
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  current_color = 0;
  set_led(true);
  set_led(false);

  Serial.begin(57600);

  setup_mrf();
  check_mrf();

  // while (!Serial);

  DEBUG_PRINTLN("Greetings!");

  // DBGP("Greetings!");
}

void loop()
{
  process_mrf_packets();
}
