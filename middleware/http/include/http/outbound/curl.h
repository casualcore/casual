//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "http/common.h"

#include "common/memory.h"



#include <array>

#include <curl/curl.h>

namespace casual
{
   namespace http
   {
      namespace outbound
      {
         namespace curl
         {
            constexpr common::platform::size::type timeout = 1000 * 60 * 5;

            namespace type
            {
               namespace native
               {
                  using easy = decltype( curl_easy_init());
               } // native

               using multi = decltype( common::memory::guard( curl_multi_init(), &curl_multi_cleanup));
               using easy = decltype( common::memory::guard( curl_easy_init(), &curl_easy_cleanup));
               using header_list = std::unique_ptr< curl_slist, std::function< void(curl_slist*)>>;

               using socket = curl_socket_t;
               using wait_descriptor = curl_waitfd;

               namespace code
               {
                  using multi = CURLMcode;
                  using easy = CURLcode;
               } // code

               namespace error
               {
                  using buffer = std::array< char, CURL_ERROR_SIZE>;
               } // error

            } // type

            namespace error
            {
               type::error::buffer& buffer();
            } // error

            void check( type::code::multi code);
            void check( type::code::easy code);

            void log( type::code::multi code);

            namespace multi
            {
               void add( const type::multi& multi, const type::easy& easy);
               void remove( const type::multi& multi, const type::easy& easy);

               common::platform::size::type perform( const type::multi& multi);

               type::multi create();
            } // multi

            namespace easy
            {
               type::easy create();
               
               namespace set
               {
                  template< typename Directive, typename Data>
                  void option( const type::easy& easy, Directive directive, Data&& data)
                  {
                     common::log::line( http::verbose::log, "directive: ", directive, " - data: ", data);
                     
                     curl::check( curl_easy_setopt( easy.get(), directive, std::forward< Data>( data)));
                  }
               } // set

            } // easy

         } // curl
      } // outbound
   } // http
} // casual
