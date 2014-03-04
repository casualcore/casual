//!
//! proxy.cpp
//!
//! Created on: Mar 1, 2014
//!     Author: Lazan
//!

#include "sf/proxy.h"
#include "sf/archive.h"
#include "sf/archive_binary.h"
#include "sf/xatmi_call.h"


namespace casual
{
   namespace sf
   {
      namespace proxy
      {

         namespace local
         {
            namespace
            {
               struct base_implementation
               {
                  base_implementation( const std::string& service, long flags)
                     : service( service), flags( flags), writer( buffer), reader( buffer)
                  {
                     input.writers.push_back( &writer);
                     output.readers.push_back( &reader);
                  }

                  IO::Input input;
                  IO::Output output;

                  std::string service;
                  long flags = 0;


                  buffer::Binary buffer;

                  archive::binary::Writer writer;
                  archive::binary::Reader reader;

               };


            } // <unnamed>
         } // local

         struct Async::Implementation : public local::base_implementation
         {
            using local::base_implementation::base_implementation;

            sf::xatmi::service::call_descriptor_type callDescriptor = 0;

            void send()
            {
               callDescriptor = xatmi::service::send( service, buffer, flags);

            }

            void receive()
            {
               if( ! xatmi::service::receive( callDescriptor, buffer, flags))
               {
                  // TODO: somehow there's a noblock flag...
               }
            }
         };



         Async::Async( const std::string& service) : Async( service, 0) {}

         Async::Async( const std::string& service, long flags) : m_implementation( new Implementation( service, flags))
         {

         }

         Async::~Async() {}

         Async& Async::interface()
         {
            return *this;
         }

         void Async::send()
         {
            m_implementation->send();
         }

         void Async::receive()
         {
            m_implementation->receive();
         }

         const IO::Input& Async::input() const
         {
            return m_implementation->input;
         }

         const IO::Output& Async::output() const
         {
            return m_implementation->output;
         }




         struct Sync::Implementation : public local::base_implementation
         {
            using local::base_implementation::base_implementation;

            void call()
            {
               xatmi::service::call( service, buffer, buffer, flags);
            }

         };



         Sync::Sync( const std::string& service) : Sync( service, 0) {}

         Sync::Sync( const std::string& service, long flags) : m_implementation( new Implementation( service, flags))
         {

         }

         Sync::~Sync() {}

         Sync& Sync::interface()
         {
            return *this;
         }

         void Sync::call()
         {
            m_implementation->call();
         }

         const IO::Input& Sync::input() const
         {
            return m_implementation->input;
         }

         const IO::Output& Sync::output() const
         {
            return m_implementation->output;
         }

      } // proxy
   } // sf


} // casual

