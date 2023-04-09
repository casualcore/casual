# casual-build-server

Builds a `causal-server`. That is, an executable that exposes `XATMI` services in a possible transactional context,
with possible `XA-resources` linked in that will be initialized on boot.


## prerequisites

`casual` is installed and `CASUAL_HOME` is set to the install path.


## example

In both examples we use a simple echo server defined in `echo.cpp`, which has one _service_ named `echo`

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

The `echo` service will have `auto` transaction semantics, see below.


```bash
$ casual-build-server --output simple-server --service echo --link-directives echo.cpp
```

### advanced

We use a _server-definition-file_ to define the server in it's services. We name this `example.server.yaml`, 

```yaml
server:

  resources:
      # key of the resource (defined in resource-properties)
    - key: rm-mockup
      # a logical name that this server will use to get the proper runtime configuration.
      # it's probably a good idea to have names like <resource-type>/<application>, ex: db/my-application
      # to make it easier to configure correctly. Can be any name though.
      # If the domain does not have resource-configuration for the defined name, the server will not boot.
      name: resource-1
      
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
      # - branch : do not use unless you know what you're doing...
      transaction: join
```

* We want the server to be named `advanced-server`
* Use the _definition-file_ to define the server (which adds a dependency to a resource with the name `resource-1` and the key `rm-mockup` )
* Pass the source file `echo.cpp` to `casual-build-server` to be compiled at the same time as _building_ the server.

```bash
$ casual-build-server --output advanced-server --definition example.server.yaml --build-directives echo.cpp 
```

We can see that `advanced-server` has a dependency to the `XA-structure` for `rm-mockup`: __casual_mockup_xa_switch_static_

```bash
$ nm advanced-server 
                 U _casual_mockup_xa_switch_static
                 U _casual_start_server
0000000100000f30 T _echo
0000000100000e70 T _main
                 U _tpreturn
                 U _tpsvrdone
                 U _tpsvrinit
                 U dyld_stub_binder
``` 

### custom

`casual-build-server` assumes _gcc/g++_ option compatibility for defaults. If you use another compiler you can
opt to **not** use the default include/library-paths and so on.

**Note:** you need to provide all paths, libraries etc.

```bash
$ casual-build-server --no-defaults --output advanced-server --definition example.server.yaml --build-directives echo.cpp <all other stuff your compiler needs>
```

### casual-build-server-generate

`casual-build-server-generate` only generate the _intermediate main file_, that has the 'magic' to bootstrap a `casual` server.

```bash
$ casual-build-server-generate --output your-name-on-the-source-file.cpp --definition example.server.yaml
```

This might be easier to use, depending och your build system.

## options

```bash
$ casual-build-server --help
NAME
   casual-build-server

DESCRIPTION
  builds a casual xatmi server

OPTIONS                                    c  value    vc  description
-----------------------------------------  -  -------  --  -----------------------------------------------
-o, --output                               ?  <value>   1  name of server to be built
-s, --service                              *  <value>   +  service names
-d, --server-definition                    ?  <value>   1  path to server definition file
-r, --resource-keys                        *  <value>   +  key of the resource
-c, --compiler                             ?  <value>   1  compiler to use
-f, --build-directives, --link-directives  *  <value>   +  additional compile and link directives
-p, --properties-file                      ?  <value>   1  path to resource properties file
--no-defaults                              ?               do not add any default compiler/link directives
--source-file                              ?  <value>   1  name of the intermediate source file
-k, --keep                                 ?               keep the intermediate source file
-v, --verbose                              ?               verbose output
--help                                     ?  <value>   *  use --help <option> to see further details

```