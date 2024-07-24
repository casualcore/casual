#!/usr/bin/env python3
import sys
import json
from casual.server.buffer import JsonBuffer
from casual.server.api import start_server, Service, casual_return, SUCCESS, FAIL, call
from casual.server.log import userlog
from casual.server.exception import BufferError, CallError

def parseInput( input ):

    parameters={}
    for nvp in input.split( '&'):
        userlog( 'nvp:[' + nvp + ']')     
        name, value = nvp.split( '=')
        parameters[name.strip()] = value.strip()
        userlog( 'name:[' + name + '], value:[' + value + ']')  

    return parameters

def parseServer( answers):
    
    result = json.loads( answers)
    message = ""
    for item in result['result']['servers']:
        message += '{}\n'.format( item['alias'] )
    data = '{"text":"' + message + '"}'

    return data

def parseService( answers):
    
    result = json.loads( answers)
    message = ""
    for item in result['result']['services']:
        message += '{}\n'.format( item['name'] )
    data = '{"text":"' + message + '"}'

    return data

def parseQueue( answers):
    
    result = json.loads( answers)
    message = ""
    for item in result['result']['queues']:
        message += '{}\n'.format( item['name'] )
    data = '{"text":"' + message + '"}'

    return data

def py_service_slack( buffer):
    """
    Implements slack
    """

    state = SUCCESS
    reply = None

    try:
        data = buffer.contents.data[0:buffer.contents.len]
        parameters = parseInput( data)
        userlog( 'parameters:[' + str(parameters) + ']')
        answer = None
        data = None
        selection = parameters['text'] if 'text' in parameters else None
        if selection == 'service':
            service = '.casual/service/state'
            answer = call(service,'{}')
            data = parseService( answer)
        elif selection == 'queue':
            service = '.casual/queue/state'
            answer = call(service,'{}')
            data = parseQueue( answer)
        else:
            service = '.casual/domain/state'
            answer = call(service,'{}')
            data = parseServer( answer)

        reply = JsonBuffer(data)

    except CallError as ex:
        reply = JsonBuffer('{"text":"' + str(ex) + '"}')
        state = SUCCESS
    except:
        reply = JsonBuffer('{"text":"some unidentified error"}')
        state = SUCCESS

    casual_return( state, reply)

def main():

   start_server(
        [ Service("slack", py_service_slack, "example", 3)
          ])


if __name__ == '__main__':
    main()

