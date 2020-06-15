//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "http/outbound/curl.h"

#include "common/service/header.h"
#include "common/strong/id.h"
#include "common/memory.h"
#include "common/process.h"
#include "common/buffer/type.h"
#include "common/pimpl.h"
#include "common/message/service.h"
#include "common/message/pending.h"

#include <string>
#include <memory>
#include <unordered_map>


#include <curl/curl.h>


namespace casual
{
   namespace http
   {
      namespace outbound
      {
         namespace state
         {
         
            struct Node
            {
               std::string url;
               std::shared_ptr< const common::service::header::Fields> headers;
               bool discard_transaction = false;
               
               CASUAL_LOG_SERIALIZE(
               { 
                  CASUAL_NAMED_VALUE( url);
                  CASUAL_NAMED_VALUE( headers);
                  CASUAL_NAMED_VALUE( discard_transaction);
               })
            };


            namespace pending
            {
               struct Request 
               {
                  Request();

                  struct State
                  {
                     common::buffer::Payload payload;
                     platform::size::type offset = 0;
                     common::process::Handle destination;
                     common::Uuid correlation;
                     common::Uuid execution;
                     platform::time::point::type start = platform::time::point::limit::zero();
                     std::string service;
                     std::string parent;
                     common::transaction::ID trid;

                     struct Header
                     {
                        //! Take care of adding headers the curl way for the request.
                        struct Request
                        {
                           void add( const common::service::header::Fields& header);
                           void add( const std::string& value);
                           
                           inline auto native() { return m_header.get();}
                           inline explicit operator bool () const { return static_cast< bool>( m_header);}
                        private:
                           curl::type::header_list m_header{ nullptr};
                        } request;

                        //! holds the reply headers, when the call is done
                        common::service::header::Fields reply;

                        CASUAL_LOG_SERIALIZE(
                        { 
                           CASUAL_NAMED_VALUE( reply);
                        })   

                     } header;

                     inline auto range() noexcept { return common::range::make( std::begin( payload.memory) + offset, std::end( payload.memory));}
                     inline void clear() noexcept
                     { 
                        payload.memory.clear();
                        offset = 0;
                     }
                     
                     CASUAL_LOG_SERIALIZE(
                     { 
                        CASUAL_NAMED_VALUE( payload);
                        CASUAL_NAMED_VALUE( offset);
                        CASUAL_NAMED_VALUE( destination);
                        CASUAL_NAMED_VALUE( correlation);
                        CASUAL_NAMED_VALUE( execution);
                        CASUAL_NAMED_VALUE( start);
                        CASUAL_NAMED_VALUE( service);
                        CASUAL_NAMED_VALUE( parent);
                        CASUAL_NAMED_VALUE( trid);
                        CASUAL_NAMED_VALUE( header);
                     })                     
                  };

                  inline const curl::type::easy& easy() const { return m_easy;}

                  //! @return state that is _stable_ in memory, hence it's address will never change 
                  State& state() { return *m_state;} 
                  const State& state() const { return *m_state;} 

                  
                  inline friend bool operator == ( const Request& lhs, curl::type::native::easy rhs) { return lhs.m_easy.get() == rhs;}

                  CASUAL_LOG_SERIALIZE(
                  { 
                     CASUAL_NAMED_VALUE_NAME( m_easy, "easy");
                     CASUAL_NAMED_VALUE_NAME( state(), "state");
                  })

               private:
                  curl::type::easy m_easy;
                  common::move::basic_pimpl< State> m_state;
               };
               static_assert( common::traits::is_nothrow_movable< Request>::value, "not movable");
               static_assert( ! common::traits::is_copyable< Request>::value, "not movable");



            } // pending

            struct Pending
            {
               Pending();
               ~Pending() noexcept;

               Pending( Pending&&) noexcept = default;
               Pending& operator = ( Pending&&) noexcept = default;

               void add( pending::Request&& request);
               pending::Request extract( curl::type::native::easy easy);

               inline const curl::type::multi& multi() const { return m_multi;}


               inline auto begin() const { return std::begin( m_pending);}
               inline auto end() const { return std::end( m_pending);}
               inline auto empty() const { return m_pending.empty();}
               explicit operator bool () const { return ! empty();}
               inline platform::size::type size() const { return m_pending.size();}
               inline auto capacity() const { return m_pending.capacity();}

            private:
               std::vector< state::pending::Request> m_pending;
               curl::type::multi m_multi;
            };
            
         } // state

         struct State 
         {
            State();

            struct
            {
               state::Pending requests;
            } pending;

            struct 
            {
               curl::type::wait_descriptor* first() { return &m_wait;}
               auto size() { return 1;}

               bool pending() const { return m_wait.revents & CURL_WAIT_POLLIN;}

               void clear() { m_wait.revents = {};}
            private:
               friend State;

               curl::type::wait_descriptor m_wait{};

            } inbound;

            std::unordered_map< std::string, state::Node> lookup;

            struct Metric 
            {
               void add( const state::pending::Request& request, common::message::service::Code code);

               explicit operator bool () const noexcept;

               void clear();

               inline auto& message() const { return m_message;}

            private:
               common::message::event::service::Calls m_message;
            } metric;

         };
      } // outbound
   } // http
} // casual
