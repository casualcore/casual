import ctypes
import json
from collections.abc import Callable
from pathlib import Path
from typing import Any

from casual.server.buffer import Buffer, JsonBuffer
from casual.server.exception import CallError
from casual.server.log import userlog
from casual.xatmi.xatmi import (
    TPESVCFAIL,
    TPFAIL,
    TPSUCCESS,
    casual_server_argument,
    casual_service_name_mapping,
    casual_start_server,
    casual_xa_switch_mapping,
    tpacall,
    tpcall,
    tpcancel,
    tperrno,
    tperrnostring,
    tpgetrply,
    tpreturn,
    tpservice,
    tpsvrinit_type,
)

# Alias constants for convenience
FAIL = TPFAIL
SUCCESS = TPSUCCESS

# Global registry of services initialized via the @service decorator
_REGISTERED_SERVICES: list["Service"] = []


def is_valid_json(data) -> bool:
    """Check whether input data represents a valid JSON string or payload."""
    if not isinstance(data, (str, bytes, bytearray)):
        return False
    try:
        json.loads(data)
        return True
    except (json.JSONDecodeError, TypeError):
        return False


def decode_data(data) -> str:
    """Helper function to decode binary data to a UTF-8 string when necessary."""
    if isinstance(data, bytes):
        return data.decode('utf-8')
    return data


def _prepare_input_buffer(input_data: Any) -> tuple[Buffer, bool, bool]:
    """
    Helper function to normalize and convert input data into an appropriate Buffer.
    Returns: (inputbuffer, is_json, is_bytes)
    """
    if isinstance(input_data, Buffer):
        return input_data, False, False

    is_bytes = isinstance(input_data, bytes)
    
    if is_bytes or isinstance(input_data, str):
        payload = input_data
    else:
        # Standard Python objects (dict, list, etc.) are serialized to JSON strings
        payload = json.dumps(input_data)

    return JsonBuffer(payload), True, is_bytes


def _format_output(data_str: str, expect_json: bool, is_bytes: bool) -> Any:
    """Helper function to format output data back to its expected type."""
    if expect_json:
        try:
            return json.loads(data_str)
        except json.JSONDecodeError:
            pass
            
    if is_bytes:
        return data_str.encode("utf-8")
        
    return data_str


class Service:
    """Represents a registered Casual service metadata and handler wrapper."""
    def __init__(self, name: str, function: Callable, category: str = "", transaction: int = 0):
        self.name = name
        self.function = function
        self.category = category
        self.transaction = transaction


def service(name: str | None = None, category: str = "", transaction: int = 0):
    """
    Decorator that allows the service to automatically receive and return 
    standard Python objects (dict/list) without having to handle buffers manually.
    """
    def decorator(func: Callable) -> Callable:
        svc_name = name or func.__name__

        def wrapped_service_handler(buffer):
            try:
                input_data = buffer.contents.data[0:buffer.contents.len]
                userlog(input_data)
                if is_valid_json(input_data):
                    input_data = json.loads(input_data)
            except Exception as e:
                input_data = {}
                userlog(f"Exception... {e}")

            try:
                result = func(input_data)
                if result is not None:
                    reply_json = json.dumps(decode_data(result))
                    reply_buffer = JsonBuffer(reply_json)
                    tpreturn(SUCCESS, 0, reply_buffer.raw, reply_buffer.size, 0)
                else:
                    tpreturn(SUCCESS, 0, None, 0, 0)
            except Exception as e:
                error_buffer = JsonBuffer(json.dumps({"error": str(e)}))
                tpreturn(TPFAIL, 0, error_buffer.raw, error_buffer.size, 0)

        _REGISTERED_SERVICES.append(Service(
            name=svc_name,
            function=wrapped_service_handler,
            category=category,
            transaction=transaction
        ))
        return func
    return decorator


class AsyncCall:
    """Wrapper for managing asynchronous XATMI call descriptors and retrieving replies."""
    def __init__(self, descriptor: int, expect_json: bool, is_bytes: bool = False):
        self._descriptor = ctypes.c_int(descriptor)
        self._expect_json = expect_json
        self._is_bytes = is_bytes

    def receive(self, flags: int = 0) -> str | bytes:
        """Fetch the reply for the asynchronous call and parse JSON/bytes if expected."""
        outputbuffer = JsonBuffer()
        result = tpgetrply(
            ctypes.byref(self._descriptor),
            ctypes.byref(outputbuffer.holder),
            ctypes.byref(outputbuffer.size),
            flags,
        )
        if result == -1:
            raise CallError(tperrnostring(tperrno()))

        data_str = decode_data(outputbuffer.data())
        return _format_output(data_str, self._expect_json, self._is_bytes)

    def cancel(self, flags: int = 0) -> int:
        """Cancel the outstanding asynchronous call descriptor."""
        result = tpcancel(self._descriptor)
        if result == -1:
            raise CallError(tperrnostring(tperrno()))
        return result


def call(service_name: str | bytes, input_data: Any, flags: int = 0) -> str | bytes:
    """
    Synchronous call that automatically handles JSON serialization and deserialization.
    You can pass a standard dict or list, and receive a dict or list back!
    """
    if not service_name:
        raise CallError("No service supplied")

    service_bytes = service_name.encode() if isinstance(service_name, str) else service_name
    inputbuffer, is_json, is_bytes = _prepare_input_buffer(input_data)
    outputbuffer = JsonBuffer()

    result = tpcall(
        service_bytes,
        inputbuffer.raw,
        inputbuffer.size,
        ctypes.byref(outputbuffer.holder),
        ctypes.byref(outputbuffer.size),
        flags,
    )

    data_str = decode_data(outputbuffer.data())

    if result == -1:
        error = tperrno()
        if error == TPESVCFAIL:
            message = f"{decode_data(tperrnostring(error))} - {data_str}"
        else:
            message = decode_data(tperrnostring(error))
        raise CallError(message)            

    return _format_output(data_str, is_json, is_bytes)


def send(service_name: str | bytes, input_data: Any, flags: int = 0) -> AsyncCall:
    """
    Asynchronous call to a service that automatically handles JSON serialization.
    Returns an AsyncCall instance to later receive or cancel the request.
    """
    if not service_name:
        raise CallError("No service supplied")

    service_bytes = service_name.encode() if isinstance(service_name, str) else service_name
    inputbuffer, is_json, is_bytes = _prepare_input_buffer(input_data)

    call_id = tpacall(service_bytes, inputbuffer.raw, inputbuffer.size, flags)
    if call_id == -1:
        raise CallError(tperrnostring(tperrno()))

    return AsyncCall(call_id, is_json, is_bytes)


def casual_return(state: int, reply: Any) -> None:
    """Convenience wrapper for returning from a service call using tpreturn."""
    tpreturn(state, 0, reply.raw, reply.size, 0)


def start_server(services: list[Service] | None = None, init: Callable = lambda *_: 0) -> None:
    """
    Starts the server. If no list is passed, the services 
    registered via the @service decorator are used.
    """
    active_services = services if services is not None else _REGISTERED_SERVICES
    
    maxsize = len(active_services) + 1
    MappingType = casual_service_name_mapping * maxsize

    mapping = MappingType()
    for i, svc in enumerate(active_services):
        mapping[i] = casual_service_name_mapping()
        mapping[i].category = svc.category.encode() if isinstance(svc.category, str) else svc.category
        mapping[i].transaction = svc.transaction
        mapping[i].name = ctypes.c_char_p(svc.name.encode())
        mapping[i].function_pointer = tpservice(svc.function)

    mapping[maxsize - 1] = casual_service_name_mapping()
    mapping[maxsize - 1].category = ctypes.c_char_p()
    mapping[maxsize - 1].transaction = 0
    mapping[maxsize - 1].name = ctypes.c_char_p()
    mapping[maxsize - 1].function_pointer = tpservice()  # type: ignore

    path = str(Path(__file__).resolve().parent).encode()
    ArgvType = ctypes.c_char_p * 1
    arg = ArgvType(ctypes.c_char_p(path))

    argument = casual_server_argument()
    argument.services = mapping
    argument.server_init = tpsvrinit_type(init)
    argument.server_done = ctypes.c_void_p()
    argument.argc = 1
    argument.argv = arg
    argument.xa_switches = ctypes.pointer(casual_xa_switch_mapping())

    casual_start_server(ctypes.pointer(argument))