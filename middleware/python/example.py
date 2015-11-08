import casual.server.api as casual
import casual.server.exception as exception
import casual.server.buffer as buffer

try:
    buf = buffer.JsonBuffer("{}")
    #buf = buffer.XmlBuffer("<root></root>")
    reply = casual.call( ".casual.broker.state", buf)
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

print reply

#
# Another call
#
print casual.call( "casual_test1", "echo echo echo")

#
# Async call
#
id = casual.send( "casual_test1", "async echo async echo async echo")
print casual.receive( id)

#
# Cancel async call
#
id = casual.send( "casual_test1", "async echo async echo async echo")
casual.cancel( id)

#
# Another call
#
print casual.call( "py_service_echo", "pyecho pyecho pyecho")
