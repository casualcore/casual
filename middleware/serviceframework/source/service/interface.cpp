//!
//! service.cpp
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
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
            return doCall();
         }
         reply::State Interface::finalize()
         {
            return doFinalize();
         }

         Interface::Input& Interface::input()
         {
            return doInput();
         }

         void Interface::handleException()
         {
            doHandleException();
         }

         Interface::Output& Interface::output()
         {
            return doOutput();
         }

         IO::IO( interface_type interface)
               : m_interface( std::move( interface))
         {
         }

         IO::~IO()
         {

         }

         bool IO::callImplementation()
         {
            return m_interface->call();
         }

         void IO::handleException()
         {
            m_interface->handleException();
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
            if( common::log::active( common::log::category::Type::parameter))
            {
               return std::unique_ptr< Interface>( new protocol::parameter::Log< T>( serviceInfo));
            }

            return std::unique_ptr< Interface>( new T( serviceInfo));
         }


         std::unique_ptr< Interface> Factory::create( TPSVCINFO* serviceInfo, const buffer::Type& type) const
         {
            const common::trace::internal::Scope trace( "sf::service::Factory::create");

            auto found = m_factories.find( type);

            if( found == m_factories.end())
            {
               throw sf::exception::Validation( "no suitable protocol was found for type: " + type.name + " subtype: " + type.subname);
            }

            return found->second( serviceInfo);
         }

         std::unique_ptr< Interface> Factory::create( TPSVCINFO* serviceInfo) const
         {
            const common::trace::internal::Scope trace( "sf::service::Factory::create");

            return create( serviceInfo, sf::buffer::type::get( serviceInfo->data));
         }

         Factory::Factory()
         {
            const common::trace::internal::Scope trace( "sf::service::Factory::Factory");

            registration< service::protocol::Yaml>();
            registration< service::protocol::Binary>();
            registration< service::protocol::Json>();
            registration< service::protocol::Xml>();
            registration< service::protocol::Describe>();
         }



      } // service
   } // sf
} // casual

