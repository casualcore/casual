//!
//! casual
//!

#include "domain/manager/admin/vo.h"

#include "common/arguments.h"
#include "common/terminal.h"
#include "common/environment.h"

#include "sf/xatmi_call.h"

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

               namespace call
               {

                  admin::vo::State state()
                  {
                     sf::xatmi::service::binary::Sync service( ".casual.domain.state");
                     auto reply = service();

                     admin::vo::State serviceReply;

                     reply >> CASUAL_MAKE_NVP( serviceReply);

                     return serviceReply;
                  }


                  std::vector< admin::vo::scale::Instances> scale_instances( const std::vector< admin::vo::scale::Instances>& instances)
                  {
                     sf::xatmi::service::binary::Sync service( ".casual.domain.scale.instances");

                     service << CASUAL_MAKE_NVP( instances);
                     auto reply = service();

                     std::vector< admin::vo::scale::Instances> serviceReply;

                     reply >> CASUAL_MAKE_NVP( serviceReply);

                     return serviceReply;
                  }

                  void boot( const std::vector< std::string>& files)
                  {
                     std::vector< std::string> arguments{ "--configuration-files"};

                     range::transform( files, arguments, &common::file::absolute);

                     common::process::spawn(
                           common::environment::variable::get( common::environment::variable::name::home()) + "/bin/casual-domain-manager",
                           arguments);
                  }

                  void shutdown()
                  {
                     sf::xatmi::service::binary::Sync service( ".casual.domain.shutdown");
                     auto reply = service();
                  }
               } // call

               namespace format
               {

                  terminal::format::formatter< admin::vo::Executable> executables()
                  {

                     auto format_no_of_instances = []( const admin::vo::Executable& e){
                        return e.instances.size();
                     };

                     auto format_restart = []( const admin::vo::Executable& e){
                        if( e.restart) { return "true";}
                        return "false";
                     };

                     auto format_restarts = []( const admin::vo::Executable& e){
                        return e.restarts;
                     };


                     return {
                        { global::porcelain, ! global::no_color, ! global::no_header},
                        terminal::format::column( "alias", std::mem_fn( &admin::vo::Executable::alias), terminal::color::yellow, terminal::format::Align::left),
                        terminal::format::column( "instances", format_no_of_instances, terminal::color::white, terminal::format::Align::right),
                        terminal::format::column( "#c", std::mem_fn( &admin::vo::Executable::configured_instances), terminal::color::blue, terminal::format::Align::right),
                        terminal::format::column( "restart", format_restart, terminal::color::blue, terminal::format::Align::right),
                        terminal::format::column( "#r", format_restarts, terminal::color::red, terminal::format::Align::right),
                        terminal::format::column( "path", std::mem_fn( &admin::vo::Executable::path), terminal::color::blue, terminal::format::Align::left),
                     };
                  }
               } // format

               namespace print
               {

                  void executables( std::ostream& out, admin::vo::State& state)
                  {
                     out << std::boolalpha;

                     auto formatter = format::executables();

                     formatter.print( std::cout, range::sort(
                           state.executables,
                           []( const admin::vo::Executable& l, const admin::vo::Executable& r){ return l.id < r.id;}));
                  }

               } // print

               namespace action
               {
                  void list_instances()
                  {
                     auto state = call::state();

                     print::executables( std::cout, state);
                  }

                  void list_executable()
                  {
                     auto state = call::state();

                     print::executables( std::cout, state);
                  }

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
                        sf::xatmi::service::binary::Sync{ ".casual/domain/configuration/persist"}();
                     }
                  } // persist


               } // action

            } // <unnamed>
         } // local



         int main( int argc, char** argv)
         {
            common::Arguments parser{ {
                  common::argument::directive( {"--porcelain"}, "easy to parse format", local::global::porcelain),
                  common::argument::directive( {"--no-color"}, "no color will be used", local::global::no_color),
                  common::argument::directive( {"--no-header"}, "no descriptive header for each column will be used", local::global::no_header),

                  common::argument::directive( {"-le", "--list-executables"}, "list all executables", &local::action::list_executable),
                  common::argument::directive( {"-li", "--list-instances"}, "list all instances", &local::action::list_instances),
                  common::argument::directive( {"-si", "--scale-instances"}, "<alias> <#> scale executable instances", &local::action::scale_instances),
                  common::argument::directive( {"-s", "--shutdown"}, "shutdown the domain", &local::action::shutdown),
                  common::argument::directive( common::argument::cardinality::Any{}, {"-b", "--boot"}, "boot domain", &local::action::boot),
                  common::argument::directive( {"-p", "--persist-configuration"}, "persist current configuration", &local::action::persist::configuration)
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








