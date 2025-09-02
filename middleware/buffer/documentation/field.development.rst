=================
field
=================


sample
-------

.. literalinclude:: ./../sample/client/source/field.cpp

remarks
-------

- No encoding of strings occurs

- The buffer is automatically resized and therefore might update the buffer-handle

- Don't manipulate the buffer in other ways (such as `memcpy()`) than by casual\_field-functions, because the internal index will then be out of sync

- A field-repository might be needed for certain functionality

tools
-----

casual\_field\_make\_header
---------------------------

The tool is to provide something like mkfldhdr

Keys (id:s) to use most of the casual\_field-functions can be provided in any way by the application it self, as long as their ranges match their types, and the repository is just needed for certain casual\_field-functions where some mapping from the id (long) to some string-name (like `casual_field_name_of_id()` etc) is needed.

If there is a repository, casual\_field\_make\_header can be used to create a header-file with defines where user-provided id:s are transformed to match it's type-range.

usage: casual\_field\_make\_header [repository]

If the repository-file is not provided, the environment-variable CASUAL\_FIELD\_TABLE is used

Layout of the repository should conform to a structure described in this: 

json-schema 
-----------
.. literalinclude:: field.json

json-sample
-----------
.. literalinclude:: ./../sample/field.json

xml-schema
----------
.. literalinclude:: field.xml 

xml-sample
----------
.. literalinclude:: ./../sample/field.xml

yaml-sample
-----------
.. literalinclude:: ./../sample/field.yaml

ini-sample
----------
.. literalinclude:: ./../sample/field.ini