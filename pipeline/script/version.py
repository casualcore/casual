#! /usr/bin/env python3

import sys
import re
import subprocess

def retrieve_tags(base_version):
   command = ['git','tag','--list']
   command.append( base_version + '*')
   
   call = subprocess.run( command, stdout=subprocess.PIPE)

   alternatives = call.stdout.decode().split()

   return alternatives

def normal():
   return "normal"

def assemble_filter(base_version, version_type):
   match_string = "^" + base_version
   if version_type == normal():
      match_string += "\."
   else:
      match_string += "-" + version_type
   
   match_string += ".*"

   return match_string

def extract_current_version(base_version, version_type = normal()):

   matcher = re.compile("^.*\.(.*)$")

   def sorter( item):
      found = matcher.match( item)
      if found:
         return int( found.group(1))

   match_string = assemble_filter(base_version, version_type)

   r = re.compile(match_string)

   version = retrieve_tags( base_version)

   answer = list(filter( r.match, version))

   sort_answer = sorted( answer, reverse = True, key = sorter)

   if sort_answer:
      return base_version, version_type, sort_answer[0]

   return base_version, version_type, None

def generate_new_version( current_version, base_version, version_type):

   if not current_version:
      if version_type == normal():
         return base_version + ".0"
      else:
         return base_version + "-" + version_type + ".0"

   r = re.compile("^(.*\.)([0-9]+)$")

   found = r.match( current_version)

   if found:
      return found.group(1) + str( int( found.group(2)) + 1)

   raise SystemError("Could not generate new number")


def main():

   if len (sys.argv) < 2:
      print( "usage: {} baseversion [alpha|beta]".format(sys.argv[0]))
      raise SystemExit(1)

   base_version = sys.argv[1]

   if len (sys.argv) == 3:
      version_type = sys.argv[2]
   else:
      version_type = normal()

   dummy, dummy, current_version = extract_current_version( base_version, version_type)

   new_version = generate_new_version(current_version, base_version, version_type)

   print( new_version)

if __name__ == '__main__':
   main()