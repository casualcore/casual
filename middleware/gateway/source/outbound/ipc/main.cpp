//!
//! casual
//!



#include "gateway/message.h"
#include "gateway/environment.h"
#include "gateway/outbound/gateway.h"
#include "gateway/outbound/routing.h"

#include "common/arguments.h"
#include "common/communication/ipc.h"
#include "common/marshal/network.h"
#include "common/marshal/binary.h"
#include "common/message/type.h"


#include <fstream>

namespace casual
{

   using namespace common;

   namespace gateway
   {
      namespace outbound
      {
         namespace ipc
         {

            namespace local
            {
               namespace
               {

                  process::Handle lookup_gateway( communication::ipc::inbound::Device& ipc, platform::queue_id_type broker)
                  {
                     common::message::lookup::process::Request request;
                     request.directive = common::message::lookup::process::Request::Directive::wait;
                     request.identification = environment::identification();
                     request.process.pid = process::handle().pid;
                     request.process.queue = ipc.connector().id();

                     return communication::ipc::call( broker, request,
                           communication::ipc::policy::Blocking{},
                           nullptr,
                           ipc).process;
                  }

                  message::inbound::ipc::connect::Reply lookup_inbound( communication::ipc::inbound::Device& ipc, platform::queue_id_type gateway)
                  {
                     message::inbound::ipc::connect::Request request;
                     request.process.pid = process::handle().pid;
                     request.process.queue = ipc.connector().id();
                     // TODO: set our domain

                     return communication::ipc::call( gateway, request,
                           communication::ipc::policy::Blocking{},
                           nullptr,
                           ipc);
                  }


                  message::inbound::ipc::connect::Reply connect_domain( communication::ipc::inbound::Device& ipc, const std::string& path)
                  {
                     Trace trace{ "outbound::ipc::local::connect_domain", log::internal::gateway};

                     std::ifstream domain_file{ path + "/.casual-broker-queue"};

                     if( domain_file)
                     {
                        platform::queue_id_type domain_qid{ 0};
                        domain_file >> domain_qid;

                        auto gateway = lookup_gateway( ipc, domain_qid);

                        return lookup_inbound( ipc, gateway.queue);
                     }

                     return {};
                  }

               } // <unnamed>
            } // local

            struct Settings
            {
               std::string domain_path;

            };


            struct Policy
            {
               struct outbound_configuration
               {
                  platform::queue_id_type id;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     archive & id;
                  )
               };

               using outbound_device_type = communication::ipc::outbound::Device;
               using inbound_device_type = communication::ipc::inbound::Device;


               struct State
               {
                  std::string domain_path;
                  inbound_device_type inbound;

                  process::Handle domain;


                  friend std::ostream& operator << ( std::ostream& out, const State& state)
                  {
                     return out << "{ domain_path: " << state.domain_path << ", inbound: " << state.inbound << "}\n";
                  }
               };


               static void validate( const Settings& settings)
               {
                  if( settings.domain_path.empty())
                  {
                     throw exception::invalid::Argument{ "invalid domain path", CASUAL_NIP( settings.domain_path)};
                  }
               }


               static State worker_state( Settings&& settings)
               {
                  Trace trace{ "outbound::ipc::Policy::state", log::internal::gateway};

                  State result;
                  result.domain_path = std::move( settings.domain_path);

                  return result;
               }


               static inbound_device_type& connect( State& state)
               {
                  Trace trace{ "outbound::ipc::Policy::connect", log::internal::gateway};

                  log::internal::gateway << "state: " << state << std::endl;

                  process::pattern::Sleep sleep{ {
                     { std::chrono::milliseconds{ 50}, 20}, // 1s
                     { std::chrono::milliseconds{ 500}, 20}, // 10s
                     { std::chrono::seconds{ 1}, 3600}, // 1h
                     { std::chrono::seconds{ 5}, 0} // forever
                  }};

                  while( ! state.domain)
                  {
                     sleep();

                     state.domain = local::connect_domain( state.inbound, state.domain_path).process;
                  }

                  return state.inbound;
               }


               static outbound_configuration outbound_device( State& state)
               {
                  Trace trace{ "outbound::ipc::Policy::outbound_device( state)", log::internal::gateway};

                  outbound_configuration result;
                  result.id = state.domain.queue;

                  return result;
               }


               static outbound_device_type outbound_device( outbound_configuration&& configuration)
               {
                  Trace trace{ "outbound::ipc::Policy::outbound_device( configuration)", log::internal::gateway};

                  return outbound_device_type{ configuration.id};
               }


            };

            using Gateway = outbound::Gateway< Policy>;

         } // ipc
      } // outbound
   } // gateway


} // casual


int main( int argc, char **argv)
{
   try
   {
      casual::gateway::outbound::ipc::Settings settings;
      {
         casual::common::Arguments parser{{
            casual::common::argument::directive( { "-p", "--domain-path"}, "path to remote domain home", settings.domain_path)
         }};
         parser.parse( argc, argv);
      }

      casual::gateway::outbound::ipc::Gateway gateway{ std::move( settings)};
      gateway();

   }
   catch( ...)
   {
      return casual::common::error::handler();
   }
   return 0;
}
