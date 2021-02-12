import casual.server.api as casual

#
# Another call
#
print( casual.call( "casual/python/example/echo", b"{'name' : 'Casual'}").decode())
