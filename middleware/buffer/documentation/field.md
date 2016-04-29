# field

## Sample

- [client](./../sample/client/source/field.c)

## Overview

### Description

This implementation contain a C-interface that offers functionality for a replacement to FML32 (non-standard (compared to XATMI)) to fill and read data into and from a buffer in a platform-indenpendent multi-key-value-pair-like way

### Remarks

- No encoding of strings occurs

- The buffer is automaticly resized and thus might update the buffer-handle

- Don't manipulate the buffer in other ways (such as `memcpy()`) than by casual\_field-functions, cause the internal index will be out of sync

- A field-repository might be needed for certain functionality

## Tools

### casual\_field\_make\_header

The tool is to provide something like mkfldhdr

Keys (id:s) to use most of the casual\_field-functions can be provided in any way by the application it self, as long as their ranges match their types, and the repository is just needed for certain casual\_field-functions where some mapping from the id (long) to some string-name (like `casual_field_name_of_id()` etc) is needed

If there is a repository, casual\_field\_make\_header can be used to create a header-file with defines where user-provided id:s are transformed to match it's type-range

usage: casual\_field\_make\_header [repository]

If the repository-file is not provided, the environment-variable CASUAL\_FIELD\_TABLE is used

Layout of the repository should conform to a structure described in this [json-schema](field.json) as in this [json-sample](./../sample/field.json) or this [xml-schema](field.xml) as in this [xml-sample](./../sample/field.xml) or even as in this [yaml-sample](./../sample/field.yaml) or this [ini-sample](./../sample/field.ini) 



## Implemtation

The main idea is to keep the buffer "ready to go" without the need for extra marshalling when transported and thus data is stored in network byteorder etc from start

The values layout is [ id ] [ size ] [ data... ]

The id - and size type are network-long

Some things that might be explained that perhaps is not so obvious concidering FML32 where the type (FLD\_SHORT CASUAL\_FIELD\_SHORT etc) can be deduced from the id, i.e. every id for 'short' must be between 0x0 and 0x1FFFFFF and every id for 'long' must be between 0x2000000 and 0x3FFFFFF etc. In this implementation no validation of whether the id exists in the repository-table occurs but just from the type and we're, for simplicity, CASUAL\_FIELD\_SHORT has base 0x2000000 and for now there's no proper error-handling while handling the table-
repository and that has to be improved and many other things can be improved as well ... Sean Parent would've cry if he saw this

The repository-implementation is a bit comme ci comme ca and to provide something like 'mkfldhdr32' the functionality has to be accessible from there (or vice versa)

## ToDo:s

- casual\_field\_match (and such) need to be better

- maybe not use an internal index

