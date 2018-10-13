//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "http/outbound/state.h"

#include "common/platform.h"
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
               common::code::xatmi transform( curl::type::code::easy code) noexcept;
            } // code
 
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
               
            } // detail

            namespace blocking
            {
               template< typename ID, typename OD>
               void dispath( State& state, ID&& inbound, OD&& outbound)
               {
                  Trace trace{ "http::outbound::request::dispatch"};
                  
                  auto& multi = state.pending.requests.multi();
                  
                  curl::multi::perform( multi);

                  using inbound_non_blocking = common::communication::ipc::inbound::Connector::non_blocking_policy;

                  // we consume any 'cached' inbound messages
                  inbound( inbound_non_blocking{});
                  
                  // we block
                  curl::check( curl_multi_wait( 
                     multi.get(), 
                     state.inbound.first(), 
                     state.inbound.size(), 
                     curl::timeout,
                     nullptr));


                  // curl dispatch
                  detail::dispath( state, outbound);

                  // check if the inbound is ready
                  if( state.inbound.pending())
                  {
                     state.inbound.clear();

                     // dispatch 
                     inbound( inbound_non_blocking{});
                  }
               }
            } // blocking


            namespace non
            {
               namespace blocking
               {
                  template< typename ID, typename OD>
                  void dispath( State& state, ID&& inbound, OD&& outbound)
                  {
                     Trace trace{ "http::outbound::request::non::blocking::dispatch"};
                     
                     curl::multi::perform( state.pending.requests.multi());

                     using inbound_non_blocking = common::communication::ipc::inbound::Connector::non_blocking_policy;

                     // curl dispatch
                     detail::dispath( state, outbound);

                     // take care of inbound
                     inbound( inbound_non_blocking{});
                  }
               } // blocking
            } // non
            
         } // request
      } // outbound
   } // http
} // casual



