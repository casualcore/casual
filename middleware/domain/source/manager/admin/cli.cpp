//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/admin/cli.h"

#include "domain/manager/admin/model.h"
#include "domain/manager/admin/server.h"
#include "domain/common.h"

#include "configuration/model.h"
#include "configuration/model/load.h"
#include "configuration/model/transform.h"

#include "common/event/listen.h"
#include "common/message/dispatch/handle.h"
#include "common/argument.h"
#include "common/terminal.h"
#include "common/environment.h"
#include "common/exception/capture.h"
#include "common/execute.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/serialize/create.h"
#include "common/chronology.h"
#include "common/result.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/log.h"

#include "casual/cli/state.h"

namespace casual
{
   using namespace common;
   namespace domain::manager::admin
   {
      namespace local
      {
         namespace
         {
            namespace event
            {
               auto handler( std::vector< common::strong::correlation::id>& tasks) 
               {
                  return message::dispatch::handler( communication::ipc::inbound::device(),
                     []( const message::event::process::Spawn& event)
                     {
                        message::event::terminal::print( std::cout, event);
                     },
                     []( const message::event::process::Exit& event)
                     {
                        message::event::terminal::print( std::cout, event);
                     },
                     [&tasks]( message::event::Task& event)
                     {
                        message::event::terminal::print( std::cout, event);
                        if( event.done())
                           if( algorithm::find( tasks, event.correlation))
                              tasks.clear();
                     },
                     []( const message::event::sub::Task& event)
                     {
                        message::event::terminal::print( std::cout, event);
                     },
                     []( const message::event::Error& event)
                     {
                        message::event::terminal::print( std::cerr, event);
                     },
                     []( const message::event::Notification& event)
                     {
                        message::event::terminal::print( std::cout, event);
                     }
                  );
               };

               // generalization of the event handling
               template< typename I, typename... Args>
               void invoke( I&& invocable, Args&&... arguments)
               {
                  // if no-block we don't mess with events
                  if( ! terminal::output::directive().block())
                  {
                     invocable( std::forward< Args>( arguments)...);
                     return;
                  }

                  decltype( invocable( std::forward< Args>( arguments)...)) tasks;

                  auto condition = common::event::condition::compose(
                     common::event::condition::prelude( [&]()
                     {
                        tasks = invocable( std::forward< Args>( arguments)...);
                     }),
                     common::event::condition::done( [&tasks]()
                     { 
                        return tasks.empty();
                     })
                  );

                  // listen for events
                  common::event::listen( 
                     condition,
                     local::event::handler( tasks));
               }
               

            } // event

            namespace call
            {

               admin::model::State state()
               {
                  serviceframework::service::protocol::binary::Call call;
                  return call( admin::service::name::state).extract< admin::model::State>();
               }

               namespace scale
               {
                  auto aliases( const std::vector< admin::model::scale::Alias>& aliases)
                  {
                     serviceframework::service::protocol::binary::Call call;
                     auto reply = call( admin::service::name::scale::aliases, aliases);
                     return reply.extract< std::vector< common::strong::correlation::id>>();
                  }
               } // scale

               namespace restart
               {
                  auto aliases( const std::vector< admin::model::restart::Alias>& aliases)
                  {
                     serviceframework::service::protocol::binary::Call call;
                     auto reply = call( admin::service::name::restart::aliases, aliases);
                     return reply.extract< std::vector< common::strong::correlation::id>>();
                  }

                  auto groups( const std::vector< admin::model::restart::Group>& groups)
                  {
                     serviceframework::service::protocol::binary::Call call;
                     auto reply = call( admin::service::name::restart::groups, groups);
                     return reply.extract< std::vector< common::strong::correlation::id>>();
                  }
                  
               } // restart

               std::vector< common::strong::correlation::id> boot( const std::vector< std::string>& pattern)
               {
                  auto correlation = common::strong::correlation::id::emplace( uuid::make());
         
                  auto get_arguments = [&]()
                  {
                     std::vector< std::string> arguments;

                     if( ! pattern.empty())
                     {
                        arguments.emplace_back( "--configuration");
                        algorithm::container::append( pattern, arguments);
                     }

                     arguments.emplace_back( "--event-pid");
                     arguments.emplace_back( common::string::compose( common::process::handle().pid));

                     arguments.emplace_back( "--event-ipc");
                     arguments.emplace_back( common::string::compose( common::process::handle().ipc));

                     arguments.emplace_back( "--event-id");
                     arguments.emplace_back( common::string::compose( correlation));

                     return arguments;
                  };

                  common::process::spawn(
                     common::environment::directory::install() / "bin" / "casual-domain-manager",
                     get_arguments());  
                  

                  return { correlation};
               }


               auto shutdown()
               {
                  serviceframework::service::protocol::binary::Call call;
                  return call( admin::service::name::shutdown).extract< std::vector< common::strong::correlation::id>>();
               }

               namespace environment
               {
                  auto set( const admin::model::set::Environment& environment)
                  {
                     serviceframework::service::protocol::binary::Call call;
                     return call( admin::service::name::environment::set, environment).extract< std::vector< std::string>>();
                  }

                  auto unset( const admin::model::unset::Environment& environment)
                  {
                     serviceframework::service::protocol::binary::Call call;
                     return call( admin::service::name::environment::unset, environment).extract< std::vector< std::string>>();
                  }
               } // environment

               namespace configuration
               {
                  auto get()
                  {
                     serviceframework::service::protocol::binary::Call call;
                     return call( admin::service::name::configuration::get).extract< casual::configuration::user::Model>();
                  }

                  auto post( const casual::configuration::user::Model& model)
                  {
                     serviceframework::service::protocol::binary::Call call;
                     return call( admin::service::name::configuration::post, model).extract< std::vector< common::strong::correlation::id>>();
                  }

                  auto put( const casual::configuration::user::Model& model)
                  {
                     serviceframework::service::protocol::binary::Call call;
                     return call( admin::service::name::configuration::put, model).extract< std::vector< common::strong::correlation::id>>();

                  }
               } // configuration
            } // call

            namespace instance
            {
               template< typename E> 
               struct basic_instance
               {
                  const typename E::instance_type* instance = nullptr;
                  const E* executable = nullptr;
               };

               using Server = basic_instance< admin::model::Server>;
               using Executable = basic_instance< admin::model::Executable>;


            } // instance

            namespace format
            {
               template< typename P>
               auto process()
               {
                  auto format_configured_instances = []( const P& e)
                  {
                     return algorithm::count_if( e.instances, []( auto& i)
                     {
                        using State = decltype( i.state);
                        return algorithm::compare::any( i.state, State::running, State::spawned, State::scale_out, State::exit, State::error);
                     });
                  };

                  auto format_running_instances = []( const P& e)
                  {
                     return common::algorithm::count_if( e.instances, []( auto& i)
                     {
                        return i.state == decltype( i.state)::running;
                     });
                  };

                  auto format_restart = []( const P& e)
                  {
                     if( e.restart) 
                        return "true";
                     return "false";
                  };

                  auto format_restarts = []( const P& e)
                  {
                     return e.restarts;
                  };


                  return terminal::format::formatter< P>::construct(
                     terminal::format::column( "alias", std::mem_fn( &P::alias), terminal::color::yellow, terminal::format::Align::left),
                     terminal::format::column( "CI", format_configured_instances, terminal::color::no_color, terminal::format::Align::right),
                     terminal::format::column( "I", format_running_instances, terminal::color::white, terminal::format::Align::right),
                     terminal::format::column( "restart", format_restart, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "#r", format_restarts, terminal::color::red, terminal::format::Align::right),
                     terminal::format::column( "path", std::mem_fn( &P::path), terminal::color::blue, terminal::format::Align::left)
                  );
               }


            } // format

            namespace print
            {
               template< typename VO>
               void processes( std::ostream& out, std::vector< VO>& processes)
               {
                  out << std::boolalpha;

                  auto formatter = format::process< VO>();

                  formatter.print( std::cout, processes);
               }

            } // print

            namespace predicate
            {
               namespace less
               {
                  auto alias = []( auto& l, auto& r){ return l.alias < r.alias;};
               } // equal
            } // predicate

            namespace fetch
            {
               auto aliases()
               {
                  auto state = call::state();

                  auto aliases = common::algorithm::transform( state.servers, []( auto& s){
                     return std::move( s.alias);
                  });
                  
                  common::algorithm::transform( state.executables, aliases, []( auto& e){
                     return std::move( e.alias);
                  });

                  return aliases;
               };

               auto groups()
               {
                  auto state = call::state();
                  
                  return common::algorithm::transform( state.groups, []( auto& group)
                  { 
                     return std::move( group.name);
                  });
               };
               
            } // fetch

            namespace action
            {


               namespace list
               {
                  namespace process
                  {
                     constexpr auto legend = R"(
   alias:
      the configured alias, or the binary name (potentially with postfix to make it unique)
   CI:
      configured number of instances
   I:
      running instances
   restart;
      if instances should be restarted if exited
   #r:
      number of times instances has been initiated for a restart
   path:
      the path to the binary

)";
                  } // process
                  namespace servers
                  {
                     void invoke()
                     {
                        auto state = call::state();

                        print::processes( std::cout, algorithm::sort( state.servers, predicate::less::alias));
                     }

                     constexpr auto description = R"(list all servers)";

                     constexpr auto legend = process::legend;

                  } // servers

                  namespace executables
                  {
                     void invoke()
                     {
                        auto state = call::state();

                        print::processes( std::cout, algorithm::sort( state.executables, predicate::less::alias));
                     }

                     constexpr auto description = R"(list all executables)";

                     constexpr auto legend = process::legend;
                  } // executables

                  namespace instances 
                  {
                     namespace server
                     {
                        struct Type
                        {
                           const typename admin::model::Server::instance_type* instance = nullptr;
                           const admin::model::Server* server = nullptr;

                           friend bool operator < ( const Type& lhs, const Type& rhs)
                           {
                              auto tie = []( const Type& value)
                              {
                                 return std::tie( value.server->alias, value.instance->spawnpoint);
                              };
                              return tie( lhs) < tie( rhs);
                           }
                        };

                        void invoke()
                        {
                           const auto state = call::state();

                           std::vector< Type> instances;

                           algorithm::for_each( state.servers, [&instances]( auto& server)
                           {
                              Type result;
                              result.server = &server;
                              algorithm::transform( server.instances, instances, [&result]( auto& instance)
                              {
                                 result.instance = &instance;
                                 return result;
                              });
                           });

                           algorithm::sort( instances);

                           auto create_formatter = []()
                           {
                              auto format_pid = []( auto& i) { return i.instance->handle.pid;};
                              auto format_ipc = []( auto& i) { return i.instance->handle.ipc;};
                              auto format_state = []( auto& i) { return i.instance->state;};
                              auto format_alias = []( auto& i) { return i.server->alias;};
                              auto format_spawnpoint = []( auto& i) { return chronology::utc::offset( i.instance->spawnpoint);};

                              return terminal::format::formatter< Type>::construct(
                                 terminal::format::column( "pid", format_pid, terminal::color::white, terminal::format::Align::right),
                                 terminal::format::column( "ipc", format_ipc, terminal::color::no_color, terminal::format::Align::right),
                                 terminal::format::column( "state", format_state, terminal::color::yellow, terminal::format::Align::left),
                                 terminal::format::column( "alias", format_alias, terminal::color::cyan, terminal::format::Align::left),
                                 terminal::format::column( "spawnpoint", format_spawnpoint, terminal::color::blue, terminal::format::Align::left)
                              );
                           };

                           create_formatter().print( std::cout, instances);
                        }

                        constexpr auto description = R"(list all running server instances)";
                     } // server

                     namespace executable
                     {
                        struct Type
                        {
                           const typename admin::model::Executable::instance_type* instance = nullptr;
                           const admin::model::Executable* executable = nullptr;

                           friend bool operator < ( const Type& lhs, const Type& rhs)
                           {
                              auto tie = []( const Type& value)
                              {
                                 return std::tie( value.executable->alias, value.instance->spawnpoint);
                              };
                              return tie( lhs) < tie( rhs);
                           }
                        };

                        void invoke()
                        {
                           const auto state = call::state();

                           std::vector< Type> instances;

                           algorithm::for_each( state.executables, [&instances]( auto& executable)
                           {
                              Type result;
                              result.executable = &executable;
                              algorithm::transform( executable.instances, instances, [&result]( auto& instance)
                              {
                                 result.instance = &instance;
                                 return result;
                              });
                           });

                           algorithm::sort( instances);

                           auto create_formatter = []()
                           {
                              auto format_pid = []( auto& i) { return i.instance->handle;};
                              auto format_state = []( auto& i) { return i.instance->state;};
                              auto format_alias = []( auto& i) { return i.executable->alias;};
                              auto format_spawnpoint = []( auto& i) { return chronology::utc::offset( i.instance->spawnpoint);};

                              return terminal::format::formatter< Type>::construct(
                                 terminal::format::column( "pid", format_pid, terminal::color::white, terminal::format::Align::right),
                                 terminal::format::column( "state", format_state, terminal::color::yellow, terminal::format::Align::left),
                                 terminal::format::column( "alias", format_alias, terminal::color::cyan, terminal::format::Align::left),
                                 terminal::format::column( "spawnpoint", format_spawnpoint, terminal::color::blue, terminal::format::Align::left)
                              );
                           };

                           create_formatter().print( std::cout, instances);
                        }

                        constexpr auto description = R"(list all running executable instances)";
                     } // executable
                  } // instances
               } // list



               namespace environment
               {
                  namespace set
                  {
                     void call( const std::string& name, const std::string& value, std::vector< std::string> aliases)
                     {
                        admin::model::set::Environment environment;
                        environment.variables.emplace_back( string::compose( name, '=', value));
                        environment.aliases = std::move( aliases);

                        call::environment::set( environment);
                     }

                     auto complete = []( auto values, bool help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<variable>", "<value>", "[<alias>*]"};

                        auto list_environment = []()
                        {
                           auto transform_name = []( auto& variable)
                           {
                              return std::string{ variable.name()};
                           };

                           return algorithm::transform( common::environment::variable::system(), transform_name);
                        };

                        switch( values.size())
                        {
                           case 0: return list_environment();
                           case 1: return { "<value>"};
                           default: return fetch::aliases();
                        }
                     };

                     constexpr auto description = R"(set an environment variable for explicit aliases
                     
if 0 aliases are provided, the environment variable will be set 
for all servers and executables 
                     )";

                  } // set

                  namespace unset
                  {
                     void call( const std::string& name, std::vector< std::string> aliases)
                     {
                        admin::model::unset::Environment environment;
                        environment.variables.emplace_back( name);
                        environment.aliases = std::move( aliases);

                        call::environment::unset( environment);
                     }

                     auto complete = []( auto values, bool help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<variable>", "[<alias>*]"};

                        auto list_environment = []()
                        {
                           auto transform_name = []( auto& variable)
                           {
                              return std::string{ variable.name()};
                           };

                           return algorithm::transform( common::environment::variable::system(), transform_name);
                        };

                        switch( values.size())
                        {
                           case 0: return list_environment();
                           default: return fetch::aliases();
                        }
                     };

                     constexpr auto description = R"(unset an environment variable for explicit aliases
                     
if 0 aliases are provided, the environment variable will be unset 
for all servers and executables 
                     )";
                  }
               } // environment 
               
               namespace ping
               {
                  void invoke( std::vector< std::string> aliases)
                  {
                     // make sure we set precision and such for cout.
                     terminal::format::customize::Stream scope{ std::cout};

                     auto state = local::call::state();

                     auto servers = std::get< 0>( algorithm::intersection( state.servers, aliases, []( auto& l, auto& r){ return l.alias == r;}));
                     
                     auto ping_server = []( auto& server)
                     {
                        auto ping_instance = [&server]( auto& instance)
                        {
                           std::cout << terminal::color::yellow << server.alias;
                           std::cout << " " << instance.handle.ipc  << " ";

                           auto start = platform::time::clock::type::now();
                           auto pid = communication::instance::ping( instance.handle.ipc).pid;
                           auto end = platform::time::clock::type::now();

                           using second = std::chrono::duration< double>;
                           std::cout << terminal::color::yellow << pid << " ";
                           std::cout << terminal::color::blue << std::chrono::duration_cast< second>( end - start).count() << '\n';
                        };

                        algorithm::for_each( server.instances, ping_instance);
                     };
                     algorithm::for_each( servers, ping_server);
                  }

                  auto complete() 
                  {
                     return []( auto values, auto help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<alias>"};

                        auto state = local::call::state();

                        return algorithm::transform( 
                           algorithm::filter( state.servers, []( auto& server){ return ! server.instances.empty();}), 
                           []( auto& server){ return server.alias;});
                     };
                  }

                  constexpr auto description = R"(ping all instances of the provided server alias
)";

                  constexpr auto legend = R"(
<alias> <ipc> <pid> <time>
)"; 
               } // ping

               namespace global
               {
                  namespace state
                  {
                     void invoke( platform::process::native::type pid, const std::optional< std::string>& format)
                     {
                        auto handle = communication::instance::fetch::handle( 
                           common::strong::process::id{ pid}, communication::instance::fetch::Directive::direct);

                        if( ! handle)
                           return;

                        auto state = communication::ipc::call( handle.ipc, message::domain::instance::global::state::Request{ process::handle()}); 
                        auto archive = common::serialize::create::writer::from( format.value_or( ""));
                        archive << CASUAL_NAMED_VALUE( state);
                        archive.consume( std::cout);
                     }

                     auto complete() 
                     {
                        return []( auto values, auto help) -> std::vector< std::string>
                        {
                           if( help)
                              return { "<pid>", "[<format>]"};

                           if( ! values.empty())
                              return serialize::create::writer::complete::format()( values, help);
                           
                           // to provide auto-complete for pids might be overkill... But it's 
                           // consistent with other stuff...

                           auto state = local::call::state();
                           std::vector< std::string> result;

                           algorithm::for_each( 
                              state.servers,
                              [&result]( auto& server)
                              {
                                 algorithm::transform_if( 
                                    server.instances, 
                                    std::back_inserter( result),
                                    []( auto& instance){ return std::to_string( instance.handle.pid.value());},
                                    []( auto& instance){ return instance.handle.pid.valid();});
                              }
                           );

                           return result;
                        };
                     }

                     constexpr auto description = R"(get the 'global state' for the provided pid
)";

                  } // state
               } // global

               namespace legend
               {
                  const std::map< std::string, const char*> legends{
                     { "list-servers", action::list::servers::legend},
                     { "list-executables", action::list::executables::legend},
                     { "ping", action::ping::legend},
                  };
                  
                  
                  void invoke( const std::string& option)
                  {
                     auto found = algorithm::find( legend::legends, option);

                     if( found)
                        std::cout << found->second;
                  }

                  auto complete() 
                  {
                     return []( auto values, auto help)
                     {
                        return algorithm::transform( legends, []( auto& pair){ return pair.first;});
                     };
                  }

                  constexpr auto description = R"(the legend for the supplied option

Documentation and description for abbreviations and acronyms used as columns in output

note: not all options has legend, use 'auto complete' to find out which legends are supported.
)";
               } // legend

               namespace information
               {
                  auto call() -> std::vector< std::tuple< std::string, std::string>>
                  {
                     auto state = local::call::state();

                     auto instances_count = []( auto& range, auto predicate)
                     {
                        return algorithm::accumulate( range, 0l, [&predicate]( auto count, auto& entity) 
                        {
                           return count + algorithm::count_if( entity.instances, predicate);
                        });
                     };

                     auto all = []( auto& instance){ return true;};
                     auto running = []( auto& instance){ return instance.state == decltype( instance.state)::running;};
                     auto scale_out = []( auto& instance){ return instance.state == decltype( instance.state)::scale_out;};
                     auto scale_in = []( auto& instance){ return instance.state == decltype( instance.state)::scale_in;};
                     
                     auto restarts = []( auto& range)
                     {
                        return algorithm::accumulate( range, 0l, []( auto count, auto& entity) 
                        {
                           return count + entity.restarts;
                        });
                     };

                     auto instances_difference = [ &instances_count]( auto& range, auto minuend_predicate, auto subtrahend_predicate)
                     {
                        return instances_count( range, minuend_predicate) - instances_count( range, subtrahend_predicate);
                     };

                     return {
                        { "version.casual", state.version.casual},
                        { "version.compiler", state.version.compiler},
                        { "domain.identity.name", state.identity.name},
                        { "domain.identity.id", string::compose( state.identity.id)},
                        { "domain.manager.runlevel", string::compose( state.runlevel)},
                        { "domain.manager.created", string::compose( state.servers.at(0).instances.at(0).spawnpoint)}, // always casual-domain-manager
                        { "domain.manager.group.count", string::compose( state.groups.size())},
                        { "domain.manager.server.count", string::compose( state.servers.size())},
                        { "domain.manager.server.instances.configured", string::compose( instances_count( state.servers, all))},
                        { "domain.manager.server.instances.running", string::compose( instances_count( state.servers, running))},
                        { "domain.manager.server.instances.scale.out", string::compose( instances_count( state.servers, scale_out))},
                        { "domain.manager.server.instances.scale.in", string::compose( instances_count( state.servers, scale_in))},
                        { "domain.manager.server.instances.missing", string::compose( instances_difference( state.servers, all, running))},
                        { "domain.manager.server.restarts", string::compose( restarts( state.servers))},
                        { "domain.manager.executable.instances.configured",string::compose( instances_count( state.executables, all))},
                        { "domain.manager.executable.instances.running",string::compose( instances_count( state.executables, running))},
                        { "domain.manager.executable.instances.scale.out", string::compose( instances_count( state.executables, scale_out))},
                        { "domain.manager.executable.instances.scale.in", string::compose( instances_count( state.executables, scale_in))},
                        { "domain.manager.executable.instances.missing", string::compose( instances_difference( state.executables, all, running))},
                        { "domain.manager.executable.restarts", string::compose( restarts( state.executables))},
                     };
                  }

                  void invoke()
                  {
                     terminal::formatter::key::value().print( std::cout, call());
                  }

                  constexpr auto description = R"(collect aggregated general information about this domain)";

               } // information

            } // action

            namespace option
            {
               auto format_list = []( auto, bool){ return std::vector< std::string>{ "json", "yaml", "xml", "ini"};};

               auto boot()
               {
                  auto invoke = []( const std::vector< std::string>& patterns)
                  {
                     if( ! terminal::output::directive().block())
                     {
                        call::boot( patterns);
                        return;
                     }

                     std::vector< common::strong::correlation::id> tasks;

                     auto condition = common::event::condition::compose(
                        common::event::condition::prelude( [&]()
                        {
                           tasks = call::boot( patterns);
                        }),
                        common::event::condition::done( [&tasks]()
                        { 
                           return tasks.empty();
                        })
                     );

                     // listen for events
                     common::event::only::unsubscribe::listen( 
                        condition,
                        local::event::handler( tasks));
                  };

                  auto completion = []( auto values, auto help) -> std::vector< std::string>
                  {
                     if( help)
                        return { "<glob patterns>"};

                     return { "<value>"};
                  };
                  

                  constexpr auto description = R"(boot domain

With supplied configuration files, in the form of glob patterns.
)";

                  return argument::Option{
                     std::move( invoke), 
                     std::move( completion), 
                     { "-b", "--boot"},
                     description};
                  
               } // boot

               auto boot_strict()
               {
                  auto invoke = []( const std::vector< std::string>& patterns)
                  {
                     std::vector< common::strong::correlation::id> tasks;

                     auto condition = common::event::condition::compose(
                        common::event::condition::prelude( [ &]
                        {
                           tasks = call::boot( patterns);
                        }),
                        common::event::condition::error( []( auto& error)
                        {
                           call::shutdown();
                           throw error;
                        }),
                        common::event::condition::done( [ &tasks]
                        { 
                           return tasks.empty();
                        })
                     );

                     auto handler = local::event::handler( tasks);

                     // override handlers for 'error' events
                     handler.insert( []( const message::event::process::Exit& event)
                     {
                        message::event::terminal::print( std::cout, event);

                        if( event.state.status != 0)
                           code::raise::error( code::casual::domain_incomplete_boot, "process exited with error");
                     },
                     []( const message::event::Error& event)
                     {
                        message::event::terminal::print( std::cout, event);
                        code::raise::error( code::casual::domain_incomplete_boot, event.message);
                     });

                     common::event::only::unsubscribe::listen( 
                        condition,
                        std::move( handler));
                  };

                  auto completion = []( auto values, auto help) -> std::vector< std::string>
                  {
                     if( help)
                        return { "<glob patterns>"};

                     return { "<value>"};
                  };

                  constexpr auto description = R"(boot domain

With supplied configuration files, in the form of glob patterns.

Fails if any configured server/executable fails to start or exits with an error during the boot sequence.
)";

                  return argument::Option{
                     std::move( invoke), 
                     std::move( completion), 
                     { "--boot-strict"},
                     description};
               }

               auto shutdown()
               {
                  auto invoke = []()
                  {
                     event::invoke( &call::shutdown);
                  };

                  constexpr auto description = R"(shutdown domain
)";

                  return argument::Option{
                     std::move( invoke),
                     { "-s", "--shutdown"},
                     description};

               } // shutdown

               namespace scale
               {
                  auto aliases()
                  {
                     auto invoke = []( const std::vector< std::tuple< std::string, int>>& values)
                     {   
                        auto transform = []( auto& value){
                           if( std::get< 1>( value) < 0)
                              code::raise::error( code::casual::invalid_argument, "number of instances cannot be negative");
                              
                           admin::model::scale::Alias result;
                           result.name = std::get< 0>( value);
                           result.instances = std::get< 1>( value);
                           return result;
                        };

                        event::invoke( call::scale::aliases, common::algorithm::transform( values, transform));
                     };
                  
                     auto completion = []( auto values, bool help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<alias>", "<#>"};
                        
                        if( values.size() % 2 == 0)
                           return fetch::aliases();
                        else
                           return { "<value>"};
                     };

                     constexpr auto description = R"(scale instances for the provided aliases
)";
                     return argument::Option{
                        argument::option::one::many( std::move( invoke)), 
                        std::move( completion), 
                        argument::option::keys( { "-sa", "--scale-aliases"}, { "-si", "--scale-instances"}),
                        description};

                  } // aliases

               } // scale


               namespace restart
               {
                  auto aliases()
                  {
                     auto invoke = []( std::vector< std::string> values)
                     {
                        auto transform = []( auto& value){
                           return admin::model::restart::Alias{ std::move( value)};
                        };

                        event::invoke( call::restart::aliases, common::algorithm::transform( values, transform));
                     };

                     auto completion = []( auto values, bool help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<alias>"};
                        
                        return fetch::aliases();
                     };

                     constexpr auto description = R"(restart instances for the given aliases

note: some aliases are unrestartable
)";
                     return argument::Option{
                        argument::option::one::many( std::move( invoke)), 
                        std::move( completion), 
                        argument::option::keys( { "-ra", "--restart-aliases"}, { "-ri", "--restart-instances"}),
                        description};

                  } // restart
                  
                  auto groups()
                  {
                     auto invoke = []( std::vector< std::string> values)
                     {
                        auto transform = []( auto& value){
                           return admin::model::restart::Group{ std::move( value)};
                        };

                        event::invoke( call::restart::groups, common::algorithm::transform( values, transform));
                     };

                     auto completion = []( auto values, bool help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<group>"};
                        
                        return fetch::groups();
                     };


                     constexpr auto description = R"(restart all instances for aliases that are members of the provided groups

if no groups are provided, all groups are restated.

note: some aliases are unrestartable
)";

                     return argument::Option{
                        std::move( invoke), 
                        std::move( completion), 
                        { "-rg", "--restart-groups"},
                        description};
                  } // restart
               } // restart

               namespace configuration
               {
                  auto get()
                  {
                     auto invoke = []( const std::optional< std::string>& format)
                     {
                        auto archive = common::serialize::create::writer::from( format.value_or( "yaml"));
                        archive << call::configuration::get();
                        archive.consume( std::cout);
                     };

                     return argument::Option{ 
                        std::move( invoke), 
                        format_list, 
                        { "--configuration-get"}, 
                        "get current configuration"};
                  }

                  auto post()
                  {
                     auto invoke = []( const std::string& format)
                     {
                        casual::configuration::user::Model model;
                        auto archive = common::serialize::create::reader::consumed::from( format, std::cin);
                        archive >> model;

                        event::invoke( call::configuration::post, model);
                     };

                     constexpr auto description = R"(reads configuration from stdin and replaces the domain configuration

casual will try to conform to the new configuration as smooth as possible. Although, there could be some "noise"
depending on what parts are updated.
)";

                     return argument::Option{ 
                        std::move( invoke), 
                        format_list, 
                        { "--configuration-post"}, 
                        description};    
                  }


                  auto edit()
                  {
                     auto invoke = []( const std::optional< std::string>& format)
                     {                        
                        auto get_editor_path = []() -> std::filesystem::path
                        {
                           return environment::variable::get( environment::variable::name::terminal::editor).value_or( 
                              environment::variable::get( "VISUAL").value_or( 
                                 environment::variable::get( "EDITOR").value_or( "vi")));
                        };

                        auto current = call::configuration::get();

                        auto get_configuration_file = []( auto& domain, auto format)
                        {
                           file::output::Truncate file{ file::temporary( format)};
                           file::scoped::Path scoped{ file.path()};
                           
                           auto archive = common::serialize::create::writer::from( format);
                           archive << domain;
                           archive.consume( file);

                           return scoped;
                        };

                        auto start_editor = []( auto editor, const auto& file)
                        {
                           const auto command = string::compose( editor, ' ', file);
                           common::log::line( verbose::log, "command: ", command);
                           
                           posix::result( ::system( command.data()), "::system( ", command, ')');
                        };

                        // sink child signal from _editor_
                        signal::callback::registration< code::signal::child>( [](){});

                        auto file = get_configuration_file( current, format.value_or( "yaml"));

                        start_editor( get_editor_path(), file);

                        auto wanted = casual::configuration::model::load( { file});

                        if( wanted == casual::configuration::model::transform( current))
                        {
                           common::log::line( std::clog, "no configuration changes detected");
                           return;
                        }

                        event::invoke( call::configuration::post, casual::configuration::model::transform( wanted));
                     };

                     constexpr auto description = R"(get current configuration, starts an editor, on quit the edited configuration is posted.

The editor is deduced from the following environment variables, in this order:
  * CASUAL_TERMINAL_EDITOR
  * VISUAL
  * EDITOR

If none is set, `vi` is used.

If no changes are detected, no update will take place.
)";

                     return argument::Option{ 
                        std::move( invoke), 
                        format_list, 
                        { "--configuration-edit"}, 
                        description};    
                  }

                  auto put()
                  {
                     auto invoke = []( const std::string& format)
                     {
                        casual::configuration::user::Model model;
                        auto archive = common::serialize::create::reader::consumed::from( format, std::cin);
                        archive >> model;

                        event::invoke( call::configuration::put, model);
                     };

                     constexpr auto description = R"(reads configuration from stdin and adds or updates parts

The semantics are similar to http PUT:
* every key that is found is treated as an update of that _entity_
* every key that is NOT found is treated as a new _entity_ and added to the current state 
)";

                     return argument::Option{ 
                        std::move( invoke), 
                        format_list, 
                        { "--configuration-put"}, 
                        description};    
                  }

               } // configuration

               namespace log
               {
                  struct Type : common::compare::Order< Type>
                  {
                     Type( strong::process::id pid, std::string alias, std::filesystem::path path) 
                        : pid{ pid}, alias{ std::move( alias)}, path{ std::move( path)} {}

                     strong::process::id pid;
                     std::string alias;
                     std::filesystem::path path;

                     auto tie() const noexcept { return std::tie( alias, pid);}
                  };

                  auto reopen()
                  {
                     auto invoke = []()
                     {
                        auto state = call::state();

                        algorithm::for_each( state.servers, []( auto& server){
                           algorithm::for_each( server.instances, []( auto& instance){
                              common::signal::send( instance.handle.pid, code::signal::hangup);
                           });
                        });

                        auto instances = algorithm::accumulate( state.executables, std::vector< Type>{}, []( auto result, auto& executable)
                        {
                           for( auto& instance : executable.instances)
                              result.emplace_back( Type{ instance.handle, executable.alias, executable.path});
                        
                           return result;
                        });

                        algorithm::sort( instances);

                        auto create_formatter = []()
                        {
                           auto format_pid = []( auto& i) { return i.pid;};
                           auto format_alias = []( auto& i) { return i.alias;};
                           auto format_path = []( auto& i) { return i.path;};

                           return terminal::format::formatter< Type>::construct(
                              terminal::format::column( "pid", format_pid, terminal::color::white, terminal::format::Align::right),
                              terminal::format::column( "alias", format_alias, terminal::color::cyan, terminal::format::Align::left),
                              terminal::format::column( "path", format_path, terminal::color::blue, terminal::format::Align::left)
                           );
                        };

                        create_formatter().print( std::cout, instances);
                     };

                     return argument::Option{
                        std::move( invoke),
                        { "--log-reopen"},
                        "reopen casual.log by sending SIGHUP to all servers, and outputs all running executables"
                     };
                  }
               } // log
            } // option

         } // <unnamed>
      } // local


      struct cli::Implementation
      {
         argument::Group options()
         {
            return argument::Group{ [](){}, { "domain"}, "local casual domain related administration",
               argument::Option( &local::action::list::servers::invoke, { "-ls", "--list-servers"}, local::action::list::servers::description),
               argument::Option( &local::action::list::executables::invoke, { "-le", "--list-executables"}, local::action::list::executables::description),
               local::option::scale::aliases(),
               local::option::restart::aliases(),
               local::option::restart::groups(),
               argument::Option( &local::action::list::instances::server::invoke, { "-lis", "--list-instances-server"}, local::action::list::instances::server::description),
               argument::Option( &local::action::list::instances::executable::invoke, { "-lie", "--list-instances-executable"}, local::action::list::instances::executable::description),
               local::option::boot(),
               local::option::boot_strict(),
               local::option::shutdown(),
               argument::Option( &local::action::environment::set::call, local::action::environment::set::complete, { "--set-environment"}, local::action::environment::set::description)( argument::cardinality::any{}),
               argument::Option( &local::action::environment::unset::call, local::action::environment::unset::complete, { "--unset-environment"}, local::action::environment::unset::description)( argument::cardinality::any{}),
               local::option::configuration::get(),
               local::option::configuration::post(),
               local::option::configuration::edit(),
               local::option::configuration::put(),
            
               argument::Option( argument::option::one::many( &local::action::ping::invoke), local::action::ping::complete(), { "--ping"}, local::action::ping::description),
               argument::Option( &local::action::global::state::invoke, local::action::global::state::complete(), { "--instance-global-state"}, local::action::global::state::description),
               argument::Option( &local::action::legend::invoke, local::action::legend::complete(), { "--legend"}, local::action::legend::description),
               argument::Option( &local::action::information::invoke, { "--information"}, local::action::information::description),
               casual::cli::state::option( &local::call::state),
               local::option::log::reopen(),
            };
         }
      };

      cli::cli() = default; 
      cli::~cli() = default; 

      common::argument::Group cli::options() &
      {
         return m_implementation->options();
      }

      std::vector< std::tuple< std::string, std::string>> cli::information() &
      {
         return local::action::information::call();
      }
            

   } // domain::manager::admin
} // casual










