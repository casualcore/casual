# Field

## Table of Contents

- [Overview](#markdown-header-overview)
- [Implementation](#markdown-header-implementation)
  * [TODO](#markdown-header-todo)
- [Development](#markdown-header-development)
  * [Sample](#markdown-header-sample)
  * [Remarks](#markdown-header-remarks)
  * [Tools](#markdown-header-tools)
    + [casual\_field\_make\_header](#markdown-header-casual_field_make_header)
- [Operation](#markdown-header-operation)

## Overview

This implementation contain a C-interface that offers functionality for a replacement to FML32 (non-standard in the contrary to XATMI) to fill and read data 
into and from a buffer in a platform-indenpendent multi-key-value-pair-like way.

## Implementation

The main idea is to keep the buffer "ready to go" without the need for extra marshalling when transported and thus data is stored in network byteorder etc from start.

The values layout is `[ id ] [ size ] [ data... ]`.

The id host-type is `long`.

The size host-type is implicitly `long`.

Some things that might be explained that perhaps is not so obvious concidering FML32 where the type (`FLD_SHORT`, `CASUAL_FIELD_SHORT` etc) can be deduced from the id, i.e. every id for `short` must be between `0x0` and `0x1FFFFFF` and every id for `long` must be between `0x2000000` and `0x3FFFFFF` etc. 

> In this implementation no validation of whether the id exists in the repository-table occurs, but just from the type and we're, for simplicity, `CASUAL_FIELD_SHORT` has base `0x2000000` and for now there's no proper error-handling while handling the table-
> repository and that has to be improved.
> 
> The repository-implementation is a bit comme ci comme ca and to provide something like `mkfldhdr32` the functionality has to be accessible from there (or vice versa).

### TODO

- `casual_field_match` (and such) needs improvement.

- Not use an internal index.

## Development

### Sample

- [Client](./../sample/client/source/field.cpp)

### Remarks

- No encoding of strings occurs

- The buffer is automaticly resized and thus might update the buffer-handle

- Don't manipulate the buffer in other ways (such as `memcpy()`) than by casual\_field-functions, 'cause the internal index will then be out of sync

- A field-repository might be needed for certain functionality

### Tools

#### casual\_field\_make\_header

The tool is to provide something like `mkfldhdr`.

Keys (id:s) to use most of the `casual_field`-functions can be provided in any way by the application it self, as long as their ranges match their types, 
and the repository is just needed for certain casual\_field-functions where some mapping from the id (`long`) to some string-name (like `casual_field_name_of_id()` etc) 
is needed.

If there is a repository, `casual_field_make_header` can be used to create a header-file with defines where user-provided id:s are transformed to match it's type-range. 

Usage:

```bash
casual_field_make_header [repository]
```

If the repository-file is not provided, the environment-variable `$CASUAL_FIELD_TABLE` is used.

Layout of the repository should conform to the following structures:

| Format | Schema                       | Example                                        |
| ------ | ---------------------------- | ---------------------------------------------- |
| JSON   | [./field.json](./field.json) | [../sample/field.json](./../sample/field.json) |
| XML    | [./field.xml](./field.xml)   | [../sample/field.xml](./../sample/field.xml)   |
| YAML   | [./field.yaml](./field.yaml) | [../sample/field.yaml](./../sample/field.yaml) |
| INI    |                              | [../sample/field.ini](./../sample/field.ini)   |

## Operation

If the application is using a casual-field-repository, the environment-variable `$CASUAL_FIELD_TABLE` needs to be present and its value should be the repository-file e.g.:

```bash
export CASUAL_FIELD_TABLE=$HOME/casual-field-repositories/field.json
```
