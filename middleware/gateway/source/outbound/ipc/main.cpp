//!
//! casual
//!



#include "gateway/message.h"
#include "gateway/common.h"
#include "gateway/environment.h"
#include "gateway/outbound/gateway.h"
#include "gateway/outbound/routing.h"

#include "common/arguments.h"


#include "common/message/domain.h"
#include "common/environment.h"
#include "common/communication/ipc.h"


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
            using size_type = common::platform::size::type;

            using outbound_device_type = common::communication::ipc::outbound::Device;
            using inbound_device_type = common::communication::ipc::inbound::Device;

            static_assert( ! common::marshal::is_network_normalizing< inbound_device_type>::value, "inbound device does not need to normalize network representation");
            static_assert( ! common::marshal::is_network_normalizing< outbound_device_type>::value, "outbound device does not need to normalize network representation");


            namespace local
            {
               namespace
               {

                  process::Handle lookup_gateway( inbound_device_type& ipc, common::platform::ipc::id broker)
                  {
                     Trace trace{ "outbound::ipc::local::lookup_gateway"};

                     common::message::domain::process::lookup::Request request;
                     request.directive = common::message::domain::process::lookup::Request::Directive::wait;
                     request.identification = process::instance::identity::gateway::manager();
                     request.process.pid = process::handle().pid;
                     request.process.queue = ipc.connector().id();

                     return communication::ipc::call( broker, request,
                           communication::ipc::policy::Blocking{},
                           nullptr,
                           ipc).process;
                  }

                  message::ipc::connect::Reply lookup_inbound( inbound_device_type& ipc, common::platform::ipc::id gateway)
                  {
                     Trace trace{ "outbound::ipc::local::lookup_inbound"};

                     message::ipc::connect::Request request;
                     request.process.pid = process::handle().pid;
                     request.process.queue = ipc.connector().id();

                     log  << "reguest: " << request << '\n';

                     auto reply = communication::ipc::call( gateway, request,
                           communication::ipc::policy::Blocking{},
                           nullptr,
                           ipc);

                     log << "reply: " << reply << '\n';

                     return reply;
                  }


                  message::ipc::connect::Reply connect_domain( inbound_device_type& ipc, const std::string& path)
                  {
                     Trace trace{ "outbound::ipc::local::connect_domain"};

                     auto result = common::domain::singleton::read( path);

                     if( result.process.queue)
                     {
                        auto gateway = lookup_gateway( ipc, result.process.queue);

                        return lookup_inbound( ipc, gateway.queue);
                     }

                     log << "failed to find domain file: " << path << std::endl;

                     return {};
                  }

               } // <unnamed>
            } // local

            struct Settings
            {
               std::string domain_path;
               std::string domain_file;
               size_type order = 0;
            };


            struct Policy
            {

               struct configuration_type
               {
                  platform::ipc::id id;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     archive & id;
                  )
               };



               struct internal_type
               {
                  internal_type( configuration_type configuration) : m_outbound{ configuration.id}
                  {
                  }

                  outbound_device_type outbound() { return { m_outbound};}

                  static std::vector< std::string> address( const outbound_device_type& device)
                  {
                     return { common::string::compose( device.connector().id())};
                  }

                  friend std::ostream& operator << ( std::ostream& out, const internal_type& value)
                  {
                     return out << "{ outbound: " << value.m_outbound
                            << '}';
                  }

               private:

                  platform::ipc::id m_outbound;
               };

               struct external_type
               {
                  external_type( Settings&& settings)
                  {
                     Trace trace{ "outbound::ipc::Policy::external_type ctor"};

                     if( ! settings.domain_file.empty())
                     {
                        m_domain_file = std::move( settings.domain_file);
                     }
                     else
                     {
                        m_domain_file = common::environment::domain::singleton::file();
                     }

                     {
                        Trace trace{ "outbound::ipc::Policy::external_type ctor connect"};


                        process::pattern::Sleep sleep{ {
                           { std::chrono::milliseconds{ 50}, 20}, // 1s
                           { std::chrono::milliseconds{ 500}, 20}, // 10s
                           { std::chrono::seconds{ 1}, 3600}, // 1h
                           { std::chrono::seconds{ 5}, 0} // forever
                        }};

                        while( ! m_process)
                        {
                           sleep();

                           auto result = local::connect_domain( m_inbound, m_domain_file);
                           m_process = result.process;
                        }
                     }
                  }

                  inbound_device_type& inbound() { return m_inbound;}

                  configuration_type configuration() const
                  {
                     return { m_process.queue};
                  }

                  friend std::ostream& operator << ( std::ostream& out, const external_type& value)
                  {
                     return out << "{ path: " << value.m_domain_file
                           << ", inbound: " << value.m_inbound
                           << ", process: " << value.m_process
                           << '}';
                  }

               private:
                  std::string m_domain_file;
                  inbound_device_type m_inbound;

                  process::Handle m_process;

               };
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
            casual::common::argument::directive( { "-a", "--address"}, "path to remote domain home", settings.domain_path),
            casual::common::argument::directive( { "-o", "--order"}, "order of the outbound connector", settings.order),
            casual::common::argument::directive( { "--domain-file"}, "only to make unittest simple", settings.domain_file)
         }};
         parser.parse( argc, argv);
      }

      casual::gateway::outbound::ipc::Gateway gateway{ std::move( settings)};
      gateway();

   }
   catch( ...)
   {
      return casual::common::exception::handle();
   }
   return 0;
}
