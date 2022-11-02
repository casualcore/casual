//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/communication/tcp/message.h"
#include "common/communication/device.h"
#include "common/serialize/native/network.h"

#include <iosfwd>


namespace casual
{
   namespace common::communication::stream
   {

      // Forwards
      namespace inbound
      {
         struct Connector;
      } // inbound

      namespace outbound
      {
         struct Connector;
      } // inbound

      namespace message
      {
         using Header = communication::tcp::message::Header;

         namespace header
         {
            using namespace communication::tcp::message::header;
         } // header

         //! TODO: stream probably needs it's own complete
         struct Complete : communication::tcp::message::Complete
         {
            using communication::tcp::message::Complete::Complete;
         };
      } // message

      namespace policy
      {
         using complete_type = message::Complete;
         using cache_type = std::vector< complete_type>;
         using cache_range_type = range::type_t< cache_type>;

         struct Blocking
         {
            cache_range_type receive( inbound::Connector& connector, cache_type& cache);
            strong::correlation::id send( outbound::Connector& connector, complete_type&& complete);
         };

         namespace non
         {
            struct Blocking
            {
               cache_range_type receive( inbound::Connector& connector, cache_type& cache);
            };  
         } // non
         
      } // policy

      namespace outbound
      {
         struct Connector
         {
            using blocking_policy = policy::Blocking;
            using complete_type = policy::complete_type;

            inline Connector( std::ostream& out) : m_out{ &out} {}

            inline std::ostream& stream() { return *m_out;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_out, "out");
            )

         private:
            std::ostream* m_out;
         };

         using Device = communication::device::Outbound< Connector>;
      } // outbound

      namespace inbound
      {
         struct Connector
         {
            using blocking_policy = policy::Blocking;
            using non_blocking_policy = policy::non::Blocking;
            using cache_type = policy::cache_type;

            inline Connector( std::istream& in) : m_in{ &in} {}

            inline std::istream& stream() { return *m_in;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_in, "in");
            )

         private:
            std::istream* m_in;
         };

         using Device = communication::device::Inbound< Connector>;
      } // inbound

   } // common::communication::stream

   namespace common::serialize::native::customization
   {
      template<>
      struct point< communication::stream::message::Complete>
      {
         using writer = binary::network::create::Writer;
         using reader = binary::network::create::Reader;
      };
         
   } // common::serialize::native::customization
} // casual