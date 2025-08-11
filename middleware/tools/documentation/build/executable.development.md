# casual-build-executable

Builds a `causal-executable`. That is, an executable with `XA-resources` linked in that will 
be initialized on boot.

The provided `entrypoint` will be invoked after `casual` has done some initialization magic
for the `XA-resources`. Hence, your `entrypoint` is your _main_, and you can start using your
`XA-resources' from there.

## prerequisite

`casual` is installed and `CASUAL_HOME` is set to the install path.


## example

We have our `main.cpp`, which defines the `entrypoint` _start_

```cpp

extern "C"
{
   int start( int argc, char** argv)
   {
      // do stuff with your xa-resources 
      return 0;
   }
}

```

We use a _executable-definition-file_ to define the resources and `entrypoint`. We name this `my-executable.yaml`, 
See *missing link?* [casual/middleware/configuration/example](../../../configuration/example/readme.md) for more information.

```yaml
executable:

  resources:
      # key of the resource (defined in resource-properties)
    - key: rm-mockup
      # a logical name that this server will use to get the proper runtime configuration.
      # it's probably a good idea to have names like <resource-type>/<application>, ex: db/my-application
      # to make it easier to configure correctly. Can be any name though.
      # If the domain does not have resource-configuration for the defined name, the executable will not boot.
      name: resource-1
      
  entrypoint: start
```

_see [build-executable-example](../../../configuration/example/build/executable.yaml) for further details._


* We want the server to be named `my-executable`
* Use the _definition-file_ to define the executable (which adds a dependency to a resource with the name `resource-1` and the key `rm-mockup` )
* Pass the source file _main.cpp_ to `casual-build-executable` to be compiled at the same time as _building_ the server.

```bash
$ casual-build-executable --output my-executable --definition my-executable.yaml --build-directives main.cpp 
```

### custom

`casual-build-executable` assumes _gcc/g++_ option compatibility for default stuff. If you use another compiler you can
opt in **not** to use default include/library-paths and so on.

**note:** you need to provide all paths, libraries and such 

```bash
$ casual-build-executable --no-defaults --output my-executable --definition my-executable.yaml --build-directives main.cpp <all other stuff your compiler needs>
```

## options

see [casual-build-executable.development.md](./casual-build-executable.development.md).
