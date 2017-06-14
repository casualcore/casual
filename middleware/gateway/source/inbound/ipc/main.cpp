//!
//! casual 
//!

#include "gateway/inbound/gateway.h"
#include "gateway/common.h"


#include "common/arguments.h"
#include "common/environment.h"


namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace inbound
      {
         namespace ipc
         {
            struct Settings
            {
               platform::ipc::id::type ipc = 0;
               std::string correlation;
            };

            struct Policy
            {

               using outbound_device_type = communication::ipc::outbound::Device;
               using inbound_device_type = communication::ipc::inbound::Device;


               struct configuration_type
               {
                  platform::ipc::id::type id;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     archive & id;
                  )
               };

               static inbound::Cache::Limit limits( const casual::gateway::inbound::ipc::Settings& settings) { return {};}

               struct internal_type
               {
                  internal_type( configuration_type configuration) : m_outbound{ configuration.id}
                  {
                  }

                  outbound_device_type outbound() { return { m_outbound};}

                  static std::vector< std::string> address( const outbound_device_type& device)
                  {
                     return { std::to_string( device.connector().id())};
                  }

                  friend std::ostream& operator << ( std::ostream& out, const internal_type& value)
                  {
                     return out << "{ outbound: " << value.m_outbound
                            << '}';
                  }

               private:

                  platform::ipc::id::type m_outbound;
               };

               struct external_type
               {
                  external_type( ipc::Settings&& settings)
                  {
                     Trace trace{ "inbound::ipc::Policy::external_type ctor"};

                     m_process.queue = settings.ipc;

                     //
                     // Send the reply
                     //
                     {
                        message::ipc::connect::Reply reply;
                        reply.correlation = Uuid{ settings.correlation};
                        reply.process.pid = common::process::id();
                        reply.process.queue = m_inbound.connector().id();

                        log << "reply: " << reply << '\n';

                        communication::ipc::blocking::send( m_process.queue, reply);
                     }
                  }

                  friend std::ostream& operator << ( std::ostream& out, const external_type& external)
                  {
                     return out << "{ process: " << external.m_process
                           << ", inbound: " << external.m_inbound << "}";
                  }

                  inbound_device_type& device() { return m_inbound;}

                  configuration_type configuration() const
                  {
                     return { m_process.queue};
                  }

               private:
                  inbound_device_type m_inbound;
                  common::process::Handle m_process;
               };

            };



            using Gateway = inbound::Gateway< Policy>;

         } // ipc
      } // inbound
   } // gateway

} // casual


int main( int argc, char **argv)
{
   try
   {
      casual::gateway::inbound::ipc::Settings settings;
      {
         casual::common::Arguments parser{{
            casual::common::argument::directive( { "--remote-ipc-queue"}, "romote domain ipc queue", settings.ipc),
            casual::common::argument::directive( { "--correlation"}, "message connect correlation", settings.correlation),
         }};
         parser.parse( argc, argv);
      }

      casual::gateway::inbound::ipc::Gateway gateway{ std::move( settings)};
      gateway();

   }
   catch( ...)
   {
      return casual::common::error::handler();
   }
   return 0;
}


