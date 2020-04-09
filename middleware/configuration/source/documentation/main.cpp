//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/user.h"
#include "configuration/example/domain.h"

#include "configuration/example/build/server.h"
#include "configuration/example/build/executable.h"
#include "configuration/example/resource/property.h"

#include "common/string.h"
#include "common/file.h"
#include "common/serialize/create.h"
#include "common/serialize/macro.h"


#include "common/argument.h"
#include "common/exception/guard.h"

namespace casual
{
   namespace configuration
   {
      namespace documentation
      {
         auto file( const std::string& root, const std::string& name)
         {
            auto path = common::string::compose( root, "/", name);
            // make sure we create directories if not present
            common::directory::create( common::directory::name::base( path));
            return std::ofstream{ path};
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

Below follows examples in human readable formats that `casual` can handle

)";
               for( auto format : { "yaml", "json", "ini", "xml"})
               {
                  write::example( out, format, configuration);
               }
            }

            void domain( const std::string& root)
            {
               auto out = documentation::file( root, "domain.operation.md");

               out << R"(# configuration domain

This is the runtime configuration for a casual domain.

Most of the _sections_ has a property `note` for the user to provide descriptions of the documentation. 
The examples further down uses this to make it easier to understand the examples by them self. But can of
course be used as a mean to help document actual production configuration.  

The following _sections_ can be used:

## transaction

Defines transaction related configuration.

### log

The path of the distributed transactions log file. When a distributed transaction reaches prepare,
this state is persistent stored, before the actual commit stage.

if `:memory:` is used, the log is non-persistent. 

### resources

Defines all resources that `servers` and `executables` can be associated with, within this configuration.

property       | description
---------------|----------------------------------------------------
key            | the resource key - correlates to a defined resource in the resource.property (for current _machine_)
name           | a user defined name, used to correlate this resource within the rest of the configuration. 
instances      | number of resource-proxy instances transaction-manger should start. To handle distributed transactions
openinfo       | resources specific _open_ configurations for the particular resource.
closeinfo      | resources specific _close_ configurations for the particular resource.


## groups

Defines the groups in the configuration. Groups are used to associate `resources` to serveres/executables
and to define the dependency order.

property       | description
---------------|----------------------------------------------------
name           | the name (unique key) of the group.
dependencies   | defines which other groups this group has dependency to.
resources      | defines which resources (name) this group associate with (transient to the members of the group)


## servers

Defines all servers of the configuration (and domain) 

property       | description
---------------|----------------------------------------------------
path           | the path to the binary, can be relative to `CASUAL_DOMAIN_HOME`, or implicit via `PATH`.
alias          | the logical (unique) name of the server. If not provided basename of `path` will be used
arguments      | arguments to `tpsvrinit` during startup.
instances      | number of instances to start of the server.
memberships    | which groups are the server member of (dictates order, and possible resource association)
restrictions   | service restrictions, if provided the intersection of _restrictions_ and advertised services are actually advertised.
resources      | explicit resource associations (transaction.resources.name)


## executables

Defines all _ordinary_ executables of the configuration. Could be any executable with a `main` function

property       | description
---------------|----------------------------------------------------
path           | the path to the binary, can be relative to `CASUAL_DOMAIN_HOME`, or implicit via `PATH`.
alias          | the logical (unique) name of the executable. If not provided basename of `path` will be used
arguments      | arguments to `main` during startup.
instances      | number of instances to start of the server.
memberships    | which groups are the server member of (dictates order)


## services

Defines service related configuration. 

Note that this configuration is tied to the service, regardless who has advertised the service.

property       | description
---------------|----------------------------------------------------
name           | name of the service
timeout        | timeout of the service, from the _caller_ perspective (example: `30ms`, `1h`, `3min`, `40s`. if no SI unit `s` is used)
routes         | defines what logical names are actually exposed. For _aliases_, it's important to include the original name.


## gateway

Defines configuration for communication with other `casual` domains.

### listeners

Defines all listeners that this configuration should listen on.

property       | description
---------------|----------------------------------------------------
address        | the address to listen on, `host:port` 
limit.size     | the maximum allowed size of all inflight messages. If reached _inbound_ stop taking more request until below the limit 
limit.messages | the maximum allowed number of inflight messages. If reached _inbound_ stop taking more request until below the limit


### connections

Defines all _outbound_ connections that this configuration should try to connect to.

The order in which connections are defined matters. services and queues found in the first connection has higher priority
than the last, hence if several remote domains exposes the the same service, the first connection will be used as long as
the service is provided. When a connection is lost, the next connection that exposes the service will be used.


property       | description
---------------|----------------------------------------------------
address        | the address to connect to, `host:port` 
restart        | if true, the connection will be restarted if connection is lost.
services       | services we're expecting to find on the other side 
queues         | queues we're expecting to find on the other side 

`services` and `queues` is used as an _optimization_ to do a _build_ discovery during startup. `casual`
will find these services later lazily otherwise. It can also be used to do some rudimentary load balancing 
to make sure lower prioritized connections are used for `services` and `queues` that could be discovered in
higher prioritized connections.

## queue

Defines the queue configuration

### groups

Defines groups of queues which share the same storage location. Groups has no other meaning.

property       | description
---------------|----------------------------------------------------
name           | the (unique) name of the group.
queuebase      | the path to the storage file. (default: `${CASUAL_DOMAIN_HOM}/queue/<group-name>.qb`, if `:memory:` is used, the storage is non persistent)
queues         | defines all queues in this group, see below

#### groups.queues

property       | description
---------------|----------------------------------------------------
name           | the (unique) name of the queue.
retry.count    | number of rollbacks before moving message to `<queue-name>.error`.
retry.delay    | if message is rolled backed, how long delay before message is avaliable for dequeue. (example: `30ms`, `1h`, `3min`, `40s`. if no SI unit `s` is used)



)"; 
               examples( out, wrapper( example::domain(), "domain"));
            }
        
            void resource_property( const std::string& root)
            {
               auto out = documentation::file( root, "resource/property.operation.md");

               out << R"(# configuration resource-property

Defines machine global configuration of resources. It's used when building casual servers
and executables, and also by `casual-transaction-manager` to deduce which _xa-resource-proxy-server_ it should start.

### resources

Defines which `xa` resources that are available on a particular _machine_.

properties     | description
---------------|----------------------------------------------------
key            | user supplied _key_ of the resource, used to correlate the resources in other configurations
xa_struct_name | the name of the `xa` struct for the particular resource implementation 
server         | name of the _resource proxy server_ that `transaction manager` delegates _prepare, commit, rollback_ to.
libraries      | libraries that is used link with the resource _build time_ 
paths          | include and library paths, during _build time_


)"; 
               examples( out, wrapper( example::resource::property::example(), "resources"));           
            }

            void build_server( const std::string& root)
            {
               auto out = documentation::file( root, "build/server.development.md");

               out << R"(# configuration build-server

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
               examples( out, wrapper( example::build::server::example(), "server"));
            }

            void build_executable( const std::string& root)
            {
               auto out = documentation::file( root, "build/executable.development.md");

               out << R"(# configuration build-executable

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
               examples( out, wrapper( example::build::executable::example(), "executable"));
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

            write::domain( root);
            write::resource_property( root);
            write::build_server( root);
            write::build_executable( root);
         }

      } // documentation

   } // configuration

} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::main::guard( std::cerr, [=]()
   {
      casual::configuration::documentation::main( argc, argv);
   });
}
