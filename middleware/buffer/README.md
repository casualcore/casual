# Buffer

## Overview

Provides C-abstractions to fill - and read data into and from XATMI-buffers allocated with `tpalloc()`.

## Types

Current abstractions are: 

- [Field](./documentation/field.md)
- [Octet](./documentation/octet.md)
- [Order](./documentation/order.md)
- [String](./documentation/string.md)

## Development

### Remarks

- All buffer-types are size-agnostic meaning that, if respectively abstractions are used, it is almost non relevant what size are applied to functions like `tpalloc()`, `tprealloc()`, `tpcall()`, `tpacall()`, `tpreturn()`. See respective abstraction for details.

- All interfaces handling "binary-data" uses the arithmetic-agnostic-byte-type `char` (in favour for various void-, signed- and unsigned-types).
