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
   namespace common::communication::stream
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

                  message::Complete complete;
                  auto current = complete.header_data();

                  // First we read the header
                  if( ! in.read( current, message::header::size))
                     code::raise::error( code::casual::communication_unavailable, "stream is unavailable - header");

                  complete.offset +=  message::header::size;
                  complete.payload.resize( complete.size());

                  auto span = binary::span::to_string_like( complete.payload);

                  if( ! in.read( span.data(), span.size()))
                     code::raise::error( code::casual::communication_unavailable, "stream is unavailable - payload");

                  complete.offset += complete.payload.size();

                  log::line( verbose::log, "stream <-- ", complete);

                  return complete;
               }

               auto send( std::ostream& out, message::Complete& complete)
               {
                  Trace trace{ "common::communication::stream::policy::local::send"};

                  // First we write the header
                  {
                     auto data = complete.header_data();
                     if( ! out.write( data, message::header::size))
                        code::raise::error( code::casual::communication_unavailable, "stream is unavailable");

                     complete.offset += message::header::size;
                  }

                  // write the complete message
                  {
                     auto char_span = binary::span::to_string_like( complete.payload);
                     if( ! out.write( char_span.data(), char_span.size()))
                        code::raise::error( code::casual::communication_unavailable, "stream is unavailable");

                     complete.offset += complete.payload.size();
                  }
                     
                  log::line( verbose::log, "stream --> ", complete);

                  return complete.correlation();
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

         strong::correlation::id Blocking::send( outbound::Connector& connector, complete_type&& complete)
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

   } // common::communication::stream
} // casual