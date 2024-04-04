//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/user.h"
#include "configuration/example/model.h"

#include "configuration/example/build/model.h"

#include "common/serialize/create.h"
#include "common/serialize/macro.h"
#include "common/file.h"

#include "common/argument.h"
#include "common/exception/guard.h"

#include <fstream>
#include <filesystem>

namespace casual
{
   namespace configuration
   {
      namespace documentation
      {
         auto file( const std::filesystem::path& root, const std::filesystem::path& name)
         {
            // make sure we create directories if not present
            common::directory::create( root);

            return std::ofstream{ root / name};
         }

         namespace write
         {

            template< typename C, typename N>
            auto wrapper( C&& configuration, N&& name)
            {
               return CASUAL_NAMED_VALUE_NAME( std::forward< C>( configuration), name);
            }

            
            template< typename C>
            void example( std::ostream& out, const std::string& format, const C& configuration)
            {
               out << "### " << format << "\n```` " << format << '\n'; 
               
               auto writer = common::serialize::create::writer::from( format);
               writer << configuration;
               writer.consume( out);

                out << "\n````\n";
            }

            template< typename C>
            void examples( std::ostream& out, const C& configuration)
            {
               out << R"(## examples 

Below follows examples in `yaml` and `json` _(casual can also handle `ini` and `xml`)_

)";
               for( auto format : { "yaml", "json"})
                  write::example( out, format, configuration);
            }

            namespace domain
            {
               constexpr auto header = R"(

[//]: # (Attention! this is a generated markdown from casual-configuration-documentation - do not edit this file!)

This is the runtime configuration for a casual domain.

The sections can be splitted in arbitrary ways, and aggregated to one configuration model `casual` uses.
Same sections can be defined in different files, hence, give more or less unlimited configuration setup.

Most of the _sections_ has a property `note` for the user to provide descriptions of the documentation. 
The examples further down uses this to make it easier to understand the examples by them self. But can of
course be used as a mean to help document actual production configuration.  

)";
               void general( const std::filesystem::path& root)
               {
                  auto out = documentation::file( root, "domain.general.operation.md");

                  out << "# configuration domain (general) " << domain::header << R"(

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

value         | description
--------------|---------------------
linger        | no action, let the server be
kill          | send `kill` signal
terminate     | `@deprecated` send terminate signal


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
[restart : `boolean`]          | default restart directive    |

### domain.default.executable

property                       | description                  | default
-------------------------------|------------------------------|---------------------
[instances : `integer`]        | default number of instances. | `1`
[memberships : `[string]`]     | default group memberships.   |
[environment : `Environment`]  | default server environment   |
[restart : `boolean`]          | default restart directive    |

### domain.default.service

property                        | description                   | default
--------------------------------|-------------------------------|------------------
[execution.timeout : `Timeout`] | default service timeout       | `domain.global.service.execution.timeout`
[visibility : Visibility]       | visibility from other domains | `discoverable`


## domain.groups _(list)_

Defines the groups in the configuration. Groups are used define dependency order, which is 
used during boot and shutdown. 
Groups can be used to associate `resources` to servers/executables.

property                      | description
------------------------------|----------------------------------------------------
name : `string`               | the name (unique key) of the group.
[dependencies : `[string]`]   | name of groups this group has dependency to.
[resources : `[string]`]      | names of resources this group is associated with (transient to the members of the group)


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


)";
                  examples( out, example::user::part::domain::general());
               }

               void transaction( const std::filesystem::path& root)
               {
                  auto out = documentation::file( root, "domain.transaction.operation.md");

                  out << "# configuration transaction"  << domain::header << R"(

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

)";
               
                  examples( out, example::user::part::domain::transaction());
               }


               void queue( const std::filesystem::path& root)
               {
                  auto out = documentation::file( root, "domain.queue.operation.md");

                  out << "# configuration queue" << domain::header << R"(

## domain.queue

Defines the queue configuration

### Duration : `string`

A string representation of a _duration_. SI units can be used. Example: `30ms`, `1h`, `3min`, `40s`. if no SI unit `s` is used


### Retry _(structure)_

property               | description                                                  | default
-----------------------|--------------------------------------------------------------|-----------
[count : `integer`]    | number of rollbacks before move to _error queue_             | `0`
[delay : `Duration`]   | delay before message is available for dequeue after rollback | `0s`

### domain.queue.default

Default properties that will be used if not defined per `queue`

property                 | description                           | default
-------------------------|---------------------------------------|------------
[directory : `string`]   | directory for generated storage files | `${CASUAL_PERSISTENT_DIRECTORY}/queue` 
[queue.retry : `Retry`]  | retry semantics                       |


### domain.queue.groups _(list)_

Defines groups of queues which share the same storage location. Groups has no other meaning.

property               | description                                  | default
-----------------------|----------------------------------------------|------
alias : `string`       | the (unique) alias of the group.             | 
[queuebase : `string`] | the path to the storage file.                | `domain.queue.default.directory/<group-name>.qb`
[queues : `list`]      | defines all queues in this group, see below  |

Note: if `:memory:` is used as `queuebase`, the storage is non persistent


#### domain.queue.groups.queues

property           | description
-------------------|----------------------------------------------------
name : `string`    | the (unique) name of the queue.
[retry : `Retry`]  | retry semantics


### domain.queue.forward

Section to configure queue to service forwards and queue to queue forwards.


#### domain.queue.forward.default

Default properties that will be used if not explicitly configured per forward.

property                           | description                   | default
-----------------------------------|-------------------------------|------------
[service.instances : `integer`]    | number of forward instances   | `1`
[service.reply.delay : `Duration`] | reply enqueue available delay | `0s`
[queue.instances : `integer`]      | number of forward instances   | `1`
[queue.target.delay : `Duration`]  | enqueue available delay       | `0s`   


##### domain.queue.forward.groups _(list)_

property             | description
---------------------|----------------------------------
alias : `string`     | the (unique) alias of the group. 
[services : `list`]  | queue-to-services forwards
[queues : `list`]    | queue-to-queue forwards 


##### domain.queue.forward.groups.services

property                   | description                         | default
---------------------------|-------------------------------------|----------------------------------------
source : `string`          | the queue to dequeue from           |
[instances : `integer`]    | number of multiplexing 'instances'  | `domain.queue.forward.default.service.instances`
target.service : `string`  | service to call                     |
[reply.queue : `string`]   | queue to enqueue reply to           |
[reply.delay : `Duration`] | reply enqueue available delay       | 


)";
                  examples( out, example::user::part::domain::queue());
               }

               void gateway( const std::filesystem::path& root)
               {
                  auto out = documentation::file( root, "domain.gateway.operation.md");

                  out << "# configuration gateway"  << domain::header << R"(


## domain.gateway

Defines configuration for communication with other `casual` domains.

### domain.gateway.inbound

Defines all inbound related configuration (from remote domains -> local domain)

#### Limit _(structure)_

property                 | description
-------------------------|------------------------------------------
[size : `integer`]       | max size in bytes
[messages : `integer`]   | max number of messages in flight

#### domain.gateway.inbound.default

Will be used as default values for all groups.

property                                   | description                                       | default
-------------------------------------------|---------------------------------------------------|--------
[limit : `Limit`]                          | default value for limit                           |
[connection.discovery.forward : `boolean`] | if a discovery is allowed to propagate downstream | `false`

##### domain.gateway.inbound.groups.Connection _(structure)_

Defines a connection that this _inbound group_ should listen to

property                        | description                                       | default 
--------------------------------|---------------------------------------------------|---------
address : `string`              | the address to listen on, `host:port`             |         
[discovery.forward : `boolean`] | if a discovery is allowed to propagate downstream | `domain.gateway.inbound.default.connection.discovery.forward`

#### domain.gateway.inbound.groups _(list)_

Defines a list of all inbound groups

property                       | description                           | default
-------------------------------|---------------------------------------|------------
[alias : `string`]             | an _identity_ for this group instance | _generated unique name_
[limit : `Limit`]              | upper limits of inflight messages     | `domain.gateway.inbound.default.limit`
[connections : `[Connection]`] | all the connections for this group    |


### domain.gateway.outbound

Defines all outbound related configuration (from local domain -> remote domains)

##### domain.gateway.outbound.groups.Connection _(structure)_

Defines a connection that this _outbound group_ should try to connect to.

property                 | description
-------------------------|----------------------------------------------------
address : `string`       | the address to connect to, `host:port` 
[services : `[string]`]  | services we're expecting to find on the other side 
[queues : `[string]`]    | queues we're expecting to find on the other side 

`services` and `queues` is used as an _optimization_ to do a _build_ discovery during startup. `casual`
will find these services later lazily otherwise. It can also be used to do some rudimentary load balancing 
to make sure lower prioritized connections are used for `services` and `queues` that could be discovered in
higher prioritized connections.

#### domain.gateway.outbound.groups _(list)_

Each group gets an _order_ in the order they are defined. Groups defined lower down will only be used if the higher
ups does not provide the wanted _service_ or _queue_. Hence, the lower downs can be used as _fallback_.

property                       | description                           | default
-------------------------------|---------------------------------------|------------
[alias : `string`]             | an _identity_ for this group instance | _generated unique name_
[connections : `[Connection]`] | all the connections for this group

All connections within a group ar treated equal, and service calls will be load balanced with _round robin_. Although,
`casual` will try to _route_ the same transaction to the previous _associated_ connection with the specific transaction. 
This is only done to minimize the amount of _resources_ involved within the prepare and commit/rollback stage.  


### domain.gateway.reverse

This section defines _reverse_ `inbound` and `outbound`. The connection phase is reversed.
* `outbound` connection listen to it's' configured address.
* `inbound` connections tries to connect to it's configured address.

Otherwise, the semantics and configuration are exactly the same.

### domain.gateway.reverse.inbound

Exactly the same as [domain.gateway.inbound](#domaingatewayinbound)

### domain.gateway.reverse.outbound

Exactly the same as [domain.gateway.outbound](#domaingatewayoutbound)


)";

                  examples( out, example::user::part::domain::gateway());
               }

               void all( const std::filesystem::path& root)
               {
                  domain::general( root);
                  domain::transaction( root);
                  domain::queue( root);
                  domain::gateway( root);

               }

            } // domain
        
            void system( const std::filesystem::path& root)
            {
               auto out = documentation::file( root , "system.operation.md");

               out << R"(# configuration system

[//]: # (Attention! this is a generated markdown from casual-configuration-documentation - do not edit this file!)

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


)"; 
               examples( out, example::user::part::system());
            }

            void build_server( const std::filesystem::path& root)
            {
               auto out = documentation::file( root / "build", "server.development.md");

               out << R"(# configuration build-server

[//]: # (Attention! this is a generated markdown from casual-configuration-documentation - do not edit this file!)

Defines user configuration when building a casual server.

### services

Defines which services the server has (and advertises on startup). The actual `xatmi` conformant
function that is bound to the service can have a different name.

Each service can have different `transaction` semantics:

type         | description
-------------|----------------------------------------------------------------------
automatic    | join transaction if present else start a new transaction (default type)
join         | join transaction if present else execute outside transaction
atomic       | start a new transaction regardless
none         | execute outside transaction regardless

### resources

Defines which `xa` resources to link and use runtime. If a name is provided for a given
resource, then startup configuration phase will ask for resource configuration for that 
given name. This is the preferred way, since it is a lot more explicit.

)"; 
               examples( out, example::build::model::server());
            }

            void build_executable( const std::filesystem::path& root)
            {
               auto out = documentation::file( root / "build", "executable.development.md");

               out << R"(# configuration build-executable

[//]: # (Attention! this is a generated markdown from casual-configuration-documentation - do not edit this file!)

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

)"; 
               examples( out, example::build::model::executable());
            }

         } // write

         void main( int argc, char **argv)
         {
            std::string root;

            using namespace common::argument;

            Parse{
               R"(
Produces configuration documentation
)",
               Option( std::tie( root), { "--root"}, "the root of where documentation will be generated"),
            }( argc, argv);

            if( root.empty())
               return;

            write::domain::all( root);
            write::system( root);
            write::build_server( root);
            write::build_executable( root);
         }

      } // documentation

   } // configuration

} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::main::cli::guard( [=]()
   {
      casual::configuration::documentation::main( argc, argv);
   });
}
