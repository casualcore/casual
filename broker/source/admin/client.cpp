//!
//! broker_admin.cpp
//!
//! Created on: Jul 9, 2013
//!     Author: Lazan
//!


#include "sf/xatmi_call.h"
#include "sf/archive/terminal.h"
#include "sf/namevaluepair.h"

#include "broker/admin/brokervo.h"

#include "common/file.h"
#include "common/arguments.h"
#include "common/chronology.h"
#include "common/terminal.h"


#include "broker/broker.h"

//
// std
//
#include <iostream>
#include <iomanip>
#include <limits>




namespace casual
{

   using namespace common;

   namespace broker
   {

      namespace normalized
      {
         namespace color
         {
            struct Solid
            {
               Solid( common::terminal::color_t& color) : m_color( color) {}

               void operator () ( std::ostream& out, const std::string& value)
               {
                  out << m_color << value;
               }
               common::terminal::color_t m_color;
            };

         } // color


         struct Instance
         {
            std::string alias;
            std::string path;
            sf::platform::pid_type pid = 0;
            sf::platform::queue_id_type queue = 0;
            std::string state = "off-line";
            long invoked = 0;
            std::string last = "---";

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( alias);
               archive & CASUAL_MAKE_NVP( pid);
               archive & CASUAL_MAKE_NVP( queue);
               archive & CASUAL_MAKE_NVP( state);
               archive & CASUAL_MAKE_NVP( invoked);
               archive & CASUAL_MAKE_NVP( last);
               archive & CASUAL_MAKE_NVP( path);
            })

            friend bool operator < ( const Instance& lhs, const Instance& rhs) { return lhs.alias < rhs.alias;}

            static std::vector< sf::archive::terminal::Directive> directive()
            {
               auto state_color = []( std::ostream& out, const std::string& value)
               {
                  if( value == "busy") { out << common::terminal::color::yellow << value; return;}
                  if( value == "idle") { out << common::terminal::color::green << value; return;}
                  if( value == "shutdown" || value == "off-line") { out << common::terminal::color::red << value; return;}
                  out << value;
                  return;
               };

               return {
                  { "alias", color::Solid{ common::terminal::color::green}},
                  { "pid", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::white}},
                  { "queue", sf::archive::terminal::Directive::Align::right},
                  { "state", sf::archive::terminal::Directive::Align::left, state_color},
                  { "invoked", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::blue}},
                  { "last", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::blue}},
               };
            }

         };

         struct Service
         {
            std::string name;
            std::string timeout;
            long instances = 0;
            std::string pids;
            long lookedup = 0;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive << CASUAL_MAKE_NVP( name);
               archive << CASUAL_MAKE_NVP( timeout);
               archive << CASUAL_MAKE_NVP( lookedup);
               archive << CASUAL_MAKE_NVP( instances);
               archive << CASUAL_MAKE_NVP( pids);
            })

            friend bool operator < ( const Service& lhs, const Service& rhs) { return lhs.name < rhs.name;}

            static std::vector< sf::archive::terminal::Directive> directive()
            {
               return {
                  { "name", color::Solid{ common::terminal::color::yellow}},
                  { "timeout", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::blue}},
                  { "lookedup", sf::archive::terminal::Directive::Align::right},
                  { "instances", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::white}},
                  //{ "pids", common::terminal::color::blue},
               };
            }
         };

      } // normalized

      std::vector< normalized::Instance> normalize( const std::vector< admin::ServerVO>& servers)
      {
         std::vector< normalized::Instance> result;

         for( auto& server : servers)
         {
            if( server.instances.empty())
            {
               normalized::Instance value;

               value.alias = server.alias;
               value.path = server.path;

               result.push_back( std::move( value));
            }
            else
            {
               for( auto& instance : server.instances)
               {
                  normalized::Instance value;

                  value.alias = server.alias;
                  value.path = server.path;
                  value.pid = instance.pid;
                  value.queue = instance.queue;


                  value.invoked = instance.invoked;

                  if( instance.last != sf::platform::time_type::min())
                  {
                     value.last = common::chronology::local( instance.last);
                  }

                  auto state = []( long state){
                     switch( static_cast< state::Server::Instance::State>( state))
                     {
                        case state::Server::Instance::State::absent: return "absent"; break;
                        case state::Server::Instance::State::prospect: return "prospect"; break;
                        case state::Server::Instance::State::idle: return "idle"; break;
                        case state::Server::Instance::State::busy: return "busy"; break;
                        case state::Server::Instance::State::shutdown: return "shutdown"; break;
                        default: return "off-line";
                     }
                  };

                  value.state = state( instance.state);

                  result.push_back( std::move( value));
               }
            }
         }

         range::sort( result);

         return result;
      }

      std::vector< normalized::Service> normalize( const std::vector< admin::ServiceVO>& services)
      {
         std::vector< normalized::Service> result;

         for( auto& service : services)
         {
            normalized::Service value;

            value.name = service.name;
            value.lookedup = service.lookedup;

            {
               using second_t = std::chrono::duration< double>;
               std::stringstream out;
               out << std::chrono::duration_cast< second_t>( std::chrono::microseconds{ service.timeout}).count();
               value.timeout = out.str();
            }

            value.instances = service.instances.size();
            value.pids = common::range::to_string( service.instances);

            result.push_back( std::move( value));
         }

         range::sort( result);

         return result;
      }


      namespace global
      {
         bool porcelain = false;
         bool header = false;

         bool colors = true;

         void no_colors() { colors = false;}

      } // global





      namespace call
      {
         std::vector< admin::ServerVO> servers()
         {

            sf::xatmi::service::binary::Sync service( ".casual.broker.list.servers");
            auto reply = service();

            std::vector< admin::ServerVO> serviceReply;

            reply >> CASUAL_MAKE_NVP( serviceReply);

            return serviceReply;
         }

         std::vector< admin::ServiceVO> services()
         {
            sf::xatmi::service::binary::Sync service( ".casual.broker.list.services");
            auto reply = service();

            std::vector< admin::ServiceVO> serviceReply;

            reply >> CASUAL_MAKE_NVP( serviceReply);

            return serviceReply;
         }

         admin::ShutdownVO shutdown()
         {
            sf::xatmi::service::binary::Sync service( ".casual.broker.shutdown");

            bool broker = true;
            service << CASUAL_MAKE_NVP( broker);

            auto reply = service();

            admin::ShutdownVO serviceReply;

            reply >> CASUAL_MAKE_NVP( serviceReply);

            return serviceReply;
         }

         std::vector< admin::ServerVO> boot()
         {
            common::process::spawn( common::environment::variable::get( "CASUAL_HOME") + "/bin/casual-broker", {});

            common::process::sleep( std::chrono::milliseconds{ 20});

            return call::servers();
         }

      }






      void listServers()
      {
         auto instances = normalize( call::servers());

         if( global::porcelain)
         {
            sf::archive::terminal::percelain::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( instances);

         }
         else
         {
            sf::archive::terminal::Writer writer{ std::cout, normalized::Instance::directive(), global::header, global::colors};
            writer << CASUAL_MAKE_NVP( instances);
         }
      }

      void listServices()
      {
         auto services = normalize( call::services());

         if( global::porcelain)
         {
            sf::archive::terminal::percelain::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( services);

         }
         else
         {
            sf::archive::terminal::Writer writer{ std::cout, normalized::Service::directive(), global::header, global::colors};
            writer << CASUAL_MAKE_NVP( services);
         }
      }


      void updateInstances( const std::vector< std::string>& values)
      {
         if( values.size() == 2)
         {
            admin::update::InstancesVO instance;

            instance.alias = values[ 0];
            instance.instances = std::stoul( values[ 1]);

            sf::xatmi::service::binary::Sync service( ".casual.broker.update.instances");

            service << CASUAL_MAKE_NVP( std::vector< admin::update::InstancesVO>{ instance});

            service();

         }
      }


      void shutdown()
      {
         auto instances = normalize( call::servers());

         auto result = call::shutdown();

         auto parts = common::range::intersection( instances, result.offline,
               []( const normalized::Instance& inst, common::platform::pid_type pid) {
                  return inst.pid == pid;
         });


         auto offline = std::get< 0>( parts);

         //
         // Set these to off-line
         //
         common::range::for_each( offline, []( normalized::Instance& inst){
            inst.state = "shutdown";
         });

         auto online = std::get< 0>( common::range::intersection( std::get< 1>( parts), result.online,
               []( const normalized::Instance& inst, common::platform::pid_type pid) {
                  return inst.pid == pid;
         }));

         //
         // Remove casual-broker, since it always will be online when it replied this information,
         // Though, it will probably be offline by now anyway.
         //
         online = std::get< 0>( common::range::partition( online,
               []( const normalized::Instance& inst){
                  return inst.alias != "casual-broker";
         }));

         if( global::porcelain)
         {
            {
               sf::archive::terminal::percelain::Writer writer{ std::cout};
               writer << CASUAL_MAKE_NVP( common::range::to_vector( range::sort( offline)));
            }

            {
               sf::archive::terminal::percelain::Writer writer{ std::cerr};
               writer << CASUAL_MAKE_NVP( common::range::to_vector( range::sort( online)));
            }
         }
         else
         {
            sf::archive::terminal::Writer writer{ std::cout, normalized::Instance::directive(), global::header, global::colors};

            writer << CASUAL_MAKE_NVP( common::range::to_vector( range::sort( online)));
            writer << CASUAL_MAKE_NVP( common::range::to_vector( range::sort( offline)));
         }
      }

      void boot()
      {
         auto instances = normalize( call::boot());

         if( global::porcelain)
         {
            sf::archive::terminal::percelain::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( instances);

         }
         else
         {
            sf::archive::terminal::Writer writer{ std::cout, normalized::Instance::directive(), global::header, global::colors};
            writer << CASUAL_MAKE_NVP( instances);
         }
      }

   } // broker
} // casual



int usage( int argc, char** argv)
{
   std::cerr << "usage:\n  " << argv[ 0] << " (--list-servers | --list-services | -update-instances)" << std::endl;
   return 2;
}



int main( int argc, char** argv)
{

   casual::common::Arguments parser;
   parser.add(
         casual::common::argument::directive( {"--header"}, "descriptive header for each column", casual::broker::global::header),
         casual::common::argument::directive( {"--porcelain"}, "easy to parse format", casual::broker::global::porcelain),
         casual::common::argument::directive( {"--no-color"}, "no color will be used", &casual::broker::global::no_colors),
         casual::common::argument::directive( {"-lsvr", "--list-servers"}, "list all servers", &casual::broker::listServers),
         casual::common::argument::directive( {"-lsvc", "--list-services"}, "list all services", &casual::broker::listServices),
         casual::common::argument::directive( {"-ui", "--update-instances"}, "<alias> <#> update server instances", &casual::broker::updateInstances),
         casual::common::argument::directive( {"-s", "--shutdown"}, "shutdown the domain", &casual::broker::shutdown),
         casual::common::argument::directive( {"-b", "--boot"}, "boot domain", &casual::broker::boot)
   );




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


