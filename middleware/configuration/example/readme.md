# configuration examples

Since the configuration in casual is represented by an object model (data structures) we
fill the object model with some representable example configuration and generate to all
format that casual knows. Hence, the information is exactly the same in the different 
formats, and you can use whichever you prefere as a reference. 

These examples are generated during build, hence shows the true configuration representation that
casual has.


There are three configuration parts:

 name                   | description
------------------------|--------------------------
**domain**              | _casual domain configuration_
**resource propertes**  | _defines machine global configuration on resources_
**build server**        | _defines user configuration when building a casual server_

Every part has a generated default to help understand what can be set default and what 
the default values are.


## domain

This is the runtime configuration for a casual domain 

We're trying to show all configuration options that is possible in casual.

 example                      | default
------------------------------|--------------------------
 [yaml](domain/domain.yaml)   | [yaml](domain/default/domain.yaml)
 [json](domain/domain.json)   | [json](domain/default/domain.json)
 [xml](domain/domain.xml)     | [xml](domain/default/domain.xml)
 [ini](domain/domain.ini)     | [ini](domain/default/domain.ini)


## resource properties

Defines machine global configuration of resources. It's used when building casual servers 
and also by `casual-transaction-manager` to deduce which xa-resource-proxy-server it should start.


 example                            | default
------------------------------------|--------------------------
 [yaml](resource/property.yaml)   | _no default values_
 [json](resource/property.json)   | _no default values_
 [xml](resource/property.xml)     | _no default values_
 [ini](resource/property.ini)     | _no default values_


## build server

Defines user configuration when building a casual server

 example                     | default
-----------------------------|--------------------------
 [yaml](build/server.yaml)   | [yaml](build/default/server.yaml)
 [json](build/server.json)   | [json](build/default/server.json)
 [xml](build/server.xml)     | [xml](build/default/server.xml)
 [ini](build/server.ini)     | [ini](build/default/server.ini)
 
  


