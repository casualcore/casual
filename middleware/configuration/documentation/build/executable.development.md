# configuration build-executable

Defines user configuration when building a casual executable.

### resources

Defines which `xa` resources to link and use runtime. A name **has** to be provided for each 
resource, startup configuration phase will ask for resource configuration for that 
given name.

### entrypoint

Defines the name of the user provided _entry point_. The signature has to be the same as a
normal main function `int <entrypoint-name>( int argc, char** argv)`.

`casual` defines the actual `main` function, and executes the startup procedure, to configure
resources and such, then invoke the `entrypoint`, and all the control is left to user. As if 
the `entrypoint` was the `main` function.


## Examples

* [build/executable.yaml](../sample/build/executable.yaml)
* [build/executable.json](../sample/build/executable.json)
* [build/executable.xml](../sample/build/executable.xml)
* [build/executable.ini](../sample/build/executable.ini)
