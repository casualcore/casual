//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "serviceframework/service/protocol.h"
#include "serviceframework/service/protocol/implementation.h"
#include "serviceframework/log.h"

#include "common/log/stream.h"

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
                  template< typename P>
                  bool describe( const P& header)
                  {
                     return common::service::header::fields().exists( "casual-service-describe") &&
                           common::service::header::fields().at( "casual-service-describe") != "false";
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

            Protocol Factory::create( common::service::invoke::Parameter&& parameter)
            {
               Trace trace{ "service::protocol::Factory::create"};
               common::log::line( log::debug, "parameter: ", parameter);

               if( auto found = common::algorithm::find( m_creators, parameter.payload.type))
               {
                  auto protocol = found->second( std::move( parameter));

                  // should we wrap it in 'adapters'?
                  if( log::parameter)
                     protocol = Protocol::emplace< protocol::implementation::parameter::Log>( std::move( protocol));
                  
                  if( local::describe( parameter))
                     protocol = Protocol::emplace< protocol::implementation::Describe>( std::move( protocol));

                  common::log::line( log::debug, "protocol: ", protocol);

                  return protocol;
               }
               
               common::code::raise::error( common::code::casual::communication_protocol, "no suitable protocol was found for type: " + parameter.payload.type);
            }


            Protocol deduce( common::service::invoke::Parameter&& parameter)
            {
               return Factory::instance().create( std::move( parameter));
            }

         } // protocol
      } // service
   } // serviceframework
} // casual
