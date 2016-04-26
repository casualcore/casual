# field

## Sample

- [client](./../sample/client/source/field.c)

## Overview

### Description

This implementation contain a C-interface that offers functionality for a replacement to FML32 (non-standard (compared to XATMI))

The content of the is a multi-key-value-pair binary buffer and the casual\_field-functions can (must) be used to add, set and get data

### Remarks

- Don't manipulate the buffer in other ways than by casual\_field-functions (such as memcpy())

- The buffer is automaticly resized

- A field-repository is just needed for certain functionality

### Tools

- casual\_field\_make\_header

### Implemtation

The implementation is quite cumbersome since "stateless" interface

The main idea is to keep the buffer "ready to go" without the need for extra marshalling when transported and thus data is stored in network byteorder etc from start

The values layout is [ id ][ size ][ data... ]

The id - and size type are network-long

Some things that might be explained that perhaps is not so obvious concidering FML32 where the type (FLD\_SHORT CASUAL\_FIELD\_SHORT etc) can be deduced from the id, i.e. every id for 'short' must be between 0x0 and 0x1FFFFFF and every id for 'long' must be between 0x2000000 and 0x3FFFFFF etc. In this implementation no validation of whether the id exists in the repository-table occurs but just from the type and we're, for simplicity, CASUAL\_FIELD\_SHORT has base 0x2000000 and for now there's no proper error-handling while handling the table-
repository and that has to be improved and many other things can be improved as well ... Sean Parent would've cry if he saw this

The repository-implementation is a bit comme ci comme ca and to provide something like 'mkfldhdr32' the functionality has to be accessible from there (or vice versa)

### ToDo:s

- casual\_field\_match (and such) need to be better

