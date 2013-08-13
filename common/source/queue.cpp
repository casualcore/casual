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

            base_writer::base_writer( ipc::send::Queue& queue) : m_queue( queue) {}

            void base_writer::send( marshal::output::Binary& archive, message_type_type type)
            {
               ipc::message::Complete message( type, archive.release());

               m_queue( message);
            }



            base_reader::base_reader( ipc::receive::Queue& queue) : m_queue( queue) {}

            marshal::input::Binary base_reader::next()
            {

               auto message = m_queue( 0);

               if( message.empty())
               {
                  throw exception::NotReallySureWhatToNameThisException( "blocking receive returned: queue: " + std::to_string( m_queue.id()));
               }

               return marshal::input::Binary( std::move( message.front()));
            }

            marshal::input::Binary base_reader::read( message_type_type type)
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
            base_writer::base_writer( ipc::send::Queue& queue) : m_queue( queue) {}



            bool base_writer::send( marshal::output::Binary& archive, message_type_type type)
            {
               ipc::message::Complete message( type, archive.release());

               return m_queue( message, ipc::receive::Queue::cNoBlocking);
            }


            base_reader::base_reader( ipc::receive::Queue& queue) : m_queue( queue) {}


            std::vector< marshal::input::Binary> base_reader::next()
            {
               std::vector< marshal::input::Binary> result;

               auto message = m_queue( ipc::receive::Queue::cNoBlocking);

               if( ! message.empty())
               {
                  result.emplace_back( std::move( message.front()));
               }

               return result;
            }

            std::vector< marshal::input::Binary> base_reader::read( message_type_type type)
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


