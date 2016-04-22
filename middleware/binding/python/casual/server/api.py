
import casual.server.buffer as buffer
import casual.xatmi.xatmi as xatmi
import casual.server.exception as exeption

import ctypes
import os


#
# Public
#
def call( service, input, flags=0):
    """
    Use function to call casual service
    """
    
    if not service:
        raise exception.CallError, "No service supplied"
    
    inputbuffer = None
    #print type(input)
    if isinstance(input, str):
        inputbuffer = buffer.JsonBuffer( input)
        outputbuffer = buffer.JsonBuffer()
    else:
        inputbuffer = input
        outputbuffer = buffer.create( input)
        
    #print type(inputbuffer)
    #sprint type(outputbuffer)

    result = xatmi.tpcall( service, inputbuffer.raw(), inputbuffer.size, ctypes.byref(outputbuffer.holder), ctypes.byref(outputbuffer.size), flags)
    if result == -1:
        raise exception.CallError, xatmi.tperrnostring( xatmi.tperrno)
    
    return outputbuffer.data()

def send( service, input, flags=0):
    """
    Use function to asynchronous call casual service
    """
    
    if not service:
        raise CallError, "No service supplied"
    
    inputbuffer = buffer.JsonBuffer( input)

    id = xatmi.tpacall( service, inputbuffer.raw(), inputbuffer.size, flags)
    if id == -1:
        raise exception.CallError, xatmi.tperrnostring( xatmi.tperrno)
    
    return id
      
def receive( id, flags=0):
    """
    Use function to get reply from service
    """
    
    id = ctypes.c_int( id)
      
    outputbuffer = buffer.JsonBuffer()

    result = xatmi.tpgetrply( ctypes.byref( id), ctypes.byref(outputbuffer.holder), ctypes.byref(outputbuffer.size), flags)
    if result == -1:
        raise exception.CallError, xatmi.tperrnostring( xatmi.tperrno)
    
    return outputbuffer.data()

def cancel( id, flags=0):
    """
    Use function to get reply from service
    """
    
    id = ctypes.c_int( id)
    result = xatmi.tpcancel( id)
    if result == -1:
        raise exception.CallError, xatmi.tperrnostring( xatmi.tperrno)
    
    return result

def init( argc, argv):
    
    return 0 

def start_server( services, init):
    
    maxsize = len(services) + 1
    
    MappingType = xatmi.casual_service_name_mapping * maxsize
    
    mapping = MappingType()
    for i in range( maxsize - 1):
        mapping[i] = xatmi.casual_service_name_mapping();
        mapping[i].type = services[i].type
        mapping[i].transaction = services[i].transaction
        mapping[i].name = ctypes.c_char_p( services[i].name)
        mapping[i].functionPointer = xatmi.tpservice( services[i].function)

    #
    # Empty position to mark end of structure
    #
    mapping[maxsize - 1] = xatmi.casual_service_name_mapping();
    mapping[maxsize - 1].type = 0
    mapping[maxsize - 1].transaction = 0
    mapping[maxsize - 1].name = ctypes.c_char_p()
    mapping[maxsize - 1].functionPointer = xatmi.tpservice()

    ArgvType = ctypes.c_char_p * 1
    path = os.path.dirname(os.path.abspath(__file__))
    arg = ArgvType(ctypes.c_char_p(path))
    
    argument = xatmi.casual_server_argument()
    argument.services = mapping
    argument.serviceInit = xatmi.tpsvrinit_type( init)
    argument.serviceDone = ctypes.c_void_p()
    argument.argc = 1
    argument.argv = arg
    argument.xaSwitches = ctypes.pointer( xatmi.casual_xa_switch_mapping())
    
    xatmi.casual_start_server( ctypes.pointer(argument))

def casual_return( state, reply):
    
    xatmi.tpreturn( state, 0, reply.raw(), reply.size, 0)

class Service(object):
    
    def __init__(self, name, function, type = 0, transaction = 0):
        self.name = name
        self.function = function
        self.type = type
        self.transaction = transaction
         
class Server(object):
    
    def __init__(self):
        self.services = list()
        self.init = init
        
    def start(self):
        start_server( self.services, self.init)
                        
 
