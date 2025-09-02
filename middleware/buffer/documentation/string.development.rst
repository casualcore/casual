string
==================

sample
------

.. literalinclude:: ./../sample/client/source/string.cpp

remarks
-------

- No encoding of strings occurs

- You may write to the buffer in other ways (such as `strcpy()` but the size in `tpalloc()` and `tprealloc()` will then be relevant
