import pickle
# from pprint import pprint
import os
import sys
import re

def find_dffile(path):
	dbfile = path + r'/.build/environment.pickle'
	if os.path.isfile(dbfile):
		return dbfile
	else:
		newpath = os.path.dirname(path)
		if path == newpath:
			raise Exception('Unable to find .build/environment.pickle')
		return find_dffile(newpath)


def load_db(filename):
	db = {}
	with open(filename) as dbfile:
		l = pickle.load(dbfile)
		for r in l:
			db[r[0]] = r[1]
			pass

		pass

	# pprint(db)

	return db


def parse_db(db):
	opts = []
	opts.extend(db['cflags'])
	opts.extend(db['cxxflags'])

	avr = db['cc']               # .../avr/bin/avr-gcc
	avr = os.path.dirname(avr) # .../avr/bin
	avr = os.path.dirname(avr) # .../avr

	opts.append("-I" + avr + r'/avr-4/include/')
	opts.append("-I" + avr + r'/lib/gcc/avr/4.3.2/include')


	recpu = re.compile(r"-mmcu=atmega(.+)")
	for o in opts:
		m = recpu.match(o)
		if m:
			mm = m.group(1)
			opts.append("-D__AVR_ATmega{0}__".format(mm.upper()))


	return opts

def file_specific(srcfile):
	ext = os.path.splitext(srcfile)[1]

	opts = []
	if ext == '.ino':
		opts.append('-includeArduino.h')

	return opts




srcfile = sys.argv[1]

dbpath = find_dffile(os.path.dirname(srcfile))
db = load_db(dbpath)

for line in parse_db(db):
	print line

for line in file_specific(srcfile):
	print line