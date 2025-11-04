# serialize

## description

## archives

### binary

### ini

### json

### line

### log

### protobuf

Non existing and with current semantics in the archive mechanism, it is not
possible to serialize in proto3 compliant manner (i.e. to be able to reverse 
engineer an value-object archive into a proto3 message)

#### obstacels

- proto3 optional in conjunction with the fact that default values are disposed
and to solve this archives need to be optional aware
- proto3 repeated and nested arrays and the fact that strings are not packed 
that makes the archive confused when zero sized array occurs (to determine the 
kind of array (composite or pod))
- casual tuples cannot be described at all in a proto3 message

### xml

### yaml
