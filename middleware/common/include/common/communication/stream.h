//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/communication/message.h"
#include "common/communication/device.h"
#include "common/serialize/native/network.h"

#include <iosfwd>


namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace stream
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

            namespace policy
            {
               using cache_type = device::inbound::cache_type;
               using cache_range_type = device::inbound::cache_range_type;

               struct Blocking
               {
                  cache_range_type receive( inbound::Connector& connector, cache_type& cache);
                  Uuid send( outbound::Connector& connector, const communication::message::Complete& complete);
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

                  inline Connector( std::ostream& out) : m_out{ &out} {}

                  inline std::ostream& stream() { return *m_out;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE_NAME( m_out, "out");
                  )

               private:
                  std::ostream* m_out;
               };

               using Device = communication::device::Outbound< Connector,  serialize::native::binary::network::create::Writer>;
            } // outbound

            namespace inbound
            {
               struct Connector
               {
                  using blocking_policy = policy::Blocking;
                  using non_blocking_policy = policy::non::Blocking;
                  inline Connector( std::istream& in) : m_in{ &in} {}

                  inline std::istream& stream() { return *m_in;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE_NAME( m_in, "in");
                  )

               private:
                  std::istream* m_in;
               };

               using Device = communication::device::Inbound< Connector, serialize::native::binary::network::create::Reader>;
            } // outbound


         } // stream

      } // communication
   } // common
} // casual