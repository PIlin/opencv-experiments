
from pprint import pprint

import simple_command_pb2 as scp
import serial

# import logging

from google.protobuf.text_format import MessageToString

host_node_tty = '/dev/tty.usbmodemfa1311'

ser = serial.Serial(host_node_tty, 57600)

def byte_from_string(s, pos):
	return ord(s[pos])

def serial_read(size = 1):
	return ser.read(size)

def get_message():
	size = byte_from_string(serial_read(1), 0)
	data = serial_read(size)
	# pprint(data)

	return decode_message(data)

def decode_message(data):
	msg = scp.MessagePackage()

	msg.ParseFromString(data)

	print('<<<<<<<<<')
	print(MessageToString(msg))


def send_message(msg):
	data = msg.SerializeToString()
	size = bytearray([len(data), ])

	ser.write(size)
	ser.write(data)

	print('>>>>>>>>>')
	pprint(bytearray(data))
	print(MessageToString(msg))




def send_beacon_req():
	msg = scp.MessagePackage()

	msg.simple_command.node_id.addr = 0
	msg.simple_command.command = scp.BEACON
	msg.simple_command.number = 0

	send_message(msg)


send_beacon_req()

# m = bytearray([0x22,0x1A,0x0A,0x18,0x70,0x61,0x63,0x6B,0x65,0x74])
# decode_message(str(m))



while True:
	msg = get_message()