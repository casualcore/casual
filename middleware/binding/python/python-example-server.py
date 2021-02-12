#!/usr/bin/env python3
import sys

from casual.server.api import FAIL, SUCCESS, Service, casual_return,start_server
from casual.server.buffer import JsonBuffer
from casual.server.log import userlog


def py_service_uppercase( buffer):
    """
    Implements uppercase
    """

    state = SUCCESS
    reply = None

    try:
        data = buffer.contents.data[0:buffer.contents.len]
        userlog(data)
        reply = JsonBuffer(data.upper())
    except:
        state = FAIL
    
    casual_return( state, reply)

def py_service_echo( buffer):
    """
    Implements echo
    """

    state = SUCCESS
    reply = None

    try:
        data = buffer.contents.data[0:buffer.contents.len]
        reply = JsonBuffer(data)
        
    except:
        state = FAIL

    casual_return( state, reply)

def main():
        
   start_server(
        [ Service("casual/python/example/uppercase", py_service_uppercase, "example", 3),
          Service("casual/python/example/echo", py_service_echo, "example", 2)])             


if __name__ == '__main__':
    main()
