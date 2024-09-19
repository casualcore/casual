# configuration transaction

This is the runtime configuration for a casual domain.

The sections can be splitted in arbitrary ways, and aggregated to one configuration model `casual` uses.
Same sections can be defined in different files, hence, give more or less unlimited configuration setup.

Most of the _sections_ has a property `note` for the user to provide descriptions of the documentation. 
The examples further down uses this to make it easier to understand the examples by them self. But can of
course be used as a mean to help document actual production configuration.  



## domain.transaction

Defines transaction related configuration.

## domain.transaction.default.resource

property                 | description                  | default
-------------------------|------------------------------|---------
[key : `string`]         | default key to use           |
[instances : `integer`]  | default number of instances  | `1`

Note: `key` has to be present in `system.resources.key`


### domain.transaction.log : `string`

The path of the distributed transactions log file. When a distributed transaction reaches prepare,
this state is persistent stored, before the actual commit stage.

if `:memory:` is used, the log is non-persistent. 

### domain.transaction.resources _(list)_

Defines all resources that `servers` and `executables` can be associated with, within this configuration.

property                | description                                | default
------------------------|--------------------------------------------|--------------------------------
name : `string`         | a (unique) name to reference this resource |
[key : `string`]        | the resource key                           | `domain.transaction.default.resource.key`
[instances : `integer`] | number of resource-proxy instances         | `domain.transaction.default.resource.instances`
[openinfo : `string`]   | resource specific _open_ configurations    |
[closeinfo : `string`]  | resource specific _close_ configurations   |


Note: `key` has to be present in `system.resources.key`

## Examples

* [domain/transaction.yaml](sample/domain/transaction.yaml)
* [domain/transaction.json](sample/domain/transaction.json)
* [domain/transaction.xml](sample/domain/transaction.xml)
* [domain/transaction.ini](sample/domain/transaction.ini)

