#! /usr/bin/env python3

import sys
import re
import subprocess

matcher = re.compile("^.*\.(.*)$")

def retrieve_tags(base_version):
   command = ['git','tag','--list']
   command.append( base_version + '*')
   
   call = subprocess.run( command, stdout=subprocess.PIPE)

   alternatives = call.stdout.decode().split()

   return alternatives

def normal():
   return "normal"

def beta():
   return "beta"

def alpha():
   return "alpha"

def assemble_filter(base_version, version_type):
   match_string = "^" + base_version
   if version_type == normal():
      match_string += "\.[0-9]+$"
      return match_string
   else:
      match_string += "-" + version_type
   
   match_string += ".*"

   return match_string

def retrieve_latest( r, versions):

   def sorter( item):
      found = matcher.match( item)
      if found:
         return int( found.group(1))

   answer = list(filter( r.match, versions))

   sort_answer = sorted( answer, reverse = True, key = sorter)

   if sort_answer:
      return sort_answer[0]

def extract_current_release_version(base_version, versions):

   match_string = assemble_filter(base_version, normal())

   version_finder = re.compile(match_string)

   return retrieve_latest( version_finder, versions)

def generate_new_version( current_release_version, base_version, version_type, versions):

   version_to_change_finder = re.compile("^(.*\.)([0-9]+)$")

   if not current_release_version:
      next_release_version =  base_version + ".0"

   else:
      found = version_to_change_finder.match( current_release_version)

      if found:
         next_release_version = found.group(1) + str( int( found.group(2)) + 1)
      else:
         raise SystemError("Could not generate new number")

   if version_type == normal():
      return next_release_version

   if version_type == beta():
      version_finder = re.compile("^.*-beta\.([0-9]+)$")
      latest = retrieve_latest( version_finder, versions)
      found = version_to_change_finder.match( latest)

      if found:
         return next_release_version + "-beta." + str( int( found.group(2)) + 1)
      else:
         raise SystemError("Could not generate new number")

   if version_type == alpha():
      return next_release_version + "-alpha."

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

   versions = retrieve_tags( base_version)

   current_release_version = extract_current_release_version( base_version, versions)

   new_version = generate_new_version( current_release_version, base_version, version_type, versions)

   print( new_version)

if __name__ == '__main__':
   main()