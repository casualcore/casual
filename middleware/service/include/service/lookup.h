//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/service.h"

#include "casual/transaction/id.h"

#include <optional>
#include <string>

namespace casual
{
   namespace service
   {
      struct Lookup;

      namespace lookup
      {
         using Context = common::message::service::lookup::request::Context;
         using Reply = common::message::service::lookup::Reply;
         using State = common::message::service::lookup::reply::State;

         //! consume the lookup and block for the _lookup reply_.
         lookup::Reply reply( Lookup&& lookup);

         namespace non::blocking
         {
            std::optional< lookup::Reply> reply( Lookup& lookup);

         } // non::blocking

         //! discard the lookup
         void discard( lookup::Reply&& lookup);
               
      } // lookup

      struct Lookup
      {                        
         //! Lookup an entry point for the `service`. `trid` represent the current transaction to give
         //! SM a chance to keep calls within the same transaction to end up at the same destination.
         Lookup( std::string service, const common::transaction::ID& trid, std::optional< common::chronology::time_point> deadline = {});

         //! Lookup an entry point for the `service`. `trid` represent the current transaction to give
         //! SM a chance to keep calls within the same transaction to end up at the same destination.
         //! `context` could be used for specific semantics.
         Lookup( std::string service, const common::transaction::ID& trid, lookup::Context context, std::optional< common::chronology::time_point> deadline = {});

         //! If pending lookup discard it.
         ~Lookup();

         Lookup( Lookup&&) noexcept;
         Lookup& operator = ( Lookup&&) noexcept;

         inline common::strong::correlation::id correlation() const noexcept { return m_correlation;}

         friend lookup::Reply lookup::reply( Lookup&& lookup);
         friend std::optional< lookup::Reply> lookup::non::blocking::reply( Lookup& lookup);

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( m_service);
            CASUAL_SERIALIZE( m_correlation);
         )
      protected:
         std::string m_service;
         common::strong::correlation::id m_correlation;
      };

   } // service
} // casual


