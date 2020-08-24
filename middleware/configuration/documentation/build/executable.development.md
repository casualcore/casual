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

## examples 

Below follows examples in human readable formats that `casual` can handle

### yaml
```` yaml
---
executable:
  resources:
    - key: rm-mockup
      name: resource-1
      note: the runtime configuration for this resource is correlated with the name 'resource-1' - no group is needed for resource configuration


  entrypoint: start

...

````
### json
```` json
{
    "executable": {
        "resources": [
            {
                "key": "rm-mockup",
                "name": "resource-1",
                "note": "the runtime configuration for this resource is correlated with the name 'resource-1' - no group is needed for resource configuration"
            }
        ],
        "entrypoint": "start"
    }
}
````
### ini
```` ini

[executable]
entrypoint=start

[executable.resources]
key=rm-mockup
name=resource-1
note=the runtime configuration for this resource is correlated with the name 'resource-1' - no group is needed for resource configuration

````
### xml
```` xml
<?xml version="1.0"?>
<executable>
 <resources>
  <element>
   <key>rm-mockup</key>
   <name>resource-1</name>
   <note>the runtime configuration for this resource is correlated with the name 'resource-1' - no group is needed for resource configuration</note>
  </element>
 </resources>
 <entrypoint>start</entrypoint>
</executable>

````
