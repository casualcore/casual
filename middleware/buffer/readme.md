# buffer

## Overview

### Description

Provides some abstractions to fill - and read data into and from XATMI-buffers allocated with tpalloc()

## Types

Current abstractions are 

- [field](documentation/field.md)
- [octet](documentation/octet.md)
- [order](documentation/order.md)
- [string](documentation/string.md)

## Implementation

### Remarks

- All buffer-types are size-agnostic meaning that, if respectively abstractions are used, it is almost non relevant what size are applied to functions like tpalloc(), tprealloc(), tpcall(), tpacall(), tpreturn(). See respectively abstraction for details

- All interfaces handling "binary-data" uses the arithmetic-agnostic-byte-type 'char' (in favour for various void-, signed- and unsigned-types)


