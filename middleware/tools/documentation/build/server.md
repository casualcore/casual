# casual-build-server

Builds a `causal-server`. That is, an executable that exposes `XATMI` services in a possible transactional context.


## prerequisite

`casual` is installed and `CASUAL_HOME` is set to the install path.


## example

In both examples we use a simple echo server defined in `main.cpp`, which has one _service_ named `echo`

```cpp
#include "xatmi.h"

extern "C"
{
   void echo( TPSVCINFO* info)
   {
      tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
   }
}

```

### simple

We want the server to be named `simple-server` that has one service `echo`. We pass the source 
file _echo.cpp_ to `casual-build-server` to be compiled at the same time as _building_ the server.


```bash
host$ casual-build-server --output simple-server --service echo --link-directives echo.cpp
```

### advanced

We use a _server-definition-file_ to define the server in it's services. We name this `example.server.yaml`, 
See [casual/middleware/configuration/example](../../../../configuration/example/readme.md) for more information.

```yaml
server:
  services:

      # name of the service, note that we use a different name from the function name
    - name: example/echo
      # name of the function the service should bind to
      function: echo

      # transaction characteristics
      # Can be one of the following
      # - auto : if a transaction is present join it, else start a new one (default)
      # - join : if a transaction is present join it,
      # - none : don't join any transaction
      # - atomic : start a new transaction regardless.
      transaction: join
```

* We want the server to be named `advanced-server`
* Use the _definition-file_ to define the server.
* Pass the source file _echo.cpp_ to `casual-build-server` to be compiled at the same time as _building_ the server.
* Build a dependency to a resource with the key `rm-mockup` 

```bash
host$ casual-build-server --output advanced-server --server-definition example.server.yaml --link-directives echo.cpp --resource-keys rm-mockup
```

We can see that `advanced-server` has a dependency to the `XA-structure` for `rm-mockup`: __casual_mockup_xa_switch_static_

```bash
host$ nm advanced-server 
                 U _casual_mockup_xa_switch_static
                 U _casual_start_server
0000000100000f30 T _echo
0000000100000e70 T _main
                 U _tpreturn
                 U _tpsvrdone
                 U _tpsvrinit
                 U dyld_stub_binder
``` 


## options

```bash
host$ casual-build-server --help
NAME
   casual-build-server

DESCRIPTION

OPTIONS
   -o, --output <value>
      name of server to be built

   -s, --service <value> 1..*
      service names

   -d, --server-definition <value>
      path to server definition file

   -r, --resource-keys <value> 1..*
      key of the resource

   -c, --compiler <value>
      compiler to use

   -f, --link-directives <value> 1..*
      additional compile and link directives

   -p, --properties-file <value>
      path to resource properties file

   -v, --verbose
      verbose output

   -k, --keep
      keep the intermediate file

   --help
      Shows this help


```