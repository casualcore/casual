order
=================

sample
------

.. literalinclude:: ./../sample/client/source/order.cpp

remarks
-------

- To use it you need to get the data in the same order as you add it since no validation of types or fields what so ever occurs. The only validation that occurs is to check whether you try to consume (get) beyond what's inserted

- No encoding of strings occurs

