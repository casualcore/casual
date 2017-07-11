# Octet

## Table of Contents

<!-- toc -->

- [Overview](#markdown-header-overview)
- [Implementation](#markdown-header-implementation)
- [Development](#markdown-header-development)
  * [Sample](#markdown-header-sample)
  * [Remarks](#markdown-header-remarks)

<!-- tocstop -->

## Overview

This implementation contain a C-interface that offers functionality to set and get data into and from a buffer and are just a tiny addition 
to the XATMI-standard `x-octet-buffer` ("X-OCTET").

## Implementation

The main objective is to just copy the data to the buffer and possibly resize it.

## Development

This buffer-type additionally supports 3 subtypes (`JSON`, `XML` and `YAML`) but it doesn't imply any magic but are just a mean to achieve some application-dispatch.

### Sample

- [Client](./../sample/client/source/octet.cpp)

### Remarks

You may write to the buffer in other ways such as `memcpy()`. But then the size in `tpalloc()` or `tprealloc()` will be relevant.
