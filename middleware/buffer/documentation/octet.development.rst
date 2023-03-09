octet development
=================

sample
------

.. literalinclude:: ./../sample/client/source/octet.cpp

overview
--------

This buffer-type additionally supports 3 subtypes ("JSON", "XML" and "YAML") but it doesn't imply any magic it's just a means to achieve some application-dispatch (or such)

remarks
-------

- You may write to the buffer in other ways (such as `memcpy()` but the size in `tpalloc()` or `tprealloc()` will then be relevant


