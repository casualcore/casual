//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log/stream.h"
#include "common/log/trace.h"

#include "common/string/compose.h"

namespace casual
{
   namespace configuration
   {
      namespace alias
      {
         namespace generate
         {
            //! @returns a unique string used to correlate aliases that shall be generated later
            //! to a more human suitable representation 
            std::string placeholder();
            
         } // generate

         namespace is
         {
            bool placeholder( const std::string& alias);
         } // is

         namespace normalize
         {
            struct State
            {
               std::map< std::string, platform::size::type> count;
               std::map< std::string, std::string> placeholders;
            };

            template< typename P>
            auto mutator( State& state, P&& prospect)
            {
               return [ &state, prospect = std::forward< P>( prospect)]( auto& value)
               {
                  std::string placeholder;

                  if( value.alias.empty())
                     value.alias = prospect( value);
                  else if( alias::is::placeholder( value.alias))
                     placeholder = std::exchange( value.alias, prospect( value));
                     
                  auto potentally_add_index = []( auto& state, auto& alias)
                  {
                     auto count = ++state.count[ alias];

                     if( count == 1)
                        return false;

                     alias = common::string::compose( alias, ".", count);
                     return true;
                  };

                  while( potentally_add_index( state, value.alias))
                     ; // no-op

                  if( ! placeholder.empty())
                     state.placeholders.emplace( placeholder, value.alias);
               };
            }
         } // normalize

      } // alias


      extern common::log::Stream log;

      namespace verbose
      {
         extern common::log::Stream log;
      } // verbose

      namespace trace
      {
         extern common::log::Stream log;
      } // trace

      struct Trace : common::log::Trace
      {
         template< typename T>
         Trace( T&& value) : common::log::Trace( std::forward< T>( value), trace::log) {}
      };

   } // configuration
} // casual


