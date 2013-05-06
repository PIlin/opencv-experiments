import select
import socket
import sys

from pprint import pprint

import simple_command_pb2 as scp
from google.protobuf.text_format import MessageToString

def byte_from_string(s, pos):
	return ord(s[pos])

def serial_read(size = 1):
	buf = str()
	while len(buf) < size:
		d = s.recv(size - len(buf))
		if not d:
			print('no data from socket')
			sys.exit()
		buf = buf + d
	return buf

def serial_write(data):
	s.sendall(data)

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

	serial_write(size)
	serial_write(data)

	print('>>>>>>>>>')
	pprint(bytearray(data))
	print(MessageToString(msg))


def send_beacon_req():
	msg = scp.MessagePackage()

	msg.simple_command.node_id.addr = 0
	msg.simple_command.command = scp.BEACON
	msg.simple_command.number = 0

	send_message(msg)




def proc_stdin():
	d = sys.stdin.readline()
	print(d)

	if d.startswith('b'):
		send_beacon_req()

def proc_sock():
	msg = get_message()



s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', 2390))

while True:
	read = [sys.stdin, s]
	# read = [sys.stdin, ]

	rrrr = select.select(read, [], [], 0.0)[0]

	for r in rrrr:
		pprint(r)
		if r == sys.stdin:
			proc_stdin()
		elif r == s:
			proc_sock()