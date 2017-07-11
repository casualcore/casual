# Configuration examples

Since the configuration in casual is represented by an object model (data structures) we
fill the object model with some representable example configuration and generate to all
format that casual knows. Hence, the information is exactly the same in the different 
formats, and you can use whichever you prefere as a reference. 

These examples are generated during build and shows the true configuration representation that
casual has.

There are three configuration parts:

- [Domain](./README.md#markdown-header-domain) - casual domain configuration
- [Resource properties](./README.md#markdown-header-resourceproperties) - defines machine global configuration on resources
- [Build server](./README.md#markdown-header-buildserver) - defines user configuration when building a casual server

Every part has a generated default to help understand what can be set default and what 
the default values are.

## Domain

This is the runtime configuration for a casual domain.

We're trying to show all configuration options that is possible in casual.

| Format | Example                                  | Default                                                  |
| ------ | ---------------------------------------- | -------------------------------------------------------- |
| JSON   | [domain/domain.json](./domain/domain.json) | [domain/default/domain.json](./domain/default/domain.json) |
| XML    | [domain/domain.xml](./domain/domain.xml)   | [domain/default/domain.xml](./domain/default/domain.xml)   |
| YAML   | [domain/domain.yaml](./domain/domain.yaml) | [domain/default/domain.yaml](./domain/default/domain.yaml) |
| INI    | [domain/domain.ini](./domain/domain.ini)   | [domain/default/domain.ini](./domain/default/domain.ini)   |

## Resource properties

Defines machine global configuration of resources. It's used when building casual servers 
and also by `casual-transaction-manager` to deduce which `xa-resource-proxy-server` it should start.

| Format | Example                                          | Default             |
| ------ | ------------------------------------------------ | ------------------- |
| JSON   | [resource/property.json](./resource/property.json) | _no default values_ |
| XML    | [resource/property.xml](./resource/property.xml)   | _no default values_ |
| YAML   | [resource/property.yaml](./resource/property.yaml) | _no default values_ |
| INI    | [resource/property.ini](./resource/property.ini)   | _no default values_ |

## Build server

Defines user configuration when building a casual server

| Format | Example                                | Default                                                |
| ------ | -------------------------------------- | ------------------------------------------------------ |
| JSON   | [build/server.json](./build/server.json) | [build/default/server.json](./build/default/server.json) |
| XML    | [build/server.xml](./build/server.xml)   | [build/default/server.xml](./build/default/server.xml)   |
| YAML   | [build/server.yaml](./build/server.yaml) | [build/default/server.yaml](./build/default/server.yaml) |
| INI    | [build/server.ini](./build/server.ini)   | [build/default/server.ini](./build/default/server.ini)   |
