# casual file development

`casual-file` is a built in mechanism to manipulate files within (XA) transactions and thus it requires a transaction.

`casual-file` requires no configuration.

## isolation

`casual::file::blocking::reserve()` is a blocking operation so the caller have to wait for potentiall other transactions using the same path.

`casual::file::non::blocking::reserve()` returns immediately if the resource (path) is used by other transactions.

Multiple access to the same (original) path within the same transaction if of course possible.

## notes

`casual-file` implies some overhead (apart from communication) compared to plain file operations in order to be able to commit and rollback content (maybe needless to write).

## samples

### create a file

```c++
#include <api/file.h>
...
   const auto path = casual::file::reserve( the_path);

   std::ofstream{ path} << data;
...
```

### update a file

```c++
#include <api/file.h>
...
   const auto path = casual::file::reserve( the_path);

   std::ofstream{ path} << data;
...
```

### read a file

```c++
#include <api/file.h>
...
   const auto path = casual::file::reserve( the_path);

   std::ifstream{ path} >> data;
...
```

### remove a file

```c++
#include <api/file.h>
...
   const auto path = casual::file::reserve( the_path);

   std::filesystem::remove( path);
...
```

### rename a file

```c++
#include <api/file.h>
...
   const auto source = casual::file::reserve( the_source_path);
   const auto target = casual::file::reserve( the_target_path);

   std::filesystem::rename( source, target);
...
```
