import ctypes
import os
from casual.xatmi.xatmi import tpalloc, tpfree, tperrno, tperrnostring, \
    X_OCTET, CASUAL_BUFFER_BINARY_TYPE, CASUAL_BUFFER_BINARY_SUBTYPE, \
    CASUAL_BUFFER_JSON_TYPE, CASUAL_BUFFER_JSON_SUBTYPE, \
    CASUAL_BUFFER_YAML_TYPE, CASUAL_BUFFER_YAML_SUBTYPE, \
    CASUAL_BUFFER_XML_TYPE, CASUAL_BUFFER_XML_SUBTYPE

from casual.server.exception import BufferError

BufferTypeMap = {
    'x_octet': (X_OCTET, 0),
    'binary': (CASUAL_BUFFER_BINARY_TYPE, CASUAL_BUFFER_BINARY_SUBTYPE),
    'json': (CASUAL_BUFFER_JSON_TYPE, CASUAL_BUFFER_JSON_SUBTYPE),
    'yaml': (CASUAL_BUFFER_YAML_TYPE, CASUAL_BUFFER_YAML_SUBTYPE),
    'xml': (CASUAL_BUFFER_XML_TYPE, CASUAL_BUFFER_XML_SUBTYPE)
}

def x_octet():
    return BufferTypeMap['x_octet']
def binary():
    return BufferTypeMap['binary']
def json():
    return BufferTypeMap['json']
def yaml():
    return BufferTypeMap['yaml']
def xml():
    return BufferTypeMap['xml']

def _convert( data):
    try:
        data = data.encode()
        is_bytes = False
    except (UnicodeDecodeError, AttributeError):
        is_bytes = True
    return is_bytes, data

class Buffer(object):

    def __init__(self, buffertype, subtype, data=None):
        if data:
            self.is_bytes, data = _convert( data)
            self.size = ctypes.c_long(len(data))
            self.holder = tpalloc(buffertype, subtype, self.size)
            if self.holder:
                self.set(data)
            else:
                raise BufferError(tperrnostring(tperrno()))
        else:
            self.size = ctypes.c_long(1024)
            self.holder = tpalloc(buffertype, subtype, self.size)

    def set(self, data):
        ctypes.memmove(self.holder, data, len(data))

    def raw(self):
        return self.holder

    def data(self):
        return self.holder[0:self.size.value]

    def __del__(self):
        if self.holder and tpfree:
            tpfree( self.holder)

#
# Supported buffer type
#
class JsonBuffer(Buffer):

    def __init__(self, data = None):
        buffertype, subtype = json()
        try:
            super().__init__(buffertype, subtype, data)
        except TypeError:
            super( JsonBuffer, self).__init__(buffertype, subtype, data)

#
# Supported buffer type
#
class XmlBuffer(Buffer):

    def __init__(self, data = None):
        buffertype, subtype = xml()
        try:
            super().__init__(buffertype, subtype, data)
        except TypeError:
            super( XmlBuffer, self).__init__(buffertype, subtype, data)

def create_buffer(buffer):
    theType=type(buffer)
    if theType is XmlBuffer:
        return XmlBuffer()
    elif theType is JsonBuffer:
        return JsonBuffer()
