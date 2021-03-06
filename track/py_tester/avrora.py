import select
import socket
import sys

from pprint import pprint

from common import ProtocolTester

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', 2390))

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

pt = ProtocolTester(serial_read, serial_write)


while True:
	read = [sys.stdin, s]
	# read = [sys.stdin, ]

	rrrr = select.select(read, [], [], 0.0)[0]

	for r in rrrr:
		pprint(r)
		if r == sys.stdin:
			pt.proc_stdin()
		elif r == s:
			pt.proc_device_input()
