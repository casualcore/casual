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
            namespace pending
            {
               void Request::State::Header::Request::add( const common::service::header::Fields& header)
               {
                  Trace trace{ "http::outbound::state::pending::Request::header_t::request_t::add Header"};

                  for( auto& field : header)
                  {
                     add( field.http());
                  }
               }

               void Request::State::Header::Request::add( const std::string& value)
               {
                  log::line( verbose::log, "header: ", value);

                  auto handle = curl_slist_append( m_header.get(), value.c_str());

                  if( ! handle)
                  {
                     log::line( log::category::error, "request - failed to add header: ", value, " - action: ignore");
                  }
                  else
                  {
                     m_header.release();
                     m_header.reset( handle);
                  }
               }

               Request::Request() : m_easy( curl::easy::create()) {}
            
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

         void State::Metric::add( const state::pending::Request& request, common::message::service::Code code)
         {
            auto now = platform::time::clock::type::now();

            m_message.metrics.push_back( [&]()
            {
               message::event::service::Metric metric;

               metric.execution = request.state().execution;
               metric.service = request.state().service;
               metric.parent = request.state().parent;
               metric.process = common::process::handle();
               metric.code = code.result;
               
               // not transactions over http...
               // metric.trid

               metric.start = request.state().start;
               metric.end = now;

               return metric;
            }());
         }

         State::Metric::operator bool () const noexcept
         {
            return m_message.metrics.size() >= platform::batch::http::outbound::concurrent::metrics;
         }

         void State::Metric::clear()
         {
            m_message.metrics.clear();
         }


      } // outbound
   } // http
} // casual
