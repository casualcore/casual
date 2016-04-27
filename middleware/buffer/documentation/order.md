# order

## Sample

- [client](./../sample/client/source/order.c)

## Overview

### Description

This implementation contain a C-interface that offers functionality to fill and read data into and from a buffer in a platform-indenpendent stream-like/first-in-first-out-like way

To use it you need to get the data in the same order as you add it since no validation of types or fields what so ever occurs. The only validation that occurs is to check whether you try to consume (get) beyond what's inserted

### Remarks

- No encoding of strings occurs

## Tools

## Implementation

The main idea is to keep the buffer "ready to go" without the need for extra marshalling when transported and thus data is stored in network byteorder etc from start

The inserted-parameter is offset just past last write

The selected-parameter is offset just past last parse

String is stored with null-termination

Binary is stored with a size (network-long) and then it's data

## ToDo:s

