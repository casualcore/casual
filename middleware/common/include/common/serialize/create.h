//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/create/writer.h"
#include "common/serialize/create/reader.h"

#include "common/serialize/archive.h"
#include "common/serialize/policy.h"

#include "casual/platform.h"

#include <string>
#include <iosfwd>

namespace casual
{
   namespace common::serialize::create
   {
      namespace reader
      {
         namespace consumed 
         {
            namespace detail
            {
               void registration( reader::Creator&& creator, const std::vector< std::string>& keys);
            } // detail

            template< typename I, typename S> 
            auto create( S&& source) { return Reader::emplace< policy::Consumed< I>>( std::forward< S>( source));}

            serialize::Reader from( std::string_view key, std::istream& stream);
            serialize::Reader from( std::string_view key, const platform::binary::type& data);

            template< typename F>
            inline auto from( F&& file)
            { 
               return from( file.path().extension().string(), file);
            }
         }

         namespace strict 
         {
            namespace detail
            {
               void registration( reader::Creator&& creator, const std::vector< std::string>& keys);
            } // detail

            template< typename I, typename S> 
            auto create( S&& source) { return Reader::emplace< policy::Strict< I>>( std::forward< S>( source));}

            serialize::Reader from( std::string_view key, std::istream& stream);
            serialize::Reader from( std::string_view key, const platform::binary::type& data);


            template< typename F>
            inline auto from( F&& file)
            { 
               return from( file.path().extension().string(), file);
            }
         }

         namespace relaxed 
         {
            namespace detail
            {
               void registration( reader::Creator&& creator, const std::vector< std::string>& keys);
            } // detail

            template< typename I, typename S> 
            auto create( S&& source) { return Reader::emplace< policy::Relaxed< I>>( std::forward< S>( source));}

            serialize::Reader from( std::string_view key, std::istream& stream);
            serialize::Reader from( std::string_view key, const platform::binary::type& data);

            template< typename F>
            inline auto from( F&& file)
            { 
               return from( file.path().extension().string(), file);
            }
         }

         //! @returns all registred reader keys
         std::vector< std::string> keys();

         namespace detail
         {
            namespace indirection
            {
               template< typename Implementation> 
               auto registration( const std::vector< std::string>& keys) 
                  -> std::enable_if_t< Implementation::archive_type() != archive::Type::static_order_type>
               {
                  relaxed::detail::registration( reader::Creator::construct< Implementation, policy::Relaxed>(), keys);
                  strict::detail::registration( reader::Creator::construct< Implementation, policy::Strict>(), keys);
                  consumed::detail::registration( reader::Creator::construct< Implementation, policy::Consumed>(), keys);
               }
            } // indirection
         } // detail

         template< typename Implementation>
         auto registration()
         {
            const auto keys = Implementation::keys();
            detail::indirection::registration< Implementation>( keys);
            return true;
         }

         template< typename P>
         struct Registration
         {
         private:
            static CASUAL_MAYBE_UNUSED bool m_dummy;
         };

         template< typename I> 
         CASUAL_MAYBE_UNUSED bool Registration< I>::m_dummy = reader::registration< I>();

         namespace complete
         {
            inline auto format() 
            {
               return []( auto values, bool)
               {
                  return reader::keys();
               };
            }
         } // complete

      } // reader

      namespace writer
      {
         namespace detail
         {
            void registration( writer::Creator&& creator, const std::vector< std::string>& keys);
         } // detail
         
         serialize::Writer from( std::string_view key);

         template< typename I> 
         auto create() { return detail::create< I>();}

         //! @returns all registred writer keys
         std::vector< std::string> keys();

         template< typename Implementation>
         auto registration()
         {
            detail::registration( writer::Creator::construct< Implementation>(), Implementation::keys());
            return true;
         }

         template< typename P>
         struct Registration
         {
         private:
            static CASUAL_MAYBE_UNUSED bool m_dummy;
         };

         template< typename I> 
         CASUAL_MAYBE_UNUSED bool Registration< I>::m_dummy = writer::registration< I>();



         namespace complete
         {
            inline auto format() 
            {
               return []( auto values, bool)
               {
                  return writer::keys();
               };
            }
         } // complete

      } // writer

   } // common::serialize::create
} // casual
