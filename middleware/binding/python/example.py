import casual.server.api as casual
import casual.server.exception as exception
import casual.server.buffer as buffer

try:
    buf = buffer.JsonBuffer("{}")
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

print( reply)

buf = buffer.XmlBuffer(b"<root/>")
reply = casual.call( ".casual/domain/state", buf)

print( reply.decode())

#
# Another call
#
print( casual.call( "casual/example/echo","echo echo echo"))

#
# Async call
#
id = casual.send( "casual/example/echo", "async echo async echo async echo")
print( casual.receive( id))

#
# Async call
#
id = casual.send( "casual/example/echo", b"async echo async echo async echo")
print( casual.receive( id).decode())

#
# Async call
#
id = casual.send( b"casual/example/echo", b"async echo async echo async echo")
print( casual.receive( id).decode())


#
# Cancel async call
#
id = casual.send( "casual/example/echo", b"async echo async echo async echo")
casual.cancel( id)

