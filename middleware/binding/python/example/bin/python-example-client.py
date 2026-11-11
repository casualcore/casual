import casual.server.api as casual
from casual.server import exception

try:
    reply = casual.call( ".casual/domain/state", {})
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

#
# Another calls
#
print( casual.call( "casual/example/echo","echo echo echo"))

print( casual.call( "casual/example/echo", b"byte echo echo echo"))

print( casual.call( "casual/example/echo", {"name" : "Donald Trump"}))

#
# Async call
#
async_call = casual.send( "casual/example/echo", "async echo async echo async echo")
print( async_call.receive())

#
# Async call
#
async_call = casual.send( "casual/example/echo", b"async echo async echo async echo")
print( async_call.receive())

#
# Async call
#
async_call = casual.send( b"casual/example/echo", {"name" : "Victoria", "age" : 49})
print( async_call.receive())


#
# Cancel async call
#
async_call = casual.send( "casual/example/echo", b"async echo async echo async echo")
async_call.cancel()

# call services in python server
print( casual.call( "casual/python/example/echo", "casual"))
print( casual.call( "casual/python/example/upper", "casual"))

print( casual.send( "casual/python/example/hello", {"name" : "Donald"}).receive())