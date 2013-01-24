# Based on @berenm's pull request https://github.com/quarnster/SublimeClang/pull/135
# Create the database with cmake with for example: cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
# or you could have set(CMAKE_EXPORT_COMPILE_COMMANDS ON) in your CMakeLists.txt
# Usage within SublimeClang:
#   "sublimeclang_options_script": "python ${home}/code/cmake_options_script.py ",

# will search recursively back from orut file


import re
import os
import os.path
import pickle
import sys
import json
from pprint import pprint

compilation_database_pattern = re.compile('(?<=\s)-[DIOUWfgs][^=\s]+(?:=\\"[^"]+\\"|=[^"]\S+)?')

def find_dffile(path):
    dbfile_pat = r'/debug/compile_commands.json'
    dbfile = path + dbfile_pat
    if os.path.isfile(dbfile):
        return dbfile
    else:
        newpath = os.path.dirname(path)
        if path == newpath:
            raise Exception(r'Unable to find ' + dbfile_pat)
        return find_dffile(newpath)

def load_db(filename):
    compilation_database = {}
    with open(filename) as compilation_database_file:
        compilation_database_entries = json.load(compilation_database_file)

    total = len(compilation_database_entries)
    entry = 0
    for compilation_entry in compilation_database_entries:
        entry = entry + 1
        compilation_database[compilation_entry["file"]] = [ p.strip() for p in compilation_database_pattern.findall(compilation_entry["command"]) ]
    return compilation_database

def guess_option(filename, db):
    guessed = []

    name, ext = os.path.splitext(filename)
    guesses = [name + '.cpp', name + '.c']

    for key in db.iterkeys():
        if key.endswith('main.cpp'):
            guesses.append(key)
            break

    for g in guesses:
        if os.path.isfile(g):
            # print ('guessed = ', g)
            for opt in db[g]:
                guessed.append(opt)
            break

    return guessed



#scriptpath = os.path.dirname(os.path.abspath(sys.argv[1]))
srcfile = sys.argv[1]
dbfile = find_dffile(os.path.dirname(srcfile))
dbpath = os.path.dirname(dbfile)
cache_file = "%s/cached_options.txt" % (dbpath)


db = None
# if os.access(cache_file, os.R_OK) == 0:
db = load_db(dbfile)
    # f = open(cache_file, "wb")
    # pickle.dump(db, f)
    # f.close()
# else:
#     f = open(cache_file)
#     db = pickle.load(f)
#     f.close()



if db:
    if srcfile in db:
        for option in db[srcfile]:
            print option
    else:
        for option in guess_option(srcfile, db):
            print option