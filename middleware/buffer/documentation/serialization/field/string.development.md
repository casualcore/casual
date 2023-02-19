# field string serialization

Functionality to serialize a fielded buffer to and from a string representation


## semantics

Developer creates a _mapping-file_ that defines a set of transformations/serializations 
for `field <-> string` bound to an arbitrary unique key.

From this _mapping-file_ the developer generates a `cpp file`, and then compiles and links
the implementation to the executable. This can of course be done via a shared object.

From the code that is going to perform the serialization, the developer uses the `C-api` 
to invoke a specific transformation/serialization by providing the arbitrary key (and the
input/output - buffer/string)

## mapping file

The following format can be used:
* yaml
* json
* xml
* ini

The file-extension has to correlate with the format.

yaml example:

``` yaml
mapping:
  - key: some_key_1
    note: some documentation

    fields: 
      - name: FLD_STRING1 # name of the field. 
        size: 10 # size of the fixed string representation.
        # alignment: "left"  <- default value
        # padding: " " <- default value, if set, at least on char has to be set .
      
      - name: FLD_STRING2
        size: 20
        alignment: "left"
        padding: "." # will be padded _to the right_ with '.'

  - key: some_key_2
    note: some documentation

    fields: 
      - name: FLD_LONG1
        size: 10
        alignment: "right"
      
      - name: FLD_STRING2
        size: 20
        alignment: "left"

      # other yaml syntax for the same thing
      - { name: FLD_SHORT1, size: 4}
```

## generate the implementation

`casual-buffer-field-serialize` is used to generate the implementation. 

_as always, use option `--help` to get additional information on what's possible_

**Example:**
```bash
$ $CASUAL_HOME/bin/casual-buffer-field-serialize --files some-mapping-file.yaml --output some-implementation.cpp
```

### compile and link

Compile and link the implementation.

**Example:**
```bash
$ g++ -o some-implementation.o some-implementation.cpp –I$CASUAL_HOME/include -pthread -c -O3 -fpic -std=c++14
```

Link the object file to the lib/executable to the lib/executable that uses the _serialization_. 

### possible build scenarios

* One _mapping-file_ for all mappings, that generate one cpp implementation file.
  - The compiled object file could be linked directly, or via a library.
* Many _mapping-files_ that generate one cpp implementation file.
  - The compiled object file could be linked directly, or via a library.
* Many _mapping-files_ that generates many implementation files.
  - These could be linked to one shared object file (lib), that is linked to the executable


## invoke the serialization

The developer uses the `C-api` to invoke a specific transformation/serialization 
by providing the arbitrary key (and the input/output - buffer/string)

**Example:**

``` C
// error handling omitted for clarity

void some_function( const char* buffer)
{
   char memory[ 1024]; // some large enough memory
   casual_field_to_string( memory, sizeof( memory), "some-defined-key", buffer);
   // use the _stringified_ representation...
}


void some_function( const char* memory, long size)
{
   char* buffer = tpalloc( CASUAL_FIELD, NULL, 128); // buffer will be expanded during the transformation
   casual_field_from_string( &buffer, "some-defined-key", memory, size);
   // use the buffer...
}
```



