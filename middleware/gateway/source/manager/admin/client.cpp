//!
//! casual
//!

#include "gateway/manager/admin/vo.h"

#include "common/arguments.h"
#include "common/message/queue.h"
#include "common/terminal.h"
#include "common/chronology.h"
#include "common/transcode.h"

#include "sf/xatmi_call.h"
#include "sf/log.h"

#include "xatmi.h"

#include <iostream>

namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace normalize
      {

         std::string timestamp( const platform::time_point& time)
         {
            if( time != platform::time_point::min())
            {
               return chronology::local( time);
            }
            return {};
         }




      } // normalize

      namespace call
      {

         manager::admin::vo::State state()
         {
            sf::xatmi::service::binary::Sync service( ".casual.gateway.state");

            auto reply = service();

            manager::admin::vo::State serviceReply;

            reply >> CASUAL_MAKE_NVP( serviceReply);

            return serviceReply;
         }

      } // call


      namespace global
      {
         bool porcelain = false;

         bool header = true;
         bool color = true;

         void no_color() { color = false;}
         void no_header() { header = false;}

      } // global


      namespace format
      {
         namespace connection
         {


         } // connection

         template< typename C>
         terminal::format::formatter< C> connections()
         {
            auto format_domain_name = []( const manager::admin::vo::base_connection& c) { return c.remote.name; };
            auto format_domain_id = []( const manager::admin::vo::base_connection& c) { return transcode::hex::encode( c.remote.id.get());};

            auto format_pid = []( const manager::admin::vo::base_connection& c){ return c.process.pid;};
            auto format_type = []( const manager::admin::vo::base_connection& c)
            {
               switch( c.type)
               {
                  case manager::admin::vo::base_connection::Type::tcp: return "tcp";
                  case manager::admin::vo::base_connection::Type::ipc: return "ipc";
                  default: return "unknown";
               }
            };

            auto format_runlevel = []( const manager::admin::vo::base_connection& c)
            {
               switch( c.runlevel)
               {
                  case manager::admin::vo::base_connection::Runlevel::booting: return "booting";
                  case manager::admin::vo::base_connection::Runlevel::online: return "online";
                  case manager::admin::vo::base_connection::Runlevel::shutdown: return "shutdown";
                  case manager::admin::vo::base_connection::Runlevel::error: return "error";
                  default: return "absent";
               }
            };

            auto format_address = []( const manager::admin::vo::base_connection& c)
            {
               return string::join( c.address, " ");
            };

            return {
               { global::porcelain, global::color, global::header},
               terminal::format::column( "name", format_domain_name, terminal::color::yellow),
               terminal::format::column( "id", format_domain_id, terminal::color::blue),
               terminal::format::column( "pid", format_pid, terminal::color::white),
               terminal::format::column( "type", format_type, terminal::color::cyan),
               terminal::format::column( "runlevel", format_runlevel, terminal::color::no_color),
               terminal::format::column( "address", format_address, terminal::color::blue),
            };

         }

      } // format


      void list_inbound()
      {

         auto state = call::state();

         auto formatter = format::connections< manager::admin::vo::inbound::Connection>();

         formatter.print( std::cout, state.connections.inbound);
      }

      void list_outbound()
      {
         auto state = call::state();

         auto formatter = format::connections< manager::admin::vo::outbound::Connection>();

         formatter.print( std::cout, state.connections.outbound);
      }

      void print_state()
      {
         auto state = call::state();

         std::cout << CASUAL_MAKE_NVP( state);
      }



   } // gateway


   common::Arguments parser()
   {
      common::Arguments parser{ {
            common::argument::directive( {"--no-header"}, "do not print headers", &gateway::global::no_header),
            common::argument::directive( {"--no-color"}, "do not use color", &gateway::global::no_color),
            common::argument::directive( {"--porcelain"}, "Easy to parse format", gateway::global::porcelain),
            common::argument::directive( {"-i", "--list-inbound"}, "list inbound connections", &gateway::list_inbound),
            common::argument::directive( {"-o", "--list-outbound"}, "list outbound connections", &gateway::list_outbound),
            common::argument::directive( { "--state"}, "print state", &gateway::print_state),
      }};

      return parser;
   }


} // casual

int main( int argc, char **argv)
{
   try
   {
      auto parser = casual::parser();

      parser.parse( argc, argv);

   }
   catch( const std::exception& exception)
   {
      std::cerr << "exception: " << exception.what() << std::endl;
      return 20;
   }


   return 0;
}



