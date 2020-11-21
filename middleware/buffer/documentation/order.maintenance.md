# order maintenance

## implementation

The main idea is to keep the buffer "ready to go" without the need for extra marshalling when transported, so the data is stored in network byteorder etc from start

The inserted-parameter is offset just past last write

The selected-parameter is offset just past last parse

String is stored with null-termination

Binary is stored with a size (network-long) and then it's data

## todo:s


