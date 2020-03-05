//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/admin/cli.h"

#include "domain/manager/admin/model.h"
#include "domain/manager/admin/server.h"
#include "domain/common.h"

#include "configuration/domain.h"

#include "common/event/listen.h"
#include "common/message/handle.h"
#include "common/argument.h"
#include "common/terminal.h"
#include "common/environment.h"
#include "common/exception/handle.h"
#include "common/execute.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/serialize/create.h"
#include "common/chronology.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/log.h"

namespace casual
{
   using namespace common;
   namespace domain
   {
      namespace manager
      {
         namespace local
         {
            namespace
            {
               namespace event
               {

                  void print( std::ostream& out, const message::event::domain::task::Begin& event)
                  {
                     out << terminal::color::blue << "task: ";
                     out << terminal::color::yellow << event.description << " - ";
                     out << terminal::color::green << "started";
                  }

                  void print( std::ostream& out, const message::event::domain::task::End& event)
                  {
                     out << terminal::color::blue << "task: ";
                     out << terminal::color::yellow << event.description << " - ";

                     auto state_color = [&event]()
                     {
                        switch( event.state)
                        {
                           using Enum = decltype( event.state);
                           case Enum::ok: return terminal::color::green;
                           case Enum::aborted: return terminal::color::magenta;
                           default: return terminal::color::red;
                        }
                     };
                     
                     out << state_color() << event.state;
                  }

                  void print( std::ostream& out, const message::event::domain::Error& event)
                  {
                     auto severity_color = [&event]()
                     {
                        switch( event.severity)
                        {
                           using Enum = decltype( event.severity);
                           case Enum::warning : return terminal::color::green;
                           case Enum::error : return terminal::color::magenta;
                           default: return terminal::color::red;
                        }
                     };

                     out << severity_color() << event.severity << ": ";
                     if( ! event.executable.empty())
                        out << terminal::color::yellow << event.executable << " ";
                     if( event.pid)
                        out << terminal::color::white << event.pid << ": ";
                     out << terminal::color::white << event.message;
                  }

                  

                  struct Done{};

                  struct Handler
                  {
                     using mapping_type = std::map< strong::process::id, std::string>;

                     Handler() = default;
                     Handler( mapping_type mapping) : m_alias_mapping{ std::move( mapping)} {}

                     void operator () ()
                     {
                        // Make sure we unsubscribe for events
                        auto unsubscribe = execute::scope( []()
                        {
                           communication::ipc::non::blocking::send(
                              communication::instance::outbound::domain::manager::optional::device(),
                              message::event::subscription::End{ process::handle()});
                        });

                        common::communication::ipc::inbound::Device::handler_type event_handler
                        {
                           [&]( message::event::process::Spawn& m)
                           {
                              group( std::cout) << "spawned: " << terminal::color::yellow << m.alias << " "
                                    << terminal::color::no_color << range::make( m.pids) << '\n';
                              for( auto pid : m.pids)
                              {
                                 m_alias_mapping[ pid] = m.alias;
                              }
                           },
                           [&]( message::event::process::Exit& m)
                           {
                              using reason_t = process::lifetime::Exit::Reason;
                              switch( m.state.reason)
                              {
                                 case reason_t::core:
                                    group( std::cout) << terminal::color::red << "core: "
                                          << terminal::color::white << m.state.pid << " " << m_alias_mapping[ m.state.pid] << '\n';
                                    break;
                                 default:
                                    group( std::cout) << terminal::color::green << "exit: "
                                       <<  terminal::color::white << m.state.pid
                                       << " " << terminal::color::yellow << m_alias_mapping[ m.state.pid] << '\n';
                                    break;
                              }

                           },
                           [&]( message::event::domain::server::Connect& m)
                           {
                              group( std::cout) << terminal::color::green << "connected: "
                                    <<  terminal::color::white << m.process.pid
                                    << " " << terminal::color::yellow << m_alias_mapping[ m.process.pid] << '\n';
                           },
                           []( message::event::domain::boot::Begin& m)
                           {
                                 std::cout << "boot domain: " << terminal::color::cyan << m.domain.name << terminal::color::no_color
                                       << " - id: " << terminal::color::yellow << m.domain.id << '\n';
                           },
                           []( message::event::domain::boot::End& m)
                           {
                              throw event::Done{};
                           },
                           []( message::event::domain::shutdown::Begin& m)
                           {
                                 std::cout << "shutdown domain: " << terminal::color::cyan << m.domain.name << terminal::color::no_color
                                       << " - id: " << terminal::color::yellow << m.domain.id << '\n';
                           },
                           []( message::event::domain::shutdown::End& m)
                           {
                              throw event::Done{};
                           },
                           []( message::event::domain::Error& event)
                           {
                              event::print( std::cerr, event);
                              std::cerr << '\n';
                           },
                           [&]( message::event::domain::Group& m)
                           {
                              using context_type = message::event::domain::Group::Context;

                              switch( m.context)
                              {
                                 case context_type::boot_start:
                                 case context_type::shutdown_start:
                                 {
                                    m_group = m.name;
                                    break;
                                 }
                                 default:
                                 {
                                    m_group.clear();
                                    break;
                                 }
                              }
                           }

                        };

                        try
                        {
                           common::message::dispatch::blocking::restriced::pump( 
                              event_handler, 
                              common::communication::ipc::inbound::device());
                        }
                        catch( const event::Done&)
                        {
                           // no-op
                        }
                        catch( const exception::signal::child::Terminate&)
                        {
                           std::cerr << terminal::color::red << "fatal";
                           std::cerr << " failed to boot domain\n";

                           // Check if we got some error events
                           common::message::dispatch::pump(
                              event_handler,
                              common::communication::ipc::inbound::device(),
                              common::communication::ipc::inbound::Device::non_blocking_policy{});

                           throw;
                        }
                     }

                  private:

                     std::ostream& group( std::ostream& out)
                     {
                        if( ! m_group.empty())
                           out << terminal::color::blue << m_group << " ";

                        return out;
                     }

                     std::map< strong::process::id, std::string> m_alias_mapping;
                     std::string m_group;
                  };

               } // event

               namespace call
               {

                  admin::model::State state()
                  {
                     serviceframework::service::protocol::binary::Call call;
                     auto reply = call( admin::service::name::state);

                     admin::model::State result;

                     reply >> CASUAL_NAMED_VALUE( result);

                     return result;
                  }


                  namespace scale
                  {
                     auto aliases( const std::vector< admin::model::scale::Alias>& aliases)
                     {
                        serviceframework::service::protocol::binary::Call call;
                        call << CASUAL_NAMED_VALUE( aliases);

                        auto reply = call( admin::service::name::scale::instances);

                        std::vector< strong::task::id> result;
                        reply >> CASUAL_NAMED_VALUE( result);

                        return result;
                     }
                  } // scale



                  namespace restart
                  {
                     auto instances( const std::vector< admin::model::restart::Alias>& aliases)
                     {
                        serviceframework::service::protocol::binary::Call call;
                        call << CASUAL_NAMED_VALUE( aliases);

                        auto reply = call( admin::service::name::restart::instances);

                        std::vector< strong::task::id> result;
                        reply >> CASUAL_NAMED_VALUE( result);

                        return result;
                     }
                     
                  } // restart

                  void boot( const std::vector< std::string>& files)
                  {
                     event::Handler events;

                     auto get_arguments = []( auto& value)
                     {
                        std::vector< std::string> arguments;

                        auto files = common::range::make( value);

                        if( ! files.empty())
                        {
                           if( ! algorithm::all_of( files, &common::file::exists))
                              throw exception::system::invalid::File{ string::compose( "at least one file does not exist - files: ", files)};
                           
                           arguments.emplace_back( "--configuration-files");
                           algorithm::append( files, arguments);
                        }

                        arguments.emplace_back( "--event-ipc");
                        arguments.emplace_back( common::string::compose( common::communication::ipc::inbound::ipc()));

                        return arguments;
                     };

                     common::process::spawn(
                        common::environment::directory::casual() + "/bin/casual-domain-manager",
                        get_arguments( files));

                     events();
                  }

                  auto get_alias_mapping()
                  {
                     auto state = call::state();

                     std::map< strong::process::id, std::string> mapping;

                     for( auto& s : state.servers)
                        for( auto& i : s.instances)
                           mapping[ i.handle.pid] = s.alias;

                     for( auto& e : state.executables)
                        for( auto& i : e.instances)
                           mapping[ i.handle] = e.alias;

                     return mapping;
                  }

                  void shutdown()
                  {
                     // subscribe for events
                     communication::ipc::non::blocking::send(
                        communication::instance::outbound::domain::manager::optional::device(),
                        message::event::subscription::Begin{ process::handle()});

                     event::Handler events{ get_alias_mapping()};

                     serviceframework::service::protocol::binary::Call{}( admin::service::name::shutdown);

                     events();
                  }

                  namespace environment
                  {
                     auto set( const admin::model::set::Environment& environment)
                     {
                        serviceframework::service::protocol::binary::Call call;
                        call << CASUAL_NAMED_VALUE( environment);

                        auto reply = call( admin::service::name::environment::set);

                        std::vector< std::string> serviceReply;

                        reply >> CASUAL_NAMED_VALUE( serviceReply);

                        return serviceReply;
                     }
                  } // environment

                  namespace configuration
                  {
                     auto get()
                     {
                        Trace trace{ "domain::manager::local::call::configuration::get"};

                        auto reply = []()
                        {  
                           Trace trace{ "domain::manager::local::call::configuration::get call"};
                           serviceframework::service::protocol::binary::Call call;
                           return call( admin::service::name::configuration::get);
                        }();
                        

                        casual::configuration::domain::Manager casual_service_reply;
                        reply >> CASUAL_NAMED_VALUE( casual_service_reply);
                        common::log::line( casual::domain::log, "casual_service_reply: ", casual_service_reply);
                        return casual_service_reply;
                     }

                     auto put( const casual::configuration::domain::Manager& domain)
                     {
                        auto reply = [&]()
                        {
                           serviceframework::service::protocol::binary::Call call;
                           call << CASUAL_NAMED_VALUE( domain);
                           return call( admin::service::name::configuration::put);
                        }();
                        
                        std::vector< manager::admin::model::Task> casual_service_reply;
                        reply >> CASUAL_NAMED_VALUE( casual_service_reply);
                        common::log::line( casual::domain::log, "casual_service_reply: ", casual_service_reply);
                        return casual_service_reply;
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
                     auto format_configured_instances = []( const P& e){
                        return e.instances.size();
                     };

                     auto format_running_instances = []( const P& e){
                        return common::algorithm::count_if( e.instances, []( auto& i){
                              return i.state == admin::model::instance::State::running;
                        });
                     };

                     auto format_restart = []( const P& e){
                        if( e.restart) 
                           return "true";
                        return "false";
                     };

                     auto format_restarts = []( const P& e){
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

               namespace action
               {
                  auto fetch_aliases()
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
      number of times instances has been restarted
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
                                 auto format_spawnpoint = []( auto& i) { return chronology::local( i.instance->spawnpoint);};

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
                                 auto format_spawnpoint = []( auto& i) { return chronology::local( i.instance->spawnpoint);};

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

                  namespace alias
                  {
                     namespace detail
                     {
                        // generalization of the event handling 
                        auto invoke = []( auto&& invocable, auto&& argument)
                        {
                           // if no-block we don't mess with events
                           if( ! terminal::output::directive().block())
                           {
                              invocable( argument);
                              return;
                           }

                           // register for events
                           auto unregister = common::event::scope::subscribe( 
                              common::process::handle(), { 
                                 message::Type::event_domain_task_end, 
                                 message::Type::event_domain_task_begin,
                                 message::Type::event_domain_error});

                           auto tasks = invocable( argument);

                           // listen for events
                           common::event::no::subscription::conditional( 
                              [ &tasks] () { return tasks.empty();}, // will end if true
                              [ &tasks]( const message::event::domain::task::Begin& task)
                              {
                                 if( auto found = algorithm::find( tasks, task.id))
                                 {
                                    event::print( std::cout, task);
                                    std::cout << '\n';
                                 }
                              },
                              [ &tasks]( const message::event::domain::task::End& task)
                              {
                                 if( auto found = algorithm::find( tasks, task.id))
                                 {
                                    event::print( std::cout, task);
                                    std::cout << '\n';
                                    
                                    // remove the task from our 'state'
                                    tasks.erase( std::begin( found));
                                 }
                              },
                              [&tasks]( const message::event::domain::Error& event)
                              {
                                 // if an error occurs - we bail out...
                                 tasks.clear();

                                 event::print( std::cerr, event);
                                 std::cerr << '\n';
                              }
                           );
                        };
                        
                     } // detail 

                     namespace scale
                     {
                        void invoke( const std::vector< std::tuple< std::string, int>>& values)
                        {   
                           auto transform = []( auto& value){
                              admin::model::scale::Alias result;
                              result.name = std::get< 0>( value);
                              result.instances = std::get< 1>( value);
                              return result;
                           };

                           detail::invoke( call::scale::aliases, common::algorithm::transform( values, transform));
                        }
                     
                        auto completion = []( auto values, bool help) -> std::vector< std::string>
                        {
                           if( help)
                              return { "<alias> <#>"};
                           
                           if( values.size() % 2 == 0)
                              return fetch_aliases();
                           else
                              return { "<value>"};
                        };

                     } // scale


                     namespace restart
                     {
                        void invoke( std::vector< std::string> values)
                        {
                           auto transform = []( auto& value){
                              //admin::model::restart::Alias result;
                              //result.name = std::move( value);
                              return admin::model::restart::Alias{ std::move( value)};
                           };

                           detail::invoke( call::restart::instances, common::algorithm::transform( values, transform));
                        }

                        auto completion = []( auto values, bool help) -> std::vector< std::string>
                        {
                           if( help)
                              return { "<alias>"};
                           
                           return fetch_aliases();
                        };
                     } // restart
                  } // alias


                  namespace environment
                  {
                     namespace set
                     {
                        void call( const std::string& name, const std::string& value, std::vector< std::string> aliases)
                        {
                           admin::model::set::Environment environment;
                           environment.variables.variables.push_back( casual::configuration::environment::Variable{ name, value});
                           environment.aliases = std::move( aliases);

                           call::environment::set( environment);
                        }

                        auto complete = []( auto values, bool help) -> std::vector< std::string>
                        {
                           if( help)
                              return { "<variable> <value> [<alias>].*"};

                           auto list_environment = []()
                           {
                              auto current = common::environment::variable::native::current();

                              auto transform_name = []( auto& variable)
                              {
                                 auto name = variable.name();
                                 return std::string( std::begin( name), std::end( name));
                              };

                              return algorithm::transform( current, transform_name);
                           };

                           switch( values.size())
                           {
                              case 0: return list_environment();
                              case 1: return { "<value>"};
                              default: return fetch_aliases();
                           }
                        };

                        constexpr auto description = R"(set an environment variable for explicit aliases
                        
if 0 aliases are provided, the environment virable will be set 
for all servers and executables 
                        )";

                     } // set
                  } // environment 


                  void boot( const std::vector< std::string>& files)
                  {
                     call::boot( files);
                  }

                  void shutdown()
                  {
                     call::shutdown();
                  }

                  namespace configuration
                  {
                     void persist()
                     {
                        serviceframework::service::protocol::binary::Call{}( admin::service::name::configuration::persist);
                     }
                     
                     void get( const common::optional< std::string>& format)
                     {
                        auto domain = call::configuration::get();
                        auto archive = common::serialize::create::writer::from( format.value_or( ""));
                        archive << CASUAL_NAMED_VALUE( domain);
                        archive.consume( std::cout);
                     }

                     namespace put
                     {
                        void call( const std::string& format)
                        {
                           casual::configuration::domain::Manager domain;
                           auto archive = common::serialize::create::reader::consumed::from( format, std::cin);
                           archive >> CASUAL_NAMED_VALUE( domain);

                           auto tasks = call::configuration::put( domain); 
                           common::log::line( casual::domain::log, "tasks: ", tasks);
                        }

                        constexpr auto description = R"(reads configuration from stdin and update the domain

The semantics are similar to http PUT:
 * every key that is found is treated as an update of that _entity_
 * every key that is NOT found is treated as a new _entity_ and added to the current state 
)";    
                     } // put

                  } // configuration


                  void state( const common::optional< std::string>& format)
                  {
                     auto state = call::state();
                     auto archive = common::serialize::create::writer::from( format.value_or( ""));
                     archive << CASUAL_NAMED_VALUE( state);
                     archive.consume( std::cout);
                  }

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
                        void invoke( platform::process::native::type pid, const optional< std::string>& format)
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
                                       []( auto& instance){ return ! instance.handle.pid.empty();});
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


               } // action

            } // <unnamed>
         } // local

         namespace admin 
         {
            struct cli::Implementation
            {
               argument::Group options()
               {
                  auto state_format = serialize::create::writer::complete::format();
                  auto configuration_format = serialize::create::reader::complete::format();

                  return argument::Group{ [](){}, { "domain"}, "local casual domain related administration",
                     argument::Option( &local::action::list::servers::invoke, { "-ls", "--list-servers"}, local::action::list::servers::description),
                     argument::Option( &local::action::list::executables::invoke, { "-le", "--list-executables"}, local::action::list::executables::description),
                     argument::Option( argument::option::one::many( &local::action::alias::scale::invoke), local::action::alias::scale::completion, { "-si", "--scale-instances"}, "<alias> <#> scale executable instances"),
                     argument::Option( argument::option::one::many( &local::action::alias::restart::invoke), local::action::alias::restart::completion, { "-ri", "--restart-instances"}, "<alias> restart instances for the given aliases"),
                     argument::Option( &local::action::list::instances::server::invoke, { "-lis", "--list-instances-server"}, local::action::list::instances::server::description),
                     argument::Option( &local::action::list::instances::executable::invoke, { "-lie", "--list-instances-executable"}, local::action::list::instances::executable::description),
                     argument::Option( &local::action::shutdown, { "-s", "--shutdown"}, "shutdown the domain"),
                     argument::Option( &local::action::boot, { "-b", "--boot"}, "boot domain -"),
                     argument::Option( &local::action::environment::set::call, local::action::environment::set::complete, { "--set-environment"}, local::action::environment::set::description)( argument::cardinality::any{}),
                     argument::Option( &local::action::configuration::persist, { "-p", "--persist-state"}, "persist current state"),
                     argument::Option( &local::action::configuration::get, configuration_format, { "--configuration-get"}, "get configuration (as provided format)"),
                     argument::Option( &local::action::configuration::put::call, configuration_format, { "--configuration-put"}, local::action::configuration::put::description),
                     argument::Option( argument::option::one::many( &local::action::ping::invoke), local::action::ping::complete(), { "--ping"}, local::action::ping::description),
                     argument::Option( &local::action::global::state::invoke, local::action::global::state::complete(), { "--instance-global-state"}, local::action::global::state::description),
                     argument::Option( &local::action::legend::invoke, local::action::legend::complete(), { "--legend"}, local::action::legend::description),
                     argument::Option( &local::action::state, state_format, { "--state"}, "domain state (as provided format)")
                  };
               }
            };

            cli::cli() = default; 
            cli::~cli() = default; 

            common::argument::Group cli::options() &
            {
               return m_implementation->options();
            }
            
         } // admin 
      } // manager
   } // domain
} // casual










