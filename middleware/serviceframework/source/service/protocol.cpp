//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "serviceframework/service/protocol.h"
#include "serviceframework/service/protocol/implementation.h"
#include "serviceframework/log.h"

#include "common/exception/system.h"

namespace casual
{
   namespace serviceframework
   {
      namespace service
      {

         Protocol::~Protocol() = default;

         Protocol::Protocol( Protocol&&) = default;
         Protocol& Protocol::operator = ( Protocol&&) = default;

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
               registration< service::protocol::implementation::Yaml>( );
               registration< service::protocol::implementation::Binary>();
               registration< service::protocol::implementation::Json>();
               registration< service::protocol::implementation::Xml>();
               registration< service::protocol::implementation::Ini>();
            }

            Protocol Factory::create( common::service::invoke::Parameter&& parameter)
            {
               auto found = common::algorithm::find( m_creators, parameter.payload.type);

               if( found)
               {
                  if( local::describe( parameter))
                  {
                     common::log::line( log::debug, "casual-service-describe protocol");

                     // service-describe protocol
                     return protocol::implementation::Describe( found->second( std::move( parameter)));
                  }

                  return found->second( std::move( parameter));
               }
               throw common::exception::system::invalid::Argument( "no suitable protocol was found for type: " + parameter.payload.type);
            }


            Protocol deduce( common::service::invoke::Parameter&& parameter)
            {
               return Factory::instance().create( std::move( parameter));
            }

         } // protocol
      } // service
   } // serviceframework
} // casual
