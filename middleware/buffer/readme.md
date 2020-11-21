# buffer

## Overview

Provides some C-abstractions to fill - and read data into and from XATMI-buffers allocated with `tpalloc()`

## Types

Current abstractions are 

- [field](documentation/field.md)
- [octet](documentation/octet.md)
- [order](documentation/order.md)
- [string](documentation/string.md)

## Development

### Remarks

- All buffer-types are size-agnostic meaning that, if the respective abstractions are used, it is almost irrelevant what sizes are applied to functions like `tpalloc()`, `tprealloc()`, `tpcall()`, `tpacall()`, `tpreturn()`. See the respective abstraction for details

- All interfaces handling "binary-data" uses the arithmetic-agnostic-byte-type 'char' (in favour for various void-, signed- and unsigned-types)


