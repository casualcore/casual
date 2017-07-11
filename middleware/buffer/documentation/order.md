# Order

## Table of Contents

<!-- toc -->

- [Overview](#markdown-header-overview)
- [Implementation](#markdown-header-implementation)
- [Development](#markdown-header-development)
  * [Sample](#markdown-header-sample)
  * [Remarks](#markdown-header-remarks)

<!-- tocstop -->

## Overview

This implementation contains a C-interface that offers functionality to fill and read data into and from a buffer in a platform-indenpendent stream-like/first-in-first-out-like way.

## Implementation

- Keep the buffer "ready to go" without the need for extra marshalling when transported and thus data is stored in network byteorder from start.

- The inserted-parameter is offset just past last write.

- The selected-parameter is offset just past last parse.

- String is stored with null-termination.

- Binary is stored with a size (network-long) and then it's data.

## Development

### Sample

- [Client](./../sample/client/source/order.cpp)

### Remarks

- To use it you need to get the data in the same order as you add it since no validation of types or fields is done. The only validation that occurs is to check whether you try to consume (get) beyond what's inserted.

- No encoding of strings occurs.
