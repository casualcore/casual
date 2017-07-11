# String

## Table of Contents

<!-- toc -->

- [Overview](#markdown-header-overview)
- [Implementation](#markdown-header-implementation)
- [Development](#markdown-header-development)
  * [Sample](#markdown-header-sample)
  * [Remarks](#markdown-header-remarks)

<!-- tocstop -->

## Overview

This implementation contain a C-interface that offers functionality to set and get a null-terminated string, with some safety, into and from a buffer.

## Implementation

The main objective is to ensure a null-terminated string with a size (including `null`) less or equal than the allocated buffer (that might 
be auto-resized if `casual_string`-functions are used) both in requests and replies.

## Development

### Sample

- [Client](./../sample/client/source/string.cpp)

### Remarks

- No encoding of strings occurs.

- You may write to the buffer in other ways such as `strcpy()`. But then the size in `tpalloc()` and `tprealloc()` will be relevant.
