# octet development

## sample

- [client](./../sample/client/source/octet.cpp)

## overview

This buffer-type additionally supports 3 subtypes ("JSON", "XML" and "YAML") but it doesn't imply any magic but are just a mean to achieve some application-dispatch (or such)

### remarks

- You may write to the buffer in other ways (such as `memcpy()` but the size in `tpalloc()` or `tprealloc()` will then be relevant

## tools

