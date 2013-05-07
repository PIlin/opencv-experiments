import sys

from pprint import pprint

import simple_command_pb2 as scp
from google.protobuf.text_format import MessageToString

def byte_from_string(s, pos):
	return ord(s[pos])

class ProtocolTester:
	def __init__(self, read_func, write_func):
		self.read_func = read_func
		self.write_func = write_func

	def get_message(self):
		size = byte_from_string(self.read_func(1), 0)
		data = self.read_func(size)
		pprint(data)

		return self.decode_message(data)

	def decode_message(self, data):
		msg = scp.MessagePackage()

		msg.ParseFromString(data)

		print('<<<<<<<<<')
		print(MessageToString(msg))

	def send_message(self, msg):
		data = msg.SerializeToString()
		size = bytearray([len(data), ])

		self.write_func(size)
		self.write_func(data)

		print('>>>>>>>>>')
		pprint(bytearray(data))
		print(MessageToString(msg))

	def send_beacon_req(self):
		msg = scp.MessagePackage()

		msg.simple_command.node_id.addr = 0
		msg.simple_command.command = scp.BEACON
		msg.simple_command.number = 0

		self.send_message(msg)

	def proc_stdin(self):
		d = sys.stdin.readline()
		print(d)

		if d.startswith('b'):
			self.send_beacon_req()

	def proc_device_input(self):
		self.get_message()