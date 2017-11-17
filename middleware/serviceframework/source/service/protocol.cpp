//!
//! casual 
//!

#include "sf/service/protocol.h"
#include "sf/service/protocol/implementation.h"

#include "sf/exception.h"
#include "sf/log.h"


namespace casual
{
   namespace sf
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
                     log::sf << "casual-service-describe protocol\n";

                     //
                     // service-describe protocol
                     //
                     return protocol::implementation::Describe( found->second( std::move( parameter)));
                  }


                  return found->second( std::move( parameter));
               }
               throw sf::exception::Validation( "no suitable protocol was found for type: " + parameter.payload.type);
            }


            Protocol deduce( common::service::invoke::Parameter&& parameter)
            {
               return Factory::instance().create( std::move( parameter));
            }

         } // protocol
      } // service
   } // sf
} // casual
