import casual.server.api as casual
import casual.server.exception as exception
import casual.server.buffer as buffer

try:
    buf = buffer.JsonBuffer()
    #buf = buffer.XmlBuffer("<root></root>")
    reply = casual.call( ".casual/domain/state", buf)
except exception.BufferError as bufferError:
    """
    Just to visualize exception type
    """
    raise bufferError
except exception.CallError as callError:
    """
    Just to visualize exception type
    """
    raise callError

print( reply.decode())
#
# Another call
#
print( casual.call( "casual/example/echo", b"echo echo echo").decode())

#
# Async call
#
id = casual.send( "casual/example/echo", b"async echo async echo async echo")
print( casual.receive( id).decode())

#
# Cancel async call
#
id = casual.send( "casual/example/echo", b"async echo async echo async echo")
casual.cancel( id)

#
#  call python service
#
# print casual.call( "py_service_echo", "pyecho pyecho pyecho")
