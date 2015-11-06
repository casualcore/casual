#! /opt/local/bin/python
import sys

from casual.server.buffer import JsonBuffer
from casual.server.api import *

def py_service_test( buffer):
   
    reply = JsonBuffer("{'kalle' : 'anka'}")
    
    casual_return( 0, reply)

def py_service_echo( buffer):

    reply = JsonBuffer(buffer.contents.data)
    
    casual_return( 0, reply)

def main():
    
    server = Server()
    server.services.append( Service("py_service_test", py_service_test))
    server.services.append( Service("py_service_echo", py_service_echo))
    
    server.start()             


if __name__ == '__main__':
    main()