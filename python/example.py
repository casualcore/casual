import casual

try:
    reply = casual.call( ".casual.broker.state", "{}")
except casual.BufferError as bufferError:
    """
    Just to visualize exception type
    """
    raise bufferError
except casual.CallError as callError:
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
