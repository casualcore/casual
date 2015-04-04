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

      namespace normalized
      {
         namespace color
         {
            using Solid = terminal::color::Solid;
         } // color


         struct Instance
         {
            std::string server;
            std::string path;
            sf::platform::pid_type pid = 0;
            sf::platform::queue_id_type queue = 0;
            std::string state = "off-line";
            long invoked = 0;
            std::string last = "---";

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( server);
               archive & CASUAL_MAKE_NVP( pid);
               archive & CASUAL_MAKE_NVP( queue);
               archive & CASUAL_MAKE_NVP( state);
               archive & CASUAL_MAKE_NVP( invoked);
               archive & CASUAL_MAKE_NVP( last);
               archive & CASUAL_MAKE_NVP( path);
            })

            friend bool operator < ( const Instance& lhs, const Instance& rhs)
            {
               if( lhs.server < rhs.server) return true;
               if( lhs.server > rhs.server) return false;
               return lhs.pid < rhs.pid;
            }

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
                  { "server", color::Solid{ common::terminal::color::yellow}},
                  { "pid", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::white}},
                  { "queue", sf::archive::terminal::Directive::Align::right},
                  { "state", sf::archive::terminal::Directive::Align::left, state_color},
                  { "invoked", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::blue}},
                  { "last", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::blue}},
               };
            }
         };


         struct Server
         {
            struct State
            {
               long invoked = 0;
               std::string last;
               long instances = 0;
               std::string state;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( invoked);
                  archive & CASUAL_MAKE_NVP( last);
                  archive & sf::makeNameValuePair( "#", instances);
                  archive & CASUAL_MAKE_NVP( state);
               })

               struct state_color_directive
               {
                  void operator()( std::ostream& out, const std::string& value)
                  {
                     auto padding = out.width( 0) - value.size();

                     for( auto& c : value)
                     {
                        switch( c)
                        {
                           case '*': out << common::terminal::color::yellow << c; break;
                           case '+': out << common::terminal::color::green << c; break;
                           default: out << common::terminal::color::red << c; break;
                        }
                     }
                     out << std::string( padding, ' ');
                  }
               };

            };

            std::string alias;
            State state;
            std::string path;


            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( alias);
               archive & CASUAL_MAKE_NVP( state);
               archive & CASUAL_MAKE_NVP( path);
            })

            friend bool operator < ( const Server& lhs, const Server& rhs) { return lhs.alias < rhs.alias;}

            static std::vector< sf::archive::terminal::Directive> directive()
            {
               return {
                  { "alias", color::Solid{ common::terminal::color::yellow}},
                  { "invoked", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::blue}},
                  { "last", sf::archive::terminal::Directive::Align::left, color::Solid{ common::terminal::color::blue}},
                  { "#", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::white}},
                  { "state", sf::archive::terminal::Directive::Align::left, State::state_color_directive{}},
               };
            }
         };

         struct Service
         {
            std::string name;
            std::string timeout;
            std::size_t pending = 0;
            std::size_t requested = 0;
            std::size_t type = 0;
            std::string mode;
            Server::State state;


            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive << CASUAL_MAKE_NVP( name);
               archive << CASUAL_MAKE_NVP( type);
               archive << CASUAL_MAKE_NVP( mode);
               archive << CASUAL_MAKE_NVP( timeout);
               archive << CASUAL_MAKE_NVP( requested);
               archive << CASUAL_MAKE_NVP( pending);
               archive & sf::makeNameValuePair( "#", state.instances);
               archive & sf::makeNameValuePair( "state", state.state);
            })

            friend bool operator < ( const Service& lhs, const Service& rhs) { return lhs.name < rhs.name;}

            static std::vector< sf::archive::terminal::Directive> directive()
            {
               return {
                  { "name", color::Solid{ common::terminal::color::yellow}},
                  { "timeout", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::blue}},
                  { "type", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::blue}},
                  { "mode", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::blue}},
                  { "pending", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::red}},
                  { "requested", sf::archive::terminal::Directive::Align::right},
                  { "#", sf::archive::terminal::Directive::Align::right, color::Solid{ common::terminal::color::white}},
                  { "state", sf::archive::terminal::Directive::Align::left, Server::State::state_color_directive{}},
               };
            }
         };

      } // normalized

      namespace normalize
      {
         std::vector< normalized::Instance> instances( const admin::StateVO& state)
         {
            std::vector< normalized::Instance> result;

            for( auto& instance : state.instances)
            {
               normalized::Instance value;

               auto server = range::find_if( state.servers, std::bind( equal_to{},
                     std::bind( &admin::ServerVO::id, std::placeholders::_1), instance.server));

               if( server)
               {
                  value.server = server->alias;
                  value.path = server->path;
               }
               value.pid = instance.process.pid;
               value.queue = instance.process.queue;


               value.invoked = instance.invoked;

               if( instance.last != sf::platform::time_type::min())
               {
                  value.last = common::chronology::local( instance.last);
               }

               auto state = []( long state){
                  switch( static_cast< state::Server::Instance::State>( state))
                  {
                     case state::Server::Instance::State::booted: return "booted"; break;
                     case state::Server::Instance::State::idle: return "idle"; break;
                     case state::Server::Instance::State::busy: return "busy"; break;
                     case state::Server::Instance::State::shutdown: return "shutdown"; break;
                     default: return "off-line";
                  }
               };

               value.state = state( instance.state);

               result.push_back( std::move( value));
            }

            range::sort( result);

            return result;
         }

         template< typename R>
         normalized::Server::State state( R&& instances)
         {
            normalized::Server::State result;

            result.instances = instances.size();

            auto latest = sf::platform::time_type::min();

            for( auto& instance : instances)
            {

               result.invoked += instance.invoked;

               latest = std::max( latest, instance.last);

               auto state = []( long state){
                  switch( static_cast< state::Server::Instance::State>( state))
                  {
                     case state::Server::Instance::State::booted: return '^'; break;
                     case state::Server::Instance::State::idle: return '+'; break;
                     case state::Server::Instance::State::busy: return '*'; break;
                     case state::Server::Instance::State::shutdown: return 'x'; break;
                     default: return '-';
                  }
               };

               result.state.push_back( state( instance.state));
            }

            if( latest != sf::platform::time_type::min())
            {
               result.last = common::chronology::local( latest);
            }

            return result;
         }


         std::vector< admin::InstanceVO> filterInstances(
               const std::vector< admin::InstanceVO>& instances,
               const std::vector< platform::pid_type>& pids)
         {
            std::vector< admin::InstanceVO> result;

            result.reserve( pids.size());

            //
            // We have to keep the order of the servers instances. We don't have an algorithm
            // that keep order in that way...
            //
            for( auto pid : pids)
            {
               auto found = range::find_if( instances, [=]( const admin::InstanceVO& inst)
               {
                  return inst.process.pid == pid;
               });
               result.push_back( *found);
            }

            return result;
         }

         std::vector< normalized::Server> servers( admin::StateVO state)
         {
            std::vector< normalized::Server> result;

            for( auto& server : state.servers)
            {
               normalized::Server value;

               value.alias = server.alias;
               value.path = server.path;
               value.state = normalize::state( filterInstances( state.instances, server.instances));

               result.push_back( std::move( value));
            }

            return range::sort( result);
         }

         std::map< std::string, std::vector< process::Handle>> pending( const std::vector< admin::PendingVO>& pending)
         {
            std::map< std::string, std::vector< process::Handle>> result;

            for( auto& pend : pending)
            {
               process::Handle process;
               process.pid = pend.process.pid;
               process.queue = pend.process.queue;
               result[ pend.requested].emplace_back( pend.process.pid, pend.process.queue);
            }

            return result;
         }


         std::vector< normalized::Service> services( admin::StateVO state)
         {
            std::vector< normalized::Service> result;

            auto pending = normalize::pending( state.pending);

            for( auto& service : state.services)
            {
               normalized::Service value;

               value.name = service.name;
               value.requested = service.lookedup;
               value.type = service.type;
               value.pending = pending[ value.name].size();

               switch( server::service::transaction::mode( service.mode))
               {
                  case server::Service::Transaction::automatic:
                     value.mode = "auto";
                     break;
                  case server::Service::Transaction::join:
                     value.mode = "join";
                     break;
                  case server::Service::Transaction::atomic:
                     value.mode = "atomic";
                     break;
                  case server::Service::Transaction::none:
                     value.mode = "none";
                     break;
               }


               {
                  using second_t = std::chrono::duration< double>;
                  std::stringstream out;
                  out << std::chrono::duration_cast< second_t>( service.timeout).count();
                  value.timeout = out.str();
               }

               value.state = normalize::state( filterInstances( state.instances, service.instances));

               //value.instances = service.instances.size();
               //value.pids = common::range::to_string( service.instances);

               result.push_back( std::move( value));
            }

            range::sort( result);

            return result;
         }
      } // normalize


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






      void listServers()
      {
         auto servers = normalize::servers( call::state());

         if( global::porcelain)
         {
            sf::archive::terminal::percelain::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( servers);

         }
         else
         {
            sf::archive::terminal::Writer writer{ std::cout, normalized::Server::directive(), ! global::no_header, ! global::no_colors};
            writer << CASUAL_MAKE_NVP( servers);
         }
      }

      void listServices()
      {
         auto services = normalize::services( call::state());

         if( global::porcelain)
         {
            sf::archive::terminal::percelain::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( services);

         }
         else
         {
            sf::archive::terminal::Writer writer{ std::cout, normalized::Service::directive(), ! global::no_header, ! global::no_colors};
            writer << CASUAL_MAKE_NVP( services);
         }
      }

      void listInstances()
      {
         auto instances = normalize::instances( call::state());

         if( global::porcelain)
         {
            sf::archive::terminal::percelain::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( instances);

         }
         else
         {
            sf::archive::terminal::Writer writer{ std::cout, normalized::Instance::directive(), ! global::no_header, ! global::no_colors};
            writer << CASUAL_MAKE_NVP( instances);
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

         auto output = normalize::servers( state);
         range::sort( output);

         if( global::porcelain)
         {
            sf::archive::terminal::percelain::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( output);
         }
         else
         {
            sf::archive::terminal::Writer writer{ std::cout, normalized::Server::directive(), ! global::no_header, ! global::no_colors};

            writer << CASUAL_MAKE_NVP( output);
         }
      }

      void boot()
      {
         auto servers = normalize::servers( call::boot());

         if( global::porcelain)
         {
            sf::archive::terminal::percelain::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( servers);

         }
         else
         {
            sf::archive::terminal::Writer writer{ std::cout, normalized::Server::directive(), ! global::no_header, ! global::no_colors};
            writer << CASUAL_MAKE_NVP( servers);
         }
      }

   } // broker
} // casual



int main( int argc, char** argv)
{

   casual::common::Arguments parser;
   parser.add(

         casual::common::argument::directive( {"--porcelain"}, "easy to parse format", casual::broker::global::porcelain),
         casual::common::argument::directive( {"--no-color"}, "no color will be used", casual::broker::global::no_colors),
         casual::common::argument::directive( {"--no-header"}, "no descriptive header for each column will be used", casual::broker::global::no_header),
         casual::common::argument::directive( {"-lsvr", "--list-servers"}, "list all servers", &casual::broker::listServers),
         casual::common::argument::directive( {"-lsvc", "--list-services"}, "list all services", &casual::broker::listServices),
         casual::common::argument::directive( {"-li", "--list-instances"}, "list all instances", &casual::broker::listInstances),
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


