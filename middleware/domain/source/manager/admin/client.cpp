//!
//! casual
//!

#include "common/event/listen.h"
#include "domain/manager/admin/vo.h"
#include "domain/manager/admin/server.h"

#include "common/arguments.h"
#include "common/terminal.h"
#include "common/environment.h"

#include "common/communication/ipc.h"

#include "sf/service/protocol/call.h"
#include "sf/archive/maker.h"

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
               namespace global
               {
                  bool porcelain = false;
                  bool no_color = false;
                  bool no_header = false;

               } // global

               namespace event
               {
                  struct Done{};

                  struct Handler
                  {
                     using mapping_type = std::map< platform::pid::type, std::string>;

                     Handler() = default;
                     Handler( mapping_type mapping) : m_alias_mapping{ std::move( mapping)} {}

                     void operator () ()
                     {
                        //
                        // Make sure we unsubscribe for events
                        //
                        auto unsubscribe = scope::execute( [](){
                           message::event::subscription::End message;
                           message.process = process::handle();

                           communication::ipc::non::blocking::send(
                                 communication::ipc::domain::manager::optional::device(),
                                 message);
                        });

                        common::communication::ipc::inbound::Device::handler_type event_handler{
                           [&]( message::event::process::Spawn& m){
                              group( std::cout) << "spawned: " << terminal::color::yellow << m.alias << " "
                                    << terminal::color::no_color << range::make( m.pids) << std::endl;
                              for( auto pid : m.pids)
                              {
                                 m_alias_mapping[ pid] = m.alias;
                              }
                           },
                           [&]( message::event::process::Exit& m){
                              using reason_t = process::lifetime::Exit::Reason;
                              switch( m.state.reason)
                              {
                                 case reason_t::core:
                                    group( std::cout) << terminal::color::red << "core: "
                                          << terminal::color::white << m.state.pid << " " << m_alias_mapping[ m.state.pid] << std::endl;
                                    break;
                                 default:
                                    group( std::cout) << terminal::color::green << "exit: "
                                       <<  terminal::color::white << m.state.pid
                                       << " " << terminal::color::yellow << m_alias_mapping[ m.state.pid] << std::endl;
                                    break;
                              }

                           },
                           [&]( message::event::domain::server::Connect& m){

                              group( std::cout) << terminal::color::green << "connected: "
                                    <<  terminal::color::white << m.process.pid
                                    << " " << terminal::color::yellow << m_alias_mapping[ m.process.pid] << std::endl;


                           },
                           []( message::event::domain::boot::Begin& m){
                                 std::cout << "boot domain: " << terminal::color::cyan << m.domain.name << terminal::color::no_color
                                       << " - id: " << terminal::color::yellow << m.domain.id << std::endl;
                           },
                           []( message::event::domain::boot::End& m){
                              throw event::Done{};
                           },
                           []( message::event::domain::shutdown::Begin& m){
                                 std::cout << "shutdown domain: " << terminal::color::cyan << m.domain.name << terminal::color::no_color
                                       << " - id: " << terminal::color::yellow << m.domain.id << std::endl;
                           },
                           []( message::event::domain::shutdown::End& m){
                              throw event::Done{};
                           },
                           []( message::event::domain::Error& m)
                           {

                              auto print_error = []( const message::event::domain::Error& m){
                                 std::cerr << terminal::color::yellow << m.executable << " "
                                       << terminal::color::white << m.pid << ": "
                                       << terminal::color::white << m.message
                                       << '\n';
                              };

                              switch( m.severity)
                              {
                                 case message::event::domain::Error::Severity::fatal:
                                 {
                                    std::cerr << terminal::color::red << "fatal: ";
                                    print_error( m);
                                    throw Done{};
                                 }
                                 case message::event::domain::Error::Severity::error:
                                 {
                                    std::cerr << terminal::color::red << "error: ";
                                    print_error( m);
                                    break;
                                 }
                                 default:
                                 {
                                    std::cerr << terminal::color::magenta << "warning ";
                                    print_error( m);
                                 }
                              }



                           },
                           [&]( message::event::domain::Group& m){
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
                           common::message::dispatch::blocking::pump( event_handler, common::communication::ipc::inbound::device());
                        }
                        catch( const event::Done&)
                        {
                           // no-op
                        }
                        catch( const exception::signal::child::Terminate&)
                        {
                           std::cerr << terminal::color::red << "fatal";
                           std::cerr << " failed to boot domain\n";

                           //
                           // Check if we got some error events
                           //
                           common::message::dispatch::pump(
                                 event_handler,
                                 common::communication::ipc::inbound::device(),
                                 common::communication::ipc::inbound::Device::non_blocking_policy{});

                        }
                        catch( ...)
                        {
                           error::handler();
                        }
                     }

                  private:

                     std::ostream& group( std::ostream& out)
                     {
                        if( ! m_group.empty())
                        {
                           out << terminal::color::blue << m_group << " ";
                        }
                        return out;
                     }

                     std::map< platform::pid::type, std::string> m_alias_mapping;
                     std::string m_group;
                  };

               } // event

               namespace call
               {

                  admin::vo::State state()
                  {
                     sf::service::protocol::binary::Call call;
                     auto reply = call( admin::service::name::state());

                     admin::vo::State serviceReply;

                     reply >> CASUAL_MAKE_NVP( serviceReply);

                     return serviceReply;
                  }


                  std::vector< admin::vo::scale::Instances> scale_instances( const std::vector< admin::vo::scale::Instances>& instances)
                  {
                     sf::service::protocol::binary::Call call;
                     call << CASUAL_MAKE_NVP( instances);

                     auto reply = call( admin::service::name::scale::instances());

                     std::vector< admin::vo::scale::Instances> serviceReply;

                     reply >> CASUAL_MAKE_NVP( serviceReply);

                     return serviceReply;
                  }

                  void boot( const std::vector< std::string>& files)
                  {

                     event::Handler events;

                     auto get_arguments = []( auto& value){
                        std::vector< std::string> arguments;

                        auto files = common::range::make( value);

                        if( ! files.empty())
                        {
                           if( ! range::all_of( files, &common::file::exists))
                           {
                              throw exception::invalid::File{ "at least one file does not exist", CASUAL_NIP( files)};
                           }
                           arguments.emplace_back( "--configuration-files");
                           range::copy( files, std::back_inserter( arguments));
                        }

                        arguments.emplace_back( "--event-queue");
                        arguments.emplace_back( std::to_string( common::communication::ipc::inbound::id()));

                        return arguments;
                     };

                     common::process::spawn(
                           common::environment::variable::get( common::environment::variable::name::home()) + "/bin/casual-domain-manager",
                           get_arguments( files));

                     events();
                  }

                  auto get_alias_mapping()
                  {
                     auto state = call::state();

                     std::map< platform::pid::type, std::string> mapping;

                     for( auto& s : state.servers)
                     {
                        for( auto& i : s.instances)
                        {
                           mapping[ i.handle.pid] = s.alias;
                        }
                     }

                     for( auto& e : state.executables)
                     {
                        for( auto& i : e.instances)
                        {
                           mapping[ i.handle] = e.alias;
                        }
                     }
                     return mapping;
                  }

                  void shutdown()
                  {

                     //
                     // subscribe for events
                     //
                     {
                        message::event::subscription::Begin message;
                        message.process = process::handle();

                        communication::ipc::non::blocking::send(
                              communication::ipc::domain::manager::optional::device(),
                              message);
                     }

                     event::Handler events{ get_alias_mapping()};

                     sf::service::protocol::binary::Call{}( admin::service::name::shutdown());

                     events();
                  }
               } // call

               namespace format
               {

                  template< typename P>
                  terminal::format::formatter< P> process()
                  {

                     auto format_no_of_instances = []( const P& e){
                        return e.instances.size();
                     };

                     auto format_restart = []( const P& e){
                        if( e.restart) { return "true";}
                        return "false";
                     };

                     auto format_restarts = []( const P& e){
                        return e.restarts;
                     };


                     return {
                        { global::porcelain, ! global::no_color, ! global::no_header},
                        terminal::format::column( "alias", std::mem_fn( &P::alias), terminal::color::yellow, terminal::format::Align::left),
                        terminal::format::column( "instances", format_no_of_instances, terminal::color::white, terminal::format::Align::right),
                        //terminal::format::column( "#c", std::mem_fn( &P::configured_instances), terminal::color::blue, terminal::format::Align::right),
                        terminal::format::column( "restart", format_restart, terminal::color::blue, terminal::format::Align::right),
                        terminal::format::column( "#r", format_restarts, terminal::color::red, terminal::format::Align::right),
                        terminal::format::column( "path", std::mem_fn( &P::path), terminal::color::blue, terminal::format::Align::left),
                     };
                  }
               } // format

               namespace print
               {

                  template< typename VO>
                  void processes( std::ostream& out, std::vector< VO>& processes)
                  {
                     out << std::boolalpha;

                     auto formatter = format::process< VO>();

                     formatter.print( std::cout, range::sort( processes));
                  }

               } // print

               namespace action
               {
                  void list_instances()
                  {
                     auto state = call::state();

                     //print::executables( std::cout, state);
                  }

                  void list_executable()
                  {
                     auto state = call::state();

                     print::processes( std::cout, state.executables);
                  }

                  void list_servers()
                  {
                     auto state = call::state();

                     print::processes( std::cout, state.servers);
                  }

                  /*
                  void list_processes()
                  {
                     auto state = call::state();

                     print::processes( std::cout, state.servers);
                  }
                  */


                  void scale_instances( const std::vector< std::string>& values)
                  {
                     if( values.size() % 2 != 0)
                     {
                        throw exception::invalid::Argument{ "<alias> <# of instances>"};
                     }

                     std::vector< admin::vo::scale::Instances> instances;

                     auto current = std::begin( values);

                     for( ; current != std::end( values); current += 2)
                     {
                        admin::vo::scale::Instances instance;
                        instance.alias = *current;
                        instance.instances = std::stoul(*( current + 1));
                        instances.push_back( std::move( instance));
                     }

                     call::scale_instances( instances);
                  }

                  void boot( const std::vector< std::string>& files)
                  {
                     call::boot( files);
                  }

                  void shutdown()
                  {
                     call::shutdown();
                  }

                  namespace persist
                  {
                     void configuration()
                     {
                        sf::service::protocol::binary::Call{}( admin::service::name::configuration::persist());
                     }
                  } // persist


                  void state( const std::string& format)
                  {
                     auto state = call::state();

                     auto archive = sf::archive::writer::from::name( std::cout, format);

                     archive << CASUAL_MAKE_NVP( state);
                  }


               } // action

            } // <unnamed>
         } // local



         int main( int argc, char** argv)
         {
            common::Arguments parser{ {
                  common::argument::directive( {"--porcelain"}, "easy to parse format", local::global::porcelain),
                  common::argument::directive( {"--no-color"}, "no color will be used", local::global::no_color),
                  common::argument::directive( {"--no-header"}, "no descriptive header for each column will be used", local::global::no_header),

                  common::argument::directive( {"--state"}, "domain state in the provided format (xml|json|yaml|ini)", &local::action::state),
                  common::argument::directive( {"-ls", "--list-servers"}, "list all servers", &local::action::list_servers),
                  common::argument::directive( {"-le", "--list-executables"}, "list all executables", &local::action::list_executable),
                  common::argument::directive( {"-li", "--list-instances"}, "list all instances", &local::action::list_instances),
                  common::argument::directive( {"-si", "--scale-instances"}, "<alias> <#> scale executable instances", &local::action::scale_instances),
                  common::argument::directive( {"-s", "--shutdown"}, "shutdown the domain", &local::action::shutdown),
                  common::argument::directive( common::argument::cardinality::Any{}, {"-b", "--boot"}, "boot domain", &local::action::boot),
                  common::argument::directive( {"-p", "--persist-state"}, "persist current state", &local::action::persist::configuration)
                }
            };

            try
            {
               parser.parse( argc, argv);

            }
            catch( const std::exception& exception)
            {
               std::cerr << "error: " << exception.what() << std::endl;
            }


            return 0;
         }

      } // manager

   } // domain

} // casual


int main( int argc, char **argv)
{
   return casual::domain::manager::main( argc, argv);
}








