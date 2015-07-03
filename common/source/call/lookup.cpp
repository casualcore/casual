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
               message::service::lookup::Request request;
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

            message::service::lookup::Reply Lookup::operator () () const
            {
               if( m_correlation == uuid::empty())
               {
                  throw exception::Casual{ "call::service::Lookup::operator () called in a improper context"};
               }

               message::service::lookup::Reply result;
               queue::blocking::Reader receive( ipc::receive::queue());
               receive( result, m_correlation);

               if( result.state != message::service::lookup::Reply::State::busy)
               {
                  //
                  // We're not expecting another message from broker
                  //
                  m_correlation = Uuid{};
               }
               return result;
            }

         } // service

      } // call
   } // common

} // casual
