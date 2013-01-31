# Based on @berenm's pull request https://github.com/quarnster/SublimeClang/pull/135
# Create the database with cmake with for example: cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
# or you could have set(CMAKE_EXPORT_COMPILE_COMMANDS ON) in your CMakeLists.txt
# Usage within SublimeClang:
#   "sublimeclang_options_script": "python ${home}/code/cmake_options_script.py ",

# 2013.01.26: Pavlo Ilin
# - Removed the usage of cache file - if cmake-scripts are changed often,
# cache file quickly becomes old and wrong.
# - Will recursively go up in directory tree and try to find compile_commands.json
# using predefined search patterns (dbfile_search_pattern)
# - Will try to guess options for files which are not listed in compile_commands.json
# Those files are, mostly, header files. See guess_option()

# 2012.01.31: Pavlo Ilin
# - Support for arduino-cmake

import re
import os
import os.path
import sys
import json
import platform
from pprint import pprint

compilation_database_options_pattern = re.compile('(?<=\s)-[DIOUWfgs][^=\s]+(?:=\\"[^"]+\\"|=[^"]\S+)?')
compilation_database_additional_options_pattern = re.compile('(?<=\s)-[m][^=\s]+(?:=\\"[^"]+\\"|=[^"]\S+)?')
compilation_database_command_pattern = re.compile(r'(.+?)\s+-[DIOUWfgs]')

dbfile_search_patterns = [r'/build/compile_commands.json',
                          r'/debug/compile_commands.json',
                          r'/release/compile_commands.json']

def find_dffile(path):

    for dbfile_pat in dbfile_search_patterns:
        dbfile = path + dbfile_pat
        if os.path.isfile(dbfile):
            return dbfile

    # not found
    newpath = os.path.dirname(path)
    if path == newpath:
        raise Exception(r'Unable to find ' + dbfile_pat)
    return find_dffile(newpath)

def load_db(filename):
    compilation_database = {}
    with open(filename) as compilation_database_file:
        compilation_database_entries = json.load(compilation_database_file)

    entry = 0



    for compilation_entry in compilation_database_entries:
        entry = entry + 1
        o = {}
        o['flags'] = [ p.strip() for p in compilation_database_options_pattern.findall(compilation_entry["command"]) ]
        o['additional'] = [ p.strip() for p in compilation_database_additional_options_pattern.findall(compilation_entry["command"]) ]
        o['command'] = compilation_database_command_pattern.match(compilation_entry["command"]).group(1)
        compilation_database[compilation_entry["file"]] = o


    return compilation_database

def guess_file(filename, db):
    # possible source file names
    name, ext = os.path.splitext(filename)
    guesses = [name + '.cpp', name + '.c']

    # possible main source file
    for key in db.iterkeys():
        if key.endswith('main.cpp') or key.endswith('main.c'):
            guesses.append(key)
            break

    # copy options from existing possible file
    for g in guesses:
        if os.path.isfile(g):
            db[filename] = db[g]
            break

    return db

def guess_additional_options(filename, options):

    #pprint(options)

    # c++11 on mac os x
    if platform.mac_ver()[0] != '':
        if any([r'-std=c++11' in s for s in options['flags']]) and any([r'-stdlib=libc++' in s for s in options['flags']]):
            options['flags'].append(r'-I/usr/lib/c++/v1')

    # arduino
    if r'tools/avr/bin' in options['command']:
        # recover avr-gcc system include path
        avr = options['command']   # .../avr/bin/avr-gcc
        avr = os.path.dirname(avr) # .../avr/bin
        avr = os.path.dirname(avr) # .../avr
        options['flags'].append("-I" + avr + r'/avr-4/include/')
        options['flags'].append("-I" + avr + r'/lib/gcc/avr/4.3.2/include')

        # recover avr type from linker flags
        if 'additional' in options:
            recpu = re.compile(r"-mmcu=atmega(.+)")
            for o in options['additional']:
                m = recpu.match(o)
                if m:
                    mm = m.group(1)
                    options['flags'].append("-D__AVR_ATmega{0}__".format(mm.upper()))

        # add few options to make clang happy
        options['flags'].append("-Wno-attributes")
        options['flags'].append("-Wno-builtin-requires-header")
        options['flags'].append("-Wno-unused-command-line-argument")


    return options



srcfile = sys.argv[1]
dbfile = find_dffile(os.path.dirname(srcfile))
dbpath = os.path.dirname(dbfile)

db = load_db(dbfile)

#pprint(db)

if db:
    file_options = {'flags':"", 'command':""}

    if not srcfile in db:
        db = guess_file(srcfile, db)

    if srcfile in db:
        file_options = db[srcfile]


    file_options = guess_additional_options(srcfile, file_options)

    for option in file_options['flags']:
        print option