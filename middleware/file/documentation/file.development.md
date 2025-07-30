# casual file development

`casual-file` is a built in mechanism to manipulate files within (XA) transactions and thus it requires a transaction.

`casual-file` requires no configuration.

## isolation

`casual::file::blocking::reserve()` is a blocking operation so the caller have to wait for potentiall other transactions using the same path.

`casual::file::non::blocking::reserve()` returns immediately if the resource (path) is used by other transactions.

Multiple access to the same (original) path within the same transaction if of course possible.

***note***

`casual-file` does not (so far) provide isolation between domains, i.e. the behaviour of using the same (physical) path simultaneously, despite the same global transaction, in different domains is undefined and of course no protection from external manipulating is provided.

## samples

`tx_begin()`and `tx_commit()` are used in the these examples just to explicitly clarify that the operations must be used with transactions, but of course there could already be an existing transaction and thus no need to start one explicitly.

### create a file

```c++
#include <api/file.h>
...
   tx_begin();
...

   const auto path = casual::file::reserve( the_path);
   std::ofstream{ path} << data;

...
   tx_commit();
...
```

### update a file

```c++
#include <api/file.h>
...
   tx_begin();
...

   const auto path = casual::file::reserve( the_path);
   std::ofstream{ path} << data;

...
   tx_commit();
...
```

### read a file

```c++
#include <api/file.h>
...
   tx_begin();
...

   const auto path = casual::file::reserve( the_path);
   std::ifstream{ path} >> data;

...
   tx_commit();
...
```

### remove a file

```c++
#include <api/file.h>
...
   tx_begin();
...

   const auto path = casual::file::reserve( the_path);
   std::filesystem::remove( path);

...
   tx_commit();
...
```

### rename a file

```c++
#include <api/file.h>
...
   tx_begin();
...

   const auto source = casual::file::reserve( the_source_path);
   const auto target = casual::file::reserve( the_target_path);
   std::filesystem::rename( source, target);

...
   tx_commit();
...
```
