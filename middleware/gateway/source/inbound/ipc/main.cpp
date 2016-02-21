//!
//! casual 
//!

#include "gateway/inbound/gateway.h"


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
               platform::queue_id_type ipc = 0;

            };



            struct Policy
            {

               using outbound_device_type = communication::ipc::outbound::Device;
               using inbound_device_type = communication::ipc::inbound::Device;

               struct outbound_configuration
               {
                  platform::queue_id_type id;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     archive & id;
                  )
               };

               struct State
               {
                  inbound_device_type inbound;
                  common::process::Handle remote;

                  friend std::ostream& operator << ( std::ostream& out, const State& state)
                  {
                     return out << "{ remote: " << state.remote << ", inbound: " << state.inbound << "}\n";
                  }


               };

               static void validate( const Settings& settings)
               {
                  if( settings.ipc == 0)
                  {
                     throw exception::invalid::Argument{ "invalid ipc queue", CASUAL_NIP( settings.ipc)};
                  }
               }

               static State worker_state( Settings&& settings)
               {
                  Trace trace{ "inbound::ipc::Policy::state", log::internal::gateway};

                  State result;

                  result.remote.queue = settings.ipc;

                  return result;
               }

               static inbound_device_type& connect( State& state)
               {
                  Trace trace{ "inbound::ipc::Policy::connect", log::internal::gateway};

                  //
                  // Send the reply
                  //
                  {
                     Trace trace{ "inbound::ipc::Policy::connect reply", log::internal::gateway};

                     message::inbound::ipc::connect::Reply reply;
                     reply.process.pid = common::process::id();
                     reply.process.queue = state.inbound.connector().id();
                     reply.domain.id = common::environment::domain::id();
                     reply.domain.name = common::environment::domain::name();

                     communication::ipc::blocking::send( state.remote.queue, reply);
                  }

                  //
                  // We ping the remote domain. We don't really have to do this...
                  //
                  {
                     Trace trace{ "inbound::ipc::Policy::connect ping", log::internal::gateway};

                     common::message::server::ping::Request request;
                     request.process.pid = common::process::id();
                     request.process.queue = state.inbound.connector().id();

                     state.remote = communication::ipc::call(
                           state.remote.queue,
                           request,
                           communication::ipc::policy::Blocking{},
                           nullptr,
                           state.inbound).process;
                  }
                  return state.inbound;
               }

               static outbound_configuration outbound_device( State& state)
               {
                  Trace trace{ "inbound::ipc::Policy::outbound_device( state)", log::internal::gateway};

                  return { state.remote.queue};
               }

               static outbound_device_type outbound_device( outbound_configuration configuration)
               {
                  Trace trace{ "inbound::ipc::Policy::outbound_device( configuration)", log::internal::gateway};

                  return { configuration.id};
               }
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
            casual::common::argument::directive( { "-ipc", "--remote-ipc-queue"}, "romote domain ipc queue", settings.ipc)
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


