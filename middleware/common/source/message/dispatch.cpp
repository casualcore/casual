//!
//! dispatch.cpp
//!
//! Created on: Dec 21, 2014
//!     Author: Lazan
//!

#include "common/message/dispatch.h"


#include "common/log.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace dispatch
         {


            bool Handler::do_dispatch( communication::message::Complete& complete) const
            {
               auto findIter = m_handlers.find( complete.type);

               if( findIter != std::end( m_handlers))
               {
                  findIter->second->dispatch( complete);
                  return true;
               }
               else
               {
                  common::log::error << "message_type: " << complete.type << " not recognized - action: discard" << std::endl;
               }
               return false;
            }


            bool Handler::do_dispatch( std::vector<communication::message::Complete>& complete) const
            {
               if( complete.empty())
               {
                  return false;
               }

               return do_dispatch( complete.front());
            }


            std::size_t Handler::size() const
            {
               return m_handlers.size();
            }

            std::vector< Handler::message_type> Handler::types() const
            {
               std::vector< message_type> result;

               for( auto& entry : m_handlers)
               {
                  result.push_back( entry.first);
               }

               return result;
            }

         } // dispatch
      } // message
   } // common
} // casual
