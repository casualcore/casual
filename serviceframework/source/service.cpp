//!
//! service.cpp
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
//!

#include "sf/service.h"
#include "sf/exception.h"

// TODO: temporary to test factory
#include "sf/service_protocol.h"

#include "utility/trace.h"


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
            if( std::uncaught_exception())
            {
               try
               {
                  m_interface->finalize();
               }
               catch( ...)
               {

               }
            }
            else
            {
               //
               // We might throw...
               //
               m_interface->finalize();
            }
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

         std::unique_ptr< Interface> Factory::create( TPSVCINFO* serviceInfo) const
         {
            utility::Trace trace( "sf::service::Factory::create");

            sf::buffer::Type type = sf::buffer::type( serviceInfo->data);

            auto found = m_factories.find( type);


            if( found == m_factories.end())
            {
               // TODO: some sf-exception
               throw sf::exception::Validation( "no suitable protocol was found for type: " + type.name + " subtype: " + type.subname);
            }

            return found->second( serviceInfo);

         }

         Factory::Factory()
         {
            utility::Trace trace( "sf::service::Factory::Factory");

            registrate< service::protocol::Yaml>( buffer::Type( "X_OCTET", "YAML"));
         }



      } // service
   } // sf
} // casual

