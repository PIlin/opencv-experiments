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
		self.number = 0

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

		self.number += 1

	def send_beacon_req(self):
		msg = scp.MessagePackage()

		msg.simple_command.node_id.addr = 0
		msg.simple_command.command = scp.BEACON
		msg.simple_command.number = self.number

		self.send_message(msg)

	def send_led_req(self, is_on):
		msg = scp.MessagePackage()

		msg.simple_command.node_id.addr = 11
		msg.simple_command.command = scp.LIGHT_ON if is_on else scp.LIGHT_OFF
		msg.simple_command.number = self.number

		self.send_message(msg)

	def send_pos_notify(self, x, y):
		msg = scp.MessagePackage()

		msg.position_notify.node_id.addr = 11
		msg.position_notify.number = self.number
		msg.position_notify.x = x
		msg.position_notify.y = y

		self.send_message(msg)

	def proc_stdin(self):
		d = sys.stdin.readline()

		comm = d.split()
		pprint(comm)

		if not comm:
			return

		if comm[0] == 'b':
			self.send_beacon_req()
		elif comm[0] == 'q':
			self.send_led_req(True)
		elif comm[0] == 'w':
			self.send_led_req(False)
		elif comm[0] == 'p':
			if len(comm) < 3:
				print('enter coordinates x y')
				return
			try:
				x = int(comm[1], base=0)
				y = int(comm[2], base=0)
				self.send_pos_notify(x, y)
			except ValueError:
				print('wrong coordinates')
		else:
			print('wrong command')

	def proc_device_input(self):
		self.get_message()