//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "gateway/outbound/gateway.h"
#include "gateway/common.h"

#include "common/communication/tcp.h"

#include "common/domain.h"
#include "common/argument.h"

namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace outbound
      {
         namespace tcp
         {
            using size_type = common::platform::size::type;
            
            struct Settings
            {
               std::string address;
               size_type order = 0;
            };



            struct Policy
            {

               using outbound_device_type = communication::tcp::outbound::Device;
               using inbound_device_type = communication::tcp::inbound::Device;

               static_assert( common::marshal::is_network_normalizing< inbound_device_type>::value, "inbound device has to normalize network representation");
               static_assert( common::marshal::is_network_normalizing< outbound_device_type>::value, "outbound device has to normalize network representation");

               struct configuration_type
               {
                  communication::tcp::socket::descriptor_type descriptor;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     archive & descriptor;
                  )
               };



               struct internal_type
               {
                  internal_type( configuration_type configuration)
                     : m_outbound{ communication::tcp::duplicate( configuration.descriptor)}
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
                  external_type( Settings&& settings)
                   : m_inbound{ communication::tcp::retry::connect( settings.address, {
                           { std::chrono::milliseconds{ 100}, 100}, // 10s
                           { std::chrono::seconds{ 1}, 3600}, // 1h
                           { std::chrono::seconds{ 5}, 0} // forever
                        })}
                  {
                     Trace trace{ "outbound::tcp::Policy::external_type ctor"};


                     /*
                      * this should work...
                      *
                     inbound_device_type temp{ communication::tcp::retry::connect( m_adress, {
                           { std::chrono::milliseconds{ 50}, 20}, // 1s
                           { std::chrono::milliseconds{ 500}, 20}, // 10s
                           { std::chrono::seconds{ 1}, 3600}, // 1h
                           { std::chrono::seconds{ 5}, 0} // forever
                        })};

                     m_inbound = std::move( temp);
                     */
                  }

                  inbound_device_type& inbound() { return m_inbound;}


                  configuration_type configuration() const
                  {
                     return { m_inbound.connector().socket().descriptor()};
                  }

                  friend std::ostream& operator << ( std::ostream& out, const external_type& value)
                  {
                     return out << "{ inbound: " << value.m_inbound
                           << '}';
                  }

               private:
                  inbound_device_type m_inbound;
               };
            };

            using Gateway = outbound::Gateway< Policy>;
         } // tcp
      } // outbound

   } // gateway

} // casual


int main( int argc, char **argv)
{
   try
   {
      casual::gateway::outbound::tcp::Settings settings;
      {
         using namespace casual::common::argument;
         Parse parse{ "tcp outbound",
            Option( std::tie( settings.address), { "-a", "--address"}, "address to the remote domain [(ip|domain):]port"),
            Option( std::tie( settings.order), { "-o", "--order"}, "order of the outbound connector"),
         };
         parse( argc, argv);
      }

      casual::gateway::outbound::tcp::Gateway gateway{ std::move( settings)};
      gateway();

   }
   catch( ...)
   {
      return casual::common::exception::handle();
   }
   return 0;
}
