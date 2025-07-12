//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "serviceframework/service/protocol.h"
#include "serviceframework/service/protocol/implementation.h"
#include "serviceframework/log.h"

#include "common/log/line.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

namespace casual
{
   namespace serviceframework
   {
      namespace service
      {

         Protocol::~Protocol() = default;

         Protocol::Protocol( Protocol&&) = default;
         Protocol& Protocol::operator = ( Protocol&&) = default;

         std::ostream& operator << ( std::ostream& out, const Protocol& value)
         {
            return out << "{ type: " << value.type() << '}';
         }

         namespace protocol
         {
            namespace local
            {
               namespace
               {
                  bool describe( const header::Fields& headers)
                  {
                     return headers.contains( "casual-service-describe") &&
                        headers.at( "casual-service-describe").value() != "false";
                  }

               } // <unnamed>
            } // local

            Factory& Factory::instance()
            {
               static Factory singleton;
               return singleton;
            }

            Factory::Factory()
            {
               registration< service::protocol::implementation::Yaml>();
               registration< service::protocol::implementation::Binary>();
               registration< service::protocol::implementation::Json>();
               registration< service::protocol::implementation::Xml>();
               registration< service::protocol::implementation::Ini>();
            }

            Protocol Factory::create( protocol::payload_type&& payload, const header::Fields& headers)
            {
               Trace trace{ "service::protocol::Factory::create"};
               common::log::debug( "payload: ", payload);

               if( auto found = common::algorithm::find( m_creators, payload.type))
               {
                  auto protocol = found->second( std::move( payload));

                  // should we wrap it in 'adapters'?
                  if( common::log::category::parameter)
                     protocol = Protocol::emplace< protocol::implementation::parameter::Log>( std::move( protocol));
                  
                  if( local::describe( headers))
                     protocol = Protocol::emplace< protocol::implementation::Describe>( std::move( protocol));

                  common::log::debug( "protocol: ", protocol);

                  return protocol;
               }
               
               common::code::raise::error( common::code::casual::communication_protocol, "no suitable protocol was found for type: ", payload.type);
            }


            Protocol deduce( protocol::payload_type&& payload, const header::Fields& headers)
            {
               return Factory::instance().create( std::move( payload), headers);
            }

         } // protocol
      } // service
   } // serviceframework
} // casual
