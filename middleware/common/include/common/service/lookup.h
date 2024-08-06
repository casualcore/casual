//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/uuid.h"
#include "common/message/service.h"

#include <optional>
#include <string>

namespace casual
{
   namespace common::service
   {
      struct Lookup;

      namespace lookup
      {
         using Context = message::service::lookup::request::Context;
         using Reply = message::service::lookup::Reply;
         using State = message::service::lookup::reply::State;

         //! consume the lookup and block for the _lookup reply_.
         lookup::Reply reply( Lookup&& lookup);

         namespace non::blocking
         {
            std::optional< lookup::Reply> reply( Lookup& lookup);

         } // non::blocking
               
      } // lookup

      struct Lookup
      {                        
         //! Lookup an entry point for the @p service
         Lookup( std::string service, std::optional< platform::time::point::type> deadline = {});

         //! Lookup an entry point for the @p service
         //! using a specific context
         Lookup( std::string service, lookup::Context context, std::optional< platform::time::point::type> deadline = {});

         //! If pending lookup discard it.
         ~Lookup();

         Lookup( Lookup&&) noexcept;
         Lookup& operator = ( Lookup&&) noexcept;

         inline strong::correlation::id correlation() const noexcept { return m_correlation;}

         friend lookup::Reply lookup::reply( Lookup&& lookup);
         friend std::optional< lookup::Reply> lookup::non::blocking::reply( Lookup& lookup);

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( m_service);
            CASUAL_SERIALIZE( m_correlation);
         )
      protected:
         std::string m_service;
         strong::correlation::id m_correlation;
      };

   } // common::service
} // casual


