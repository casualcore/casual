# field development

## Sample

- [client](./../sample/client/source/field.cpp)

## Remarks

- No encoding of strings occurs

- The buffer is automaticly resized and thus might update the buffer-handle

- Don't manipulate the buffer in other ways (such as `memcpy()`) than by casual\_field-functions, 'cause the internal index will then be out of sync

- A field-repository might be needed for certain functionality

## Tools

### casual\_field\_make\_header

The tool is to provide something like mkfldhdr

Keys (id:s) to use most of the casual\_field-functions can be provided in any way by the application it self, as long as their ranges match their types, and the repository is just needed for certain casual\_field-functions where some mapping from the id (long) to some string-name (like `casual_field_name_of_id()` etc) is needed

If there is a repository, casual\_field\_make\_header can be used to create a header-file with defines where user-provided id:s are transformed to match it's type-range

usage: casual\_field\_make\_header [repository]

If the repository-file is not provided, the environment-variable CASUAL\_FIELD\_TABLE is used

Layout of the repository should conform to a structure described in this [json-schema](field.json) as in this [json-sample](./../sample/field.json) or this [xml-schema](field.xml) as in this [xml-sample](./../sample/field.xml) or even as in this [yaml-sample](./../sample/field.yaml) or this [ini-sample](./../sample/field.ini) 

