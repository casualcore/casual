//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "http/outbound/state.h"
#include "http/outbound/request.h"

#include "common/communication/ipc.h"
#include "common/exception/handle.h"
#include "common/exception/system.h"

namespace casual
{
   using namespace common;

   namespace http
   {
      namespace outbound
      {
         namespace state
         {
            std::ostream& operator << ( std::ostream& out, const Node& value)
            {
               return out << "{ url: " << value.url 
                  << ", headers: " << *value.headers
                  << ", discard-transaction:" << value.discard_transaction
                  << '}';
            }

            namespace pending
            {
               void Request::State::header_t::request_t::add( const Header& header)
               {
                  Trace trace{ "http::outbound::state::pending::Request::header_t::request_t::add Header"};

                  for( auto& field : header)
                  {
                     add( field.http());
                  }
               }

               void Request::State::header_t::request_t::add( const std::string& value)
               {
                  log::line( verbose::log, "header: ", value);

                  m_header.reset( curl_slist_append( m_header.release(), value.c_str()));
               }

               Request::Request() : m_easy( curl::easy::create()) {}

               
               std::ostream& operator << ( std::ostream& out, const Request::State& value)
               {
                  return out << "{ destination: " << value.destination
                     << ", payload size: " << value.payload.memory.size() 
                     << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Request& value)
               {
                  return out << "{ state: " << value.state()
                     << '}';
               }
            } // pending

            Pending::Pending() : m_multi( curl::multi::create())
            {

            }

            Pending::~Pending() noexcept
            {
               try
               {
                  algorithm::for_each( m_pending, [&]( const pending::Request& request){
                     curl::multi::remove( m_multi, request.easy());
                  });
               }
               catch( ...)
               {
                  exception::handle();
               }
            }

            void Pending::add( pending::Request&& request)
            {
               Trace trace{ "http::outbound::state::Pending::add"};

               curl::multi::add( m_multi, request.easy());
               m_pending.push_back( std::move( request));
            }

            pending::Request Pending::extract( curl::type::native::easy easy)
            {
               Trace trace{ "http::outbound::state::Pending::extract"};

               auto found = algorithm::find( m_pending, easy);

               if( ! found)
               {
                  throw exception::system::invalid::Argument{ "failed to find pending"};
               }

               auto result = std::move( *found);
               m_pending.erase( std::begin( found));
               curl::multi::remove( m_multi, result.easy());

               return result;
            }

         } // state

         State::State()
         {
            inbound.m_wait.events = CURL_WAIT_POLLIN;
            inbound.m_wait.fd = communication::ipc::inbound::handle().socket().descriptor().value();
         }


      } // outbound
   } // http
} // casual