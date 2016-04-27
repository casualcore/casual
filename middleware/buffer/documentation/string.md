# string

## Sample

- [client](./../sample/client/source/string.c)

## Overview

### Description

This implementation contain a C-interface that offers functionality to set and get a null-terminated string, with some safety, into and from a buffer

### Remarks

- No encoding of strings occurs

- You may write to the buffer in other ways (such as strcpy() but the the size in tpalloc() tprealloc() and nulltermination etc will the be relevant

## Tools

## Implementation

The main objective is to ensure a null-terminated string with a size (including null) less or equal than the allocated buffer (that might be auto-resized it casual\_string-functions are used) both in requests and replies  

## ToDo:s

