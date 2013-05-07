import select
import serial
import sys

from pprint import pprint

from common import ProtocolTester

host_node_tty = '/dev/tty.usbmodemfa1311'
ser = serial.Serial(host_node_tty, 57600)

def serial_read(size = 1):
	return ser.read(size)

def serial_write(data):
	ser.write(data)


pt = ProtocolTester(serial_read, serial_write)


while True:
	read = [sys.stdin, ser]
	# read = [sys.stdin, ]

	rrrr = select.select(read, [], [], 0.0)[0]

	for r in rrrr:
		pprint(r)
		if r == sys.stdin:
			pt.proc_stdin()
		elif r == ser:
			pt.proc_device_input()
