import ctypes
import os
from test.test_multiprocessing import c_int
import xatmi
import exception

def x_octet():
     return xatmi.X_OCTET, 0
def binary(): 
    return xatmi.CASUAL_BUFFER_BINARY_TYPE, xatmi.CASUAL_BUFFER_BINARY_SUBTYPE
def json(): 
    return xatmi.CASUAL_BUFFER_JSON_TYPE, xatmi.CASUAL_BUFFER_JSON_SUBTYPE
def yaml(): 
    return xatmi.CASUAL_BUFFER_YAML_TYPE, xatmi.CASUAL_BUFFER_YAML_SUBTYPE
def xml(): 
    return xatmi.CASUAL_BUFFER_XML_TYPE, xatmi.CASUAL_BUFFER_XML_SUBTYPE

class Buffer(object):
    
    def __init__(self, buffertype, subtype, data=None):
        
        if data:
            self.size = ctypes.c_long(len(data))
            self.holder = xatmi.tpalloc( buffertype, subtype, self.size)
            if self.holder:
                self.set(data)
            else:
                raise exception.BufferError, xatmi.tperrnostring( xatmi.tperrno)
        else:
            self.size = ctypes.c_long(1024)
            self.holder=xatmi.tpalloc( buffertype, subtype, self.size)
                
    def set(self, data):
        ctypes.memmove(self.holder, data, len(data))
    
    def raw(self):
        return self.holder
        
    def data(self):
        return self.holder[0:self.size.value]
     
    def __del__(self):
        if self.holder:
            xatmi.tpfree( self.holder)
        
#
# Supported buffer type
#
class JsonBuffer(Buffer):
    
    def __init__(self, data=None):
        buffertype, subtype = json()
        Buffer.__init__(self, buffertype, subtype, data)
        
#
# Supported buffer type
#
class XmlBuffer(Buffer):
    
    def __init__(self, data=None):
        buffertype, subtype = xml()
        Buffer.__init__(self, buffertype, subtype, data)
  
def create(buffer):
    theType = type(buffer)
    if theType is XmlBuffer:
        return XmlBuffer()
    elif theType is JsonBuffer:
        return JsonBuffer()
