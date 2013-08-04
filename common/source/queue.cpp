//!
//! casual_queue.cpp
//!
//! Created on: Jun 10, 2012
//!     Author: Lazan
//!

#include "common/queue.h"
#include "common/ipc.h"


// temp
#include <iostream>


#include <algorithm>

namespace casual
{
   namespace common
   {
      namespace queue
      {

         namespace blocking
         {

            Writer::Writer( ipc::send::Queue& queue) : m_queue( queue) {}

            void Writer::send( marshal::output::Binary& archive, message_type_type type)
            {
               ipc::message::Complete message( type, archive.release());

               m_queue( message);
            }



            Reader::Reader( ipc::receive::Queue& queue) : m_queue( queue) {}

            marshal::input::Binary Reader::next()
            {

               auto message = m_queue( 0);

               if( message.empty())
               {
                  throw exception::NotReallySureWhatToNameThisException( "blocking receive returned: queue: " + std::to_string( m_queue.id()));
               }

               return marshal::input::Binary( std::move( message.front()));
            }

            marshal::input::Binary Reader::read( message_type_type type)
            {
               auto message = m_queue( type, 0);

               if( message.empty())
               {
                  throw exception::NotReallySureWhatToNameThisException( "blocking receive returned: queue: " + std::to_string( m_queue.id()));
               }

               return marshal::input::Binary( std::move( message.front()));
            }

         } // blocking


         namespace non_blocking
         {
            Writer::Writer( ipc::send::Queue& queue) : m_queue( queue) {}



            bool Writer::send( marshal::output::Binary& archive, message_type_type type)
            {
               ipc::message::Complete message( type, archive.release());

               return m_queue( message, ipc::receive::Queue::cNoBlocking);
            }


            Reader::Reader( ipc::receive::Queue& queue) : m_queue( queue) {}


            std::vector< marshal::input::Binary> Reader::next()
            {
               std::vector< marshal::input::Binary> result;

               auto message = m_queue( ipc::receive::Queue::cNoBlocking);

               if( ! message.empty())
               {
                  result.emplace_back( std::move( message.front()));
               }

               return result;
            }

            std::vector< marshal::input::Binary> Reader::read( message_type_type type)
            {
               std::vector< marshal::input::Binary> result;

               auto message = m_queue( type, ipc::receive::Queue::cNoBlocking);

               if( ! message.empty())
               {
                  result.emplace_back( std::move( message.front()));
               }

               return result;
            }

         } // non_blocking
      } // queue
   } // common
} // casual


