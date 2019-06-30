//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/manager/listen.h"
#include "gateway/common.h"

namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace listen
         {

            Entry::Entry( common::communication::tcp::Address address)
               : Entry( std::move( address), {})
            {

            }

            Entry::Entry( common::communication::tcp::Address address, Limit limit) 
               : m_address( std::move( address)), 
                  m_socket{ common::communication::tcp::socket::listen( m_address)}, 
                  m_limit( limit) 
            {

            }

            Connection Entry::accept() const
            {
               return { common::communication::tcp::socket::accept( m_socket), m_limit};
            }

         } // listen
      } // manager
   } // gateway
} // casual