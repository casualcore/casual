//!
//! lookup.cpp
//!
//! Created on: Jun 26, 2015
//!     Author: Lazan
//!

#include "common/call/lookup.h"

#include "common/queue.h"

namespace casual
{
   namespace common
   {
      namespace call
      {
         namespace service
         {
            Lookup::Lookup( const std::string& service, long flags)
            {
               message::service::name::lookup::Request request;
               request.requested = service;
               request.process = process::handle();
               request.flags = flags;

               queue::blocking::Send send;
               m_correlation = send( ipc::broker::id(), request);
            }

            Lookup::~Lookup()
            {
               if( m_correlation != uuid::empty())
               {
                  ipc::receive::queue().discard( m_correlation);
               }
            }

            message::service::name::lookup::Reply Lookup::operator () () const
            {
               message::service::name::lookup::Reply result;
               queue::blocking::Reader receive( ipc::receive::queue());
               receive( result, m_correlation);

               m_correlation = Uuid();

               return result;
            }

         } // service

      } // call
   } // common

} // casual
