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

         struct Instance
         {
            std::string alias;
            std::string path;
            sf::platform::pid_type pid = 0;
            sf::platform::queue_id_type queue = 0;
            long state = 0;
            long invoked = 0;
            sf::platform::time_type last = sf::platform::time_type::min();

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
         };

         struct Service
         {
            std::string name;
            long long timeout = 0;
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
                  value.state = instance.state;
                  value.invoked = instance.invoked;
                  value.last = instance.last;

                  result.push_back( std::move( value));
               }
            }
         }
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
               value.timeout = service.timeout;
               value.instances = service.instances.size();
               value.pids = common::range::to_string( service.instances);

               result.push_back( std::move( value));
            }

         return result;
      }


      namespace global
      {
         bool porcelain = false;
         bool header = false;
      } // global


      namespace print
      {
         template< typename C, typename M>
         std::size_t column( const C& container, M&& member)
         {
            using value_type = decltype( *container.begin());
            auto size_member = std::mem_fn( member);
            auto max = std::max_element( std::begin( container), std::end( container),
                  [&]( const value_type& lhs, const value_type& rhs){
                     return size_member( lhs).size() < size_member( rhs).size();
            });
            return size_member( max).size();
         }

         std::ostream& operator << ( std::ostream& out, state::Server::Instance::State state)
         {
            switch( state)
            {
               case state::Server::Instance::State::absent: return out << common::terminal::color::red << "absent"; break;
               case state::Server::Instance::State::prospect: return out << common::terminal::color::blue << "prospect"; break;
               case state::Server::Instance::State::idle: return out << common::terminal::color::green << "idle"; break;
               case state::Server::Instance::State::busy: return out << common::terminal::color::yellow << "busy"; break;
               case state::Server::Instance::State::shutdown: return out << common::terminal::color::red << "shutdown"; break;
               default: return out << common::terminal::color::red << "off-line";
            }
         }

         template< typename C>
         void instances( C&& container, common::terminal::color_t& main = common::terminal::color::green)
         {
            auto alias_column = column( container, &normalized::Instance::alias);

            if( container.empty())
            {
               return;
            }

            range::sort( container);

            if( global::header)
            {
               std::cout << terminal::color::white << std::setw( alias_column) << std::setfill( ' ') << std::left << "alias"
                  << std::setw( 12) << std::setfill( ' ')  << std::right  << "pid"
                  << std::setw( 11) << std::right << "queue"
                  << std::setw( 11) << std::setfill( ' ') << std::right << "invoked"
                  << "  " << std::setw( 9) << std::setfill( ' ') << std::left << "state"
                  << "  " << std::setw( 24) << std::setfill( ' ') << std::left << "accessed"
                  << "path"
                  << std::endl;

               std::cout << terminal::color::white << std::setw( alias_column + 1) << std::setfill( '-') << std::right << " "
                  << std::setw( 11) << "----------- ---------- ----------  ---------- ----------------------- -------------------------"
                  << std::endl;
            }


            for( auto& instance : container)
            {
               std::cout << main << std::setw( alias_column + 1) << std::setfill( ' ') << std::left << instance.alias;
               std::cout << terminal::color::white << std::setw( 11) << std::setfill( ' ') << std::right << ( instance.pid == 0 ? "---" : std::to_string( instance.pid));
               std::cout << std::setw( 11) << ( instance.queue == 0 ? "---" : std::to_string( instance.queue));
               std::cout << terminal::color::blue << std::setw( 11) << instance.invoked;
               std::cout << std::setw( 9) << std::left << std::setfill( ' ') << static_cast< state::Server::Instance::State>( instance.state);
               std::cout << terminal::color::blue << std::setw( 25) << std::setfill( ' ') << std::right << ( instance.last == common::platform::time_point::min() ? "---" : common::chronology::local( instance.last));
               std::cout << " " << instance.path;
               std::cout << std::endl;
            }
         }

         template< typename C>
         void services( C&& container, common::terminal::color_t& main = common::terminal::color::yellow)
         {
            if( container.empty())
            {
               return;
            }

            range::sort( container);

            auto name_column = column( container, &normalized::Service::name);

            if( global::header)
            {
               std::cout << terminal::color::white << std::setw( name_column) << std::setfill( ' ') << std::left << "name"
                  << std::setw( 12) << std::setfill( ' ')  << std::right  << "lookedup"
                  << " " << std::setw( 11) << std::right << "timeout"
                  << " " << std::setw( 6) << std::setfill( ' ') << std::right << "#"
                  << " " << "servers"
                  << std::endl;

               std::cout << terminal::color::white << std::setw( name_column + 1) << std::setfill( '-') << " "
                  << std::setw( 11) << "----------- ----------- ------ -------------------------"
                  << std::endl;
            }

            for( auto& service : container)
            {
               using second_t = std::chrono::duration< double>;
               std::cout << main << std::setw( name_column) << std::setfill( ' ') << std::left << service.name << " ";
               std::cout << terminal::color::blue << std::setw( 11) << std::right  << service.lookedup << " ";
               std::cout << std::setw( 11) << std::chrono::duration_cast< second_t>( std::chrono::microseconds{ service.timeout}).count() << " ";
               std::cout << std::setw( 6) << std::setfill( ' ') << std::right << terminal::color::white << service.instances << " ";
               std::cout << service.pids;
               std::cout << std::endl;
            }
         }


      } // print




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
            sf::archive::terminal::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( instances);

         }
         else
         {
            print::instances( instances);
         }
      }

      void listServices()
      {
         auto services = normalize( call::services());

         if( global::porcelain)
         {
            sf::archive::terminal::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( services);

         }
         else
         {
            print::services( services);
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
            inst.state = static_cast< long>( state::Server::Instance::State::shutdown);
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
               sf::archive::terminal::Writer writer{ std::cout};
               writer << CASUAL_MAKE_NVP( common::range::to_vector( offline));
            }

            {
               sf::archive::terminal::Writer writer{ std::cerr};
               writer << CASUAL_MAKE_NVP( common::range::to_vector( online));
            }
         }
         else
         {
            print::instances( online);
            print::instances( offline);
         }
      }

      void boot()
      {
         auto instances = normalize( call::boot());

         if( global::porcelain)
         {
            sf::archive::terminal::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( instances);

         }
         else
         {
            print::instances( instances);
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
      std::cerr << exception.what() << std::endl;
   }


   return 0;
}


