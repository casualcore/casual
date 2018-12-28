//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/uuid.h"
#include "common/message/service.h"
#include "common/optional.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace detail
         {
            struct Lookup
            {
               using Context = message::service::lookup::Request::Context;
               using Reply = message::service::lookup::Reply;
               using State = Reply::State;

               Lookup() noexcept;
               
               
               //! Lookup an entry point for the @p service
               Lookup( std::string service);


               //!
               //! Lookup an entry point for the @p service
               //! using a specific context
               //!  * regular
               //!  * no_reply
               //!  * forward
               //!  * gateway
               //!
               Lookup( std::string service, Context context);

               ~Lookup();

               Lookup( Lookup&&) noexcept;
               Lookup& operator = ( Lookup&&) noexcept;


               friend void swap( Lookup& lhs, Lookup& rhs);
               friend std::ostream& operator << ( std::ostream& out, const Lookup& value);


            protected:

               bool update( Reply&& reply);

               std::string m_service;
               Uuid m_correlation;
               optional< Reply> m_reply;
            };
         } // detail

         struct Lookup : detail::Lookup
         {
            using detail::Lookup::Lookup;

            //!
            //! @return the reply from the `service-manager`
            //!    can only be either idle (reserved) or busy.
            //!
            //!    if busy, a second invocation will block until it's idle
            //!
            //! @throws common::exception::xatmi::service::no::Entry if the service is not present or discovered
            //!
            const Reply& operator () ();
         };

         namespace non
         {
            namespace blocking
            {
               //! non-blocking lookup
               //! 

               struct Lookup : detail::Lookup
               {
                  using detail::Lookup::Lookup;

                  //! return true if the service is ready to be called
                  //! @throws common::exception::xatmi::service::no::Entry if the service is not present or discovered
                  explicit operator bool ();

                  //! converts this non-blocking to a blocking lookup
                  //! usage:
                  //! void some_function( service::Lookup&& lookup);
                  //!
                  //! service::non::blocking::Lookup lookup( "someService");
                  //!
                  //! if( lookup)
                  //!   some_function( std::move( lookup));
                  //! 
                  operator service::Lookup () &&;
           
               };
            } // blocking
         } // non
      } // service
   } // common
} // casual


