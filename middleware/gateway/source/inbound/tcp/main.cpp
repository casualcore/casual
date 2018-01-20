//!
//! casual 
//!

#include "gateway/inbound/gateway.h"
#include "gateway/common.h"

#include "common/communication/tcp.h"


#include "common/arguments.h"
#include "common/environment.h"


namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace inbound
      {
         namespace tcp
         {
            using size_type = common::platform::size::type;

            struct Settings
            {
               communication::socket::descriptor_type descriptor;

               struct
               {
                  size_type size = 0;
                  size_type messages = 0;
               } limit;

            };

            struct Policy
            {


               using outbound_device_type = communication::tcp::outbound::Device;
               using inbound_device_type = communication::tcp::inbound::Device;
               
               static_assert( common::marshal::is_network_normalizing< inbound_device_type>::value, "inbound device has to normalize network representation");
               static_assert( common::marshal::is_network_normalizing< outbound_device_type>::value, "outbound device has to normalize network representation");

               static inbound::Cache::Limit limits( const Settings& settings)
               {
                  return { settings.limit.size, settings.limit.messages};
               }

               struct configuration_type
               {
                  communication::socket::descriptor_type descriptor;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     archive & descriptor;
                  )
               };

               struct internal_type
               {
                  internal_type( configuration_type configuration)
                     : m_outbound{ communication::socket::duplicate( configuration.descriptor)}
                  {
                  }

                  outbound_device_type& outbound() { return m_outbound;}

                  static std::vector< std::string> address( const outbound_device_type& device)
                  {
                     auto address = communication::tcp::socket::address::peer( device.connector().socket().descriptor());
                     return { address.host + ':' + address.port};
                  }

                  friend std::ostream& operator << ( std::ostream& out, const internal_type& value)
                  {
                     return out << "{ outbound: " << value.m_outbound
                            << '}';
                  }

               private:

                  outbound_device_type m_outbound;
               };

               struct external_type
               {
                  external_type( tcp::Settings&& settings)
                     : m_inbound{ communication::Socket{ settings.descriptor}}
                  {
                     log << "external_type: " << *this << '\n';

                  }

                  friend std::ostream& operator << ( std::ostream& out, const external_type& external)
                  {
                     return out << "{ inbound: " << external.m_inbound
                           << "}";
                  }

                  inbound_device_type& device() { return m_inbound;}

                  configuration_type configuration() const
                  {
                     return { m_inbound.connector().socket().descriptor()};
                  }

               private:
                  inbound_device_type m_inbound;
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
      casual::gateway::inbound::tcp::Settings settings;
      {
         casual::common::Arguments parser{{
            casual::common::argument::directive( { "--descriptor"}, "socket descriptor", settings.descriptor.underlaying()),
            casual::common::argument::directive( { "--limit-messages"}, "# of concurrent messages", settings.limit.messages),
            casual::common::argument::directive( { "--limit-size"}, "max size of concurrent messages", settings.limit.size),
         }};
         parser.parse( argc, argv);
      }

      casual::gateway::inbound::tcp::Gateway gateway{ std::move( settings)};
      gateway();

   }
   catch( ...)
   {
      return casual::common::exception::handle();
   }
   return 0;
}
