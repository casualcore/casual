//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "http/outbound/state.h"

#include "casual/platform.h"
#include "common/buffer/type.h"
#include "common/service/header.h"
#include "common/pimpl.h"

#include "common/message/service.h"
#include "common/communication/ipc.h"

namespace casual
{
   namespace http
   {  
      namespace outbound
      { 
         namespace request
         {
            
            state::pending::Request prepare( const state::Node& node, common::message::service::call::callee::Request&& message);


            namespace code
            {
               common::message::service::Code transform( const state::pending::Request& request, curl::type::code::easy code) noexcept;
            } // code


            namespace receive
            {
               namespace transcode
               {
                  //! transocode the 'reply' to reply-payload.
                  //! @attention the request is consumed!
                  common::buffer::Payload payload( state::pending::Request&& request);
               } // transcode
            } // receive
 
            namespace detail
            {
               template< typename OD>
               void dispath( State& state, OD& outbound)
               {
                  Trace trace{ "http::outbound::request::detail::dispatch"};
                  
                  auto& multi = state.pending.requests.multi();

                  int messages = 0;

                  auto message = curl_multi_info_read( multi.get(), &messages);

                  while( message)
                  {
                     if( message->msg == CURLMSG_DONE)
                     {
                        auto request = state.pending.requests.extract( message->easy_handle);
                        auto result = message->data.result;

                        outbound( std::move( request), result);
                     }

                     message = curl_multi_info_read( multi.get(), &messages);
                  }
               }
               

               //! @returns true if there are stuff to block on
               template< typename OD>
               bool perform( State& state, OD& outbound)
               {
                  auto running = curl::multi::perform( state.pending.requests.multi());

                  // we always do a dispatch, since it doesn't seem reliable to base it
                  // on number of running _easy-handles_ from curl_multi_perform...
                  // Though, it could easily be my misunderstanding of the API that is the reason...
                  detail::dispath( state, outbound);

                  common::log::line( verbose::log, "running: ", running);

                  return running != 0;
               }

               inline bool wait( State& state)
               {
                  Trace trace{ "http::outbound::request::detail::wait"};
                  
                  {
                     long timeout{};
                     curl_multi_timeout( state.pending.requests.multi().get(), &timeout);
                     common::log::line( verbose::log, "timeout: ", timeout);
                  }

                  int count{};
                  
                  // we block
                  curl::check( curl_multi_wait(
                     state.pending.requests.multi().get(),
                     state.inbound.first(),
                     state.inbound.size(),
                     curl::timeout,
                     &count));

                  common::log::line( verbose::log, "count: ", count);

                  return count != 0;
               }

            } // detail

            namespace blocking
            {
               template< typename ID, typename OD>
               void dispath( State& state, ID&& inbound, OD&& outbound)
               {
                  Trace trace{ "http::outbound::request::blocking::dispatch"};
                  
                  using inbound_non_blocking = common::communication::ipc::inbound::Connector::non_blocking_policy;

                  // we consume any 'cached' inbound messages
                  inbound( inbound_non_blocking{});
                  

                  // perform, and if there are stuff to wait for, do a blocking wait.
                  if( detail::perform( state, outbound) && detail::wait( state))
                  {
                     // check if the inbound is ready
                     if( state.inbound.pending())
                     {
                        state.inbound.clear();

                        // dispatch
                        inbound( inbound_non_blocking{});
                     }

                     // It's a good chance that there are stuff ready for
                     // perform, so we always do it...
                     detail::perform( state, outbound);
                  }
               }
            } // blocking
            
         } // request
      } // outbound
   } // http
} // casual



