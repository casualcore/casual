//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/communication/stream.h"

#include "common/log.h"
#include "common/code/raise.h"
#include "common/code/casual.h"

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace stream
         {
            namespace policy
            {
               namespace local
               {
                  namespace
                  {
                     auto receive( std::istream& in)
                     {
                        Trace trace{ "common::communication::stream::policy::local::receive"};

                        communication::message::complete::network::Header header{};
                        auto current = reinterpret_cast< char*>( &header);

                        // First we read the header
                        if( ! in.read( current, communication::message::complete::network::header::size()))
                           code::raise::log( code::casual::communication_unavailable, "stream is unavailable - header");

                        // we now know the size, read the complete message
                        communication::message::Complete message{ header};

                        if( ! in.read( message.payload.data(), message.payload.size()))
                           code::raise::log( code::casual::communication_unavailable, "stream is unavailable - payload");

                        log::line( verbose::log, "stream <-- ", message);

                        return message;
                     }

                     auto send( std::ostream& out, const communication::message::Complete& complete)
                     {
                        Trace trace{ "common::communication::stream::policy::local::send"};

                        // First we write the header
                        {
                           auto header = complete.header();

                           auto data = reinterpret_cast< const char*>( &header);
                           if( ! out.write( data, communication::message::complete::network::header::size()))
                              code::raise::log( code::casual::communication_unavailable, "stream is unavailable");
                        }

                        // write the complete message
                        if( ! out.write( complete.payload.data(), complete.payload.size()))
                           code::raise::log( code::casual::communication_unavailable, "stream is unavailable");

                        log::line( verbose::log, "stream --> ", complete);

                        return complete.correlation;
                     }
   
                  } // <unnamed>
               } // local

               cache_range_type Blocking::receive( inbound::Connector& connector, cache_type& cache)
               {
                  Trace trace{ "common::communication::stream::policy::Blocking::receive"};

                  auto message = local::receive( connector.stream());

                  if( ! message)
                     return {};

                  cache.push_back( std::move( message));
                  return policy::cache_range_type{ std::end( cache) - 1, std::end( cache)};
               }

               Uuid Blocking::send( outbound::Connector& connector, const communication::message::Complete& complete)
               {
                  Trace trace{ "common::communication::stream::policy::Blocking::send"};

                  return local::send( connector.stream(), complete);
               }


               namespace non
               {
                  cache_range_type Blocking::receive( inbound::Connector& connector, cache_type& cache)
                  {
                     Trace trace{ "common::communication::stream::policy::non::Blocking::receive"};

                     if( connector.stream().peek() == std::istream::traits_type::eof())
                        return {};

                     return policy::Blocking{}.receive( connector, cache);
                  }

               } // non
            } // policy

         } // stream

      } // communication
   } // common
} // casual