
from casual.xatmi.xatmi import tpsvrinit_type, \
    tpcall, tpacall, tpcancel, tpgetrply, tperrno, \
    tperrnostring, tpservice, casual_service_name_mapping, casual_server_argument, \
    casual_xa_switch_mapping, casual_start_server, tpreturn, TPFAIL, TPSUCCESS
from casual.server.exception import CallError
from casual.server.buffer import JsonBuffer, Buffer, create_buffer

import ctypes
import os

# global descriptor buffertype list
descriptor_buffer_store = {} 

#
# Public
#

FAIL = TPFAIL
SUCCESS = TPSUCCESS

def _convert( data):
    try:
        data = data.encode()
    except (UnicodeDecodeError, AttributeError):
        pass
    return data

def call(service, input, flags=0):
    """
    Use function to call casual service
    """

    if not service:
        raise CallError("No service supplied")

    # some convertions
    service = _convert(service)

    if isinstance(input, bytes) or isinstance(input, str):
        inputbuffer = JsonBuffer(input)
        outputbuffer = JsonBuffer()
    else:
        inputbuffer = input
        outputbuffer = create_buffer(input)

    result = tpcall(service, inputbuffer.raw(), inputbuffer.size, ctypes.byref(
        outputbuffer.holder), ctypes.byref(outputbuffer.size), flags)

    if result == -1:
        raise CallError(tperrnostring(tperrno()))

    return outputbuffer.data() if inputbuffer.is_bytes else outputbuffer.data().decode()
        

def send(service, input, flags=0):
    """
    Use function to asynchronous call casual service
    """

    if not service:
        raise CallError("No service supplied")

    # some convertions
    service = _convert(service)

    inputbuffer = JsonBuffer(input)

    id = tpacall(service, inputbuffer.raw(), inputbuffer.size, flags)
    if id == -1:
        raise CallError(tperrnostring(tperrno))

    descriptor_buffer_store[id] = inputbuffer.is_bytes

    return id


def receive(id, flags=0):
    """
    Use function to get reply from service
    """

    input_was_bytes = descriptor_buffer_store[id]

    id = ctypes.c_int(id)

    outputbuffer = JsonBuffer()

    result = tpgetrply(ctypes.byref(id), ctypes.byref(
        outputbuffer.holder), ctypes.byref(outputbuffer.size), flags)
    if result == -1:
        raise CallError(tperrnostring(tperrno))

    return outputbuffer.data() if input_was_bytes else outputbuffer.data().decode()


def cancel(id, flags=0):
    """
    Use function to get reply from service
    """

    id = ctypes.c_int(id)
    result = tpcancel(id)
    if result == -1:
        raise CallError(tperrnostring(tperrno))

    return result


def init(argc, argv):

    return 0


def start_server(services, init=init):

    maxsize = len(services) + 1

    MappingType = casual_service_name_mapping * maxsize

    mapping = MappingType()
    for i in range(maxsize - 1):
        mapping[i] = casual_service_name_mapping()
        mapping[i].category = services[i].category.encode()
        mapping[i].transaction = services[i].transaction
        mapping[i].name = ctypes.c_char_p(services[i].name.encode())
        mapping[i].function_pointer = tpservice(services[i].function)

    #
    # Empty position to mark end of structure
    #
    mapping[maxsize - 1] = casual_service_name_mapping()
    mapping[maxsize - 1].category = ctypes.c_char_p()
    mapping[maxsize - 1].transaction = 0
    mapping[maxsize - 1].name = ctypes.c_char_p()
    mapping[maxsize - 1].function_pointer = tpservice()

    ArgvType = ctypes.c_char_p * 1
    path = os.path.dirname(os.path.abspath(__file__)).encode()
    arg = ArgvType(ctypes.c_char_p(path))

    argument = casual_server_argument()
    argument.services = mapping
    argument.server_init = tpsvrinit_type(init)
    argument.server_done = ctypes.c_void_p()
    argument.argc = 1
    argument.argv = arg
    argument.xa_switches = ctypes.pointer(casual_xa_switch_mapping())

    casual_start_server(ctypes.pointer(argument))


def casual_return(state, reply):
    tpreturn(state, 0, reply.raw(), reply.size, 0)


class Service(object):

    def __init__(self, name, function, category=b"", transaction=0):
        self.name = name
        self.function = function
        self.category = category
        self.transaction = transaction
