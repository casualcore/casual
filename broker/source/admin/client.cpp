//!
//! broker_admin.cpp
//!
//! Created on: Jul 9, 2013
//!     Author: Lazan
//!


#include "sf/xatmi_call.h"
#include "sf/namevaluepair.h"

#include "broker/admin/brokervo.h"

#include "common/file.h"
#include "common/arguments.h"
#include "common/chronology.h"
#include "common/terminal.h"
#include "common/server/service.h"


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

      namespace global
      {
         bool porcelain = false;

         bool no_colors = false;
         bool no_header = false;

      } // global



      namespace call
      {

         admin::StateVO state()
         {
            sf::xatmi::service::binary::Sync service( ".casual.broker.state");
            auto reply = service();

            admin::StateVO serviceReply;

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

         admin::StateVO boot()
         {
            common::process::spawn( common::environment::variable::get( "CASUAL_HOME") + "/bin/casual-broker", {});

            common::process::sleep( std::chrono::milliseconds{ 20});

            return call::state();
         }

      } // call



      namespace format
      {


         struct base_instances
         {
            base_instances( const std::vector< admin::InstanceVO>& instances) : m_instances( instances) {}

         protected:

            std::vector< admin::InstanceVO> instances( const std::vector< platform::pid_type>& pids) const
            {
               std::vector< admin::InstanceVO> result;

               for( auto& pid : pids)
               {
                  auto found = range::find_if( m_instances, [=]( const admin::InstanceVO& v){
                     return v.process.pid == pid;
                  });
                  if( found)
                  {
                     result.push_back( *found);
                  }
               }

               return result;
            }

            const std::vector< admin::InstanceVO>& m_instances;
         };

         struct format_state :  base_instances
         {
            using base_instances::base_instances;

            template< typename T>
            std::size_t width( const T& value) const
            {
               return value.instances.size();
            }



            template< typename T>
            void print( std::ostream& out, const T& value, std::size_t width, bool color) const
            {
               if( color)
               {
                  for( auto& instance : instances( value.instances))
                  {
                     switch( static_cast< state::Server::Instance::State>( instance.state))
                     {
                        case state::Server::Instance::State::booted: out << terminal::color::magenta.start() << '^'; break;
                        case state::Server::Instance::State::idle: out << terminal::color::green.start() << '+'; break;
                        case state::Server::Instance::State::busy: out << terminal::color::yellow.start() << '*'; break;
                        case state::Server::Instance::State::shutdown: out << terminal::color::red.start() << 'x'; break;
                        default: out << terminal::color::red.start() <<  '-'; break;
                     }
                  }
                  out << terminal::color::green.end();
               }
               else
               {
                  for( auto& instance : instances( value.instances))
                  {
                     switch( static_cast< state::Server::Instance::State>( instance.state))
                     {
                        case state::Server::Instance::State::booted: out << '^'; break;
                        case state::Server::Instance::State::idle: out << '+'; break;
                        case state::Server::Instance::State::busy: out << '*'; break;
                        case state::Server::Instance::State::shutdown: out << 'x'; break;
                        default: out <<  '-'; break;
                     }
                  }
               }

               //
               // Pad manually
               //

               if( width != 0 )
               {
                  out << std::string( width - value.instances.size(), ' ');
               }
            }
         };


         struct format_instances
         {
            template< typename T>
            std::size_t operator () ( const T& value) const { return value.instances.size();}
         };

         terminal::format::formatter< admin::ServiceVO> services( const std::vector< admin::InstanceVO>& instances)
         {

            struct format_timeout
            {
               double operator () ( const admin::ServiceVO& value) const
               {
                  using second_t = std::chrono::duration< double>;
                  return std::chrono::duration_cast< second_t>( value.timeout).count();
               }
            };


            struct format_mode
            {
               const char* operator () ( const admin::ServiceVO& value) const
               {
                  static std::map< server::Service::Transaction, const char*> mapping{
                     {server::Service::Transaction::automatic, "auto"},
                     {server::Service::Transaction::join, "join"},
                     {server::Service::Transaction::atomic, "atomic"},
                     {server::Service::Transaction::none, "none"},
                  };
                  return mapping[ server::Service::Transaction( value.mode)];
               }
            };

            return {
               { global::porcelain, ! global::no_colors, ! global::no_header},
               terminal::format::column( "name", std::mem_fn( &admin::ServiceVO::name), terminal::color::yellow, terminal::format::Align::left),
               terminal::format::column( "type", std::mem_fn( &admin::ServiceVO::type), terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "mode", format_mode{}, terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "timeout", format_timeout{}, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "requested", std::mem_fn( &admin::ServiceVO::lookedup), terminal::color::cyan, terminal::format::Align::right),
               terminal::format::column( "#", format_instances{}, terminal::color::white, terminal::format::Align::right),
               terminal::format::custom_column( "state", format_state{ instances})
            };
         }

         terminal::format::formatter< admin::ServerVO> servers( const std::vector< admin::InstanceVO>& instances)
         {

            struct format_last : base_instances
            {
               using base_instances::base_instances;

               std::string operator () ( const admin::ServerVO& value) const
               {
                  auto inst = instances( value.instances);

                  decltype( inst.front().last) last;

                  for( auto& instance : inst)
                  {
                     last = std::max( last, instance.last);
                  }
                  return chronology::local( last);
               }
            };

            struct format_invoked : base_instances
            {
               using base_instances::base_instances;

               std::size_t operator () ( const admin::ServerVO& value) const
               {
                  auto inst = instances( value.instances);

                  decltype( inst.front().invoked) invoked = 0;

                  for( auto& instance : inst)
                  {
                     invoked += instance.invoked;
                  }
                  return invoked;
               }
            };


            return {
               { global::porcelain, ! global::no_colors, ! global::no_header},
               terminal::format::column( "alias", std::mem_fn( &admin::ServerVO::alias), terminal::color::yellow, terminal::format::Align::left),
               terminal::format::column( "invoked", format_invoked{ instances}, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "last", format_last{ instances}, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "#", format_instances{}, terminal::color::white, terminal::format::Align::right),
               terminal::format::custom_column( "state", format_state{ instances}),
               terminal::format::column( "path", std::mem_fn( &admin::ServerVO::path), terminal::color::no_color, terminal::format::Align::left)
            };
         }


         terminal::format::formatter< admin::InstanceVO> instances( const std::vector< admin::ServerVO>& servers)
         {
            struct base_server
            {
               base_server( const std::vector< admin::ServerVO>& servers) : m_servers( servers) {}

               const std::string& operator () ( const admin::InstanceVO& value) const
               {
                  return server( value.server).alias;
               }

            protected:

               const admin::ServerVO& server( std::size_t server_id) const
               {
                  auto found = range::find_if( m_servers, [=] (const admin::ServerVO& v){
                     return v.id == server_id;
                  });

                  if( ! found)
                     throw exception::invalid::Configuration{ "Inconsistency"};

                  return *found;
               }

               const std::vector< admin::ServerVO>& m_servers;
            };


            struct format_server_name : base_server
            {
               using base_server::base_server;

               const std::string& operator () ( const admin::InstanceVO& value) const
               {
                  return server( value.server).alias;
               }
            };

            struct format_pid
            {
               platform::pid_type operator () ( const admin::InstanceVO& v) const { return v.process.pid;}
            };

            struct format_queue
            {
               platform::queue_id_type operator () ( const admin::InstanceVO& v) const { return v.process.queue;}
            };


            struct format_state
            {
               std::size_t width( const admin::InstanceVO& value) const { return 6;}

               void print( std::ostream& out, const admin::InstanceVO& value, std::size_t width, bool color) const
               {
                  out << std::setfill( ' ');

                  if( color)
                  {
                     switch( state::Server::Instance::State( value.state))
                     {
                        case state::Server::Instance::State::booted: out << std::right << std::setw( width) << terminal::color::red << "booted"; break;
                        case state::Server::Instance::State::idle: out << std::right << std::setw( width) << terminal::color::green << "idle"; break;
                        case state::Server::Instance::State::busy: out << std::right << std::setw( width) << terminal::color::yellow << "busy"; break;
                        case state::Server::Instance::State::shutdown: out << std::right << std::setw( width) << terminal::color::red << "shutdown"; break;
                     }
                  }
                  else
                  {
                     switch( state::Server::Instance::State( value.state))
                     {
                        case state::Server::Instance::State::booted: out << std::right << std::setw( width) << "booted"; break;
                        case state::Server::Instance::State::idle: out << std::right << std::setw( width) << "idle"; break;
                        case state::Server::Instance::State::busy: out << std::right << std::setw( width) << "busy"; break;
                        case state::Server::Instance::State::shutdown: out  << std::right << std::setw( width) << "shutdown"; break;
                     }
                  }
               }
            };

            struct format_last
            {
               std::string operator () ( const admin::InstanceVO& v) const { return chronology::local( v.last);}
            };

            struct format_path : base_server
            {
               using base_server::base_server;

               const std::string& operator () ( const admin::InstanceVO& v) const
               {
                  return server( v.server).path;
               }
            };

            return {
               { global::porcelain, ! global::no_colors, ! global::no_header},
               terminal::format::column( "server", format_server_name{ servers}, terminal::color::yellow, terminal::format::Align::left),
               terminal::format::column( "pid", format_pid{}, terminal::color::white, terminal::format::Align::right),
               terminal::format::column( "queue", format_queue{}, terminal::color::no_color, terminal::format::Align::right),
               terminal::format::custom_column( "state", format_state{}),
               terminal::format::column( "invoked", std::mem_fn( &admin::InstanceVO::invoked), terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "last", format_last{}, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "path", format_path{ servers}, terminal::color::no_color, terminal::format::Align::left),
            };
         }

      } // format


      namespace print
      {

         void servers( std::ostream& out, admin::StateVO& state)
         {
            range::sort( state.servers, []( const admin::ServerVO& l, const admin::ServerVO& r){ return l.alias < r.alias;});

            auto formatter = format::servers( state.instances);

            formatter.print( std::cout, std::begin( state.servers), std::end( state.servers));
         }

         void services( std::ostream& out, admin::StateVO& state)
         {
            range::sort( state.services, []( const admin::ServiceVO& l, const admin::ServiceVO& r){ return l.name < r.name;});

            auto formatter = format::services( state.instances);

            formatter.print( std::cout, std::begin( state.services), std::end( state.services));
         }

         void instances( std::ostream& out, admin::StateVO& state)
         {
            range::sort( state.instances, []( const admin::InstanceVO& l, const admin::InstanceVO& r){ return l.server < r.server;});

            auto formatter = format::instances( state.servers);

            formatter.print( std::cout, std::begin( state.instances), std::end( state.instances));
         }

      } // print

      namespace action
      {


         void listServers()
         {

            auto state = call::state();

            print::servers( std::cout, state);
         }

         void listServices()
         {
            auto state = call::state();

            print::services( std::cout, state);
         }

         void listInstances()
         {
            auto state = call::state();

            print::instances( std::cout, state);
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
            auto state = call::state();

            auto result = call::shutdown();

            {
               for( auto& instance : state.instances)
               {
                  if( common::range::find( result.offline, instance.process.pid))
                  {
                     instance.state = static_cast< long>( state::Server::Instance::State::shutdown);
                  }
               }
            }

            print::servers( std::cout, state);
         }

         void boot()
         {
            auto state =  call::boot();

            print::servers( std::cout, state);
         }

      } // action

   } // broker
} // casual



int main( int argc, char** argv)
{

   casual::common::Arguments parser;
   parser.add(

         casual::common::argument::directive( {"--porcelain"}, "easy to parse format", casual::broker::global::porcelain),
         casual::common::argument::directive( {"--no-color"}, "no color will be used", casual::broker::global::no_colors),
         casual::common::argument::directive( {"--no-header"}, "no descriptive header for each column will be used", casual::broker::global::no_header),
         casual::common::argument::directive( {"-lsvr", "--list-servers"}, "list all servers", &casual::broker::action::listServers),
         casual::common::argument::directive( {"-lsvc", "--list-services"}, "list all services", &casual::broker::action::listServices),
         casual::common::argument::directive( {"-li", "--list-instances"}, "list all instances", &casual::broker::action::listInstances),
         casual::common::argument::directive( {"-ui", "--update-instances"}, "<alias> <#> update server instances", &casual::broker::action::updateInstances),
         casual::common::argument::directive( {"-s", "--shutdown"}, "shutdown the domain", &casual::broker::action::shutdown),
         casual::common::argument::directive( {"-b", "--boot"}, "boot domain", &casual::broker::action::boot)
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


