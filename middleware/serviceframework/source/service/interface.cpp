//!
//! casual
//!

#include "sf/service/interface.h"
#include "sf/exception.h"

// TODO: temporary to test factory
#include "sf/service/protocol.h"

#include "common/internal/trace.h"


//
// std
//
#include <stdexcept>

namespace casual
{
   namespace sf
   {
      namespace service
      {
         Interface::~Interface()
         {

         }

         bool Interface::call()
         {
            return do_call();
         }
         reply::State Interface::finalize()
         {
            return do_finalize();
         }

         Interface::Input& Interface::input()
         {
            return do_input();
         }

         void Interface::handle_exception()
         {
            do_andle_exception();
         }

         Interface::Output& Interface::output()
         {
            return do_output();
         }

         IO::IO( interface_type interface)
               : m_interface( std::move( interface))
         {
         }

         IO::~IO()
         {

         }

         bool IO::call_implementation()
         {
            return m_interface->call();
         }

         void IO::handle_exception()
         {
            m_interface->handle_exception();
         }

         reply::State IO::finalize()
         {
            return m_interface->finalize();
         }


         Factory& Factory::instance()
         {
            static Factory singleton;
            return singleton;
         }


         template< typename T>
         std::unique_ptr< Interface> Factory::Creator< T>::operator()( TPSVCINFO* serviceInfo) const
         {
            if( common::log::parameter)
            {
               return std::unique_ptr< Interface>( new protocol::parameter::Log< T>( serviceInfo));
            }

            return std::unique_ptr< Interface>( new T( serviceInfo));
         }


         std::unique_ptr< Interface> Factory::create( TPSVCINFO* service_info, const buffer::Type& type) const
         {
            sf::Trace trace( "sf::service::Factory::create");

            auto found = common::range::find_if( m_factories, [&]( const Holder& h){
               return h.type.equal( h.type.type, type);
            });

            if( found)
            {
               return found->create( service_info);
            }

            throw sf::exception::Validation( "no suitable protocol was found for type: " + type.name + " subtype: " + type.subname);


         }

         std::unique_ptr< Interface> Factory::create( TPSVCINFO* serviceInfo) const
         {
            sf::Trace trace( "sf::service::Factory::create");

            return create( serviceInfo, sf::buffer::type::get( serviceInfo->data));
         }

         Factory::Factory()
         {
            sf::Trace trace( "sf::service::Factory::Factory");

            registration< service::protocol::Yaml>();
            registration< service::protocol::Binary>();
            registration< service::protocol::Json>();
            registration< service::protocol::Xml>();
            registration< service::protocol::Describe>();
         }



      } // service
   } // sf
} // casual

