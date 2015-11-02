
import buffer
import xatmi
import exception

import ctypes
from test.test_multiprocessing import c_int


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
    
    id = c_int( id)
      
    outputbuffer = buffer.JsonBuffer()

    result = xatmi.tpgetrply( ctypes.byref( id), ctypes.byref(outputbuffer.holder), ctypes.byref(outputbuffer.size), flags)
    if result == -1:
        raise exception.CallError, xatmi.tperrnostring( xatmi.tperrno)
    
    return outputbuffer.data()

def cancel( id, flags=0):
    """
    Use function to get reply from service
    """
    
    id = c_int( id)
    result = xatmi.tpcancel( id)
    if result == -1:
        raise exception.CallError, xatmi.tperrnostring( xatmi.tperrno)
    
    return result
 
 
