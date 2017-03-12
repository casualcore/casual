# octet

## Sample

- [client](./../sample/client/source/octet.cpp)

## Overview

### Description

This implementation contain a C-interface that offers functionality to set and get data into and from a buffer and are just a tiny addition to the XATMI-standard x-octet-buffer ("X-OCTET")

This buffer-type additionally supports 3 subtypes ("JSON", "XML" and "YAML") but it doesn't imply any magic but are just a mean to achieve some application-dispatch (or such)

### Remarks

- You may write to the buffer in other ways (such as `memcpy()` but the the size in `tpalloc()` or `tprealloc()` will the be relevant

## Tools

## Implementation

The main objective is to just copy the data to the buffer and possibly resize it

## ToDo:s

