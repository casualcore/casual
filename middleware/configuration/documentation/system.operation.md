# configuration system

System wide configuration, that is not bound to a particular domain. Contains configuration that is used when
building servers, executable, transaction resource proxies. 

## system.resource.Paths _(structure)_

property                | description
------------------------|----------------------------------------------------
[include : `[string]`]  | include paths to use during _build time_
[library : `[string]`]  | library path to use during _build time_

## system.resources _(list)_

Defines which `xa` resources that are to be used when building servers, executable, resource-proxies. This is 
also used runtime by the transaction-manager

property                  | description
--------------------------|----------------------------------------------------
key : `string`            | user supplied _key_ of the resource, used to correlate the resources in other configurations
xa_struct_name : `string` | the name of the `xa` struct for the particular resource implementation 
server : `string`         | name of the _resource proxy server_ that `transaction manager` delegates _prepare, commit, rollback_ to.
[libraries : `[string]`]  | libraries that is used link with the resource _build time_ 
[paths : `Path`]          | include and library paths, during _build time_

## Examples

* [system.yaml](sample/system.yaml)
* [system.json](sample/system.json)
* [system.xml](sample/system.xml)
* [system.ini](sample/system.ini)

