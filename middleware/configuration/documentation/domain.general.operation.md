# configuration domain (general) 

This is the runtime configuration for a casual domain.

The sections can be splitted in arbitrary ways, and aggregated to one configuration model `casual` uses.
Same sections can be defined in different files, hence, give more or less unlimited configuration setup.

Most of the _sections_ has a property `note` for the user to provide descriptions of the documentation. 
The examples further down uses this to make it easier to understand the examples by them self. But can of
course be used as a mean to help document actual production configuration.  



## structures

General "structures" that other parts of the configuration refers to.

### Duration : `string`

A string representation of a _duration_. SI units can be used. Example: `30ms`, `1h`, `3min`, `40s`. if no SI unit `s` is used

### domain::service::Visibility : `string`

A string representation that defines the visibility of a service

value           | description
----------------|---------------------
discoverable    | service is discoverable from other domains
undiscoverable  | service is **not** discoverable from other domains


### domain::service::timeout::Contract : `string`

A string representation that defines the timeout contract. Can be one of the following:

value         |  severity  | description
--------------|------------|---------------------
linger        | non-lethal | no action, let the server be
interrupt     | non-lethal | send `interrupt` signal as a tap on the shoulder
kill          | lethal     | send `kill` signal
abort         | lethal     | send 'abort' signal 


### domain::service::Timeout _(structure)_

property                 | description                          | default
-------------------------|--------------------------------------|-------------
[duration : `Duration`]  | timeout duration for a service.      | _no duration_
[contract : `Contract`]  | action to take if timeout is passed  | `linger`


### domain::environment::Variable _(structure)_

property         | description
-----------------|----------------------------------------------------
key : `string`   | environment variable name
value : `string` | the value to associate with the environment variable

### domain::Environment _(structure)_

property                   | description
---------------------------|----------------------------------------------------
[variables : `[Variable]`] | environment variables
[files : `[string]`]       | paths to files that contains `Environment`

Note: a file referred from `files` must contain the structure.

```yaml
environment:
  variables:
    - key: "SOME_VARIABLE"
      value: "foo"
  files:
    - "another-file.json"
```

And has corresponding extension as the format _(my-environment.json, env.yaml and so on)_.


## domain.global

Defines _domain global_ configuration.

### domain.global.service

_domain global_ settings for services. This will effect services that are not otherwise configured explicitly,

property                        | description
--------------------------------|----------------------------------------------------
[execution.timeout : `Timeout`] | global default timeout

## domain.default

'default' configuration. Will be used as fallback within a configuration. Will not aggregate 'between' configurations.
Only used to help user minimize common configuration.


### domain.default.server

property                       | description                  | default
-------------------------------|------------------------------|---------------------
[instances : `integer`]        | default number of instances. | `1`
[memberships : `[string]`]     | default group memberships.   |
[environment : `Environment`]  | default server environment   |
[restart : `boolean`]          | default restart directive    | `false`

### domain.default.executable

property                       | description                  | default
-------------------------------|------------------------------|---------------------
[instances : `integer`]        | default number of instances. | `1`
[memberships : `[string]`]     | default group memberships.   |
[environment : `Environment`]  | default server environment   |
[restart : `boolean`]          | default restart directive    | `false`

### domain.default.service

property                        | description                   | default
--------------------------------|-------------------------------|------------------
[execution.timeout : `Timeout`] | default service timeout       | `domain.global.service.execution.timeout`
[visibility : Visibility]       | visibility from other domains | `discoverable`


## domain.groups _(list)_

Defines the groups in the configuration. Groups are used define dependency order, which is 
used during boot and shutdown. Arbitrary number of groups and group hierarchies can be 
defined.
Groups can be used to associate `resources` to servers/executables.

property                      | description                                        | default
------------------------------|----------------------------------------------------|-----------
name : `string`               | the name (unique key) of the group.                |
[dependencies : `[string]`]   | name of groups this group has dependency to.       |
[resources : `[string]`]      | names of resources this group is associated with   |
[enabled : `boolean`]         | if the group is enabled or not _see below_         | `true`


**[enabled : `boolean`]**

This property enables or disables the group. This could enable/disable _entities_ that has 
membership to the group.

For an entity to be _enabled_: ALL explicit and implicit membership groups has `enabled: true`

For an entity to be _disabled_: ANY explicit and implicit membership groups has `enabled: false`

Entities effected:
* domain.executables
* domain.servers
* queue.forward.groups


## domain.servers _(list)_

Defines all servers of the configuration (and domain) 

property                       | description                                                | default
-------------------------------|------------------------------------------------------------|---------------------------
path : `string`                | the path to the binary (if not absolute, `PATH` is used)   |
[alias : `string`]             | the logical (unique) name of the server.                   | `basename( path)`
[arguments : `[string]`]       | arguments to `tpsvrinit` during startup.                   |
[instances : `integer`]        | number of instances to start of the server.                | `domain.default.server.instances`
[memberships : `[string]`]     | groups that the server is member of                        | `domain.default.server.memberships`
[environment : `Environment`]  | explicit environments for instances of this server         | `domain.default.server.environment`
[restrictions : `[regex]`]     | regex patterns, only services that matches are advertised. |
[resources : `[string]`]       | explicit resource associations (resource names)            |
[restart : `boolean`]          | if the server should be restarted, if exit.                | `domain.default.server.restart`


## domain.executables _(list)_

Defines all _ordinary_ executables of the configuration. Could be any executable with a `main` function

property                       | description                                                | default
-------------------------------|------------------------------------------------------------|---------------------------
path : `string`                | the path to the binary (if not absolute, `PATH` is used)   |
[alias : `string`]             | the logical (unique) name of the executable.               | `basename( path)`
[arguments : `[string]`]       | arguments to main during startup.                          |
[instances : `integer`]        | number of instances to start of the server.                | `domain.default.executable.instances`
[memberships : `[string]`]     | groups that the executable is member of                    | `domain.default.executable.memberships`
[environment : `Environment`]  | explicit environments for instances of this executable     | `domain.default.executable.environment`
[restart : `boolean`]          | if the executable should be restarted, if exit.            | `domain.default.executable.restart`


## domain.services _(list)_

Defines service related configuration. 

Note: This configuration is tied to the service, regardless who has advertised the service.

property                        | description                      | default
--------------------------------|----------------------------------|-----------------
name : `string`                 | name of the service              |
[routes : `[string]`]           | names to use instead of `name`.  |
[execution.timeout : `Timeout`] | timeout of the service           | `domain.default.service.execution.timeout` 
[visibility : `Visibility`]     | visibility from other domains    | `domain.default.service.visibility`

Note: For _service aliases_, it's important to include the original name in `routes`

## Examples

* [domain/general.yaml](sample/domain/general.yaml)
* [domain/general.json](sample/domain/general.json)
* [domain/general.xml](sample/domain./eneral.xml)
* [domain/general.ini](sample/domain./eneral.ini)