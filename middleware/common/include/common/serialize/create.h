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
      using range_type = range::type_t< std::array< std::string_view, 1>>;

      namespace reader
      {
         namespace consumed 
         {
            namespace detail
            {
               void registration( reader::Creator&& creator, range_type keys);
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
               void registration( reader::Creator&& creator, range_type keys);
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
               void registration( reader::Creator&& creator, range_type keys);
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
         std::vector< std::string_view> keys();

         namespace detail
         {
            namespace indirection
            {

               template< typename Value, typename... Values>
               inline constexpr bool value_any_of( Value value, Values... values)
               {
                  static_assert( concepts::same_as< Value, Values...>, "all compared values has to be the same type");

                  return ( ... || (value == values) );
               }


               template< typename Implementation, typename Range> 
               auto registration( Range keys) 
                  -> std::enable_if_t< value_any_of( 
                        Implementation::archive_type(), 
                        archive::Type::static_need_named,
                        archive::Type::dynamic_type), bool>
               {
                  relaxed::detail::registration( reader::Creator::construct< Implementation, policy::Relaxed>(), keys);
                  strict::detail::registration( reader::Creator::construct< Implementation, policy::Strict>(), keys);
                  consumed::detail::registration( reader::Creator::construct< Implementation, policy::Consumed>(), keys);

                  return true;
               }
            } // indirection
         } // detail

         template< typename Implementation>
         auto registration()
         {
            auto keys = Implementation::keys();
            return detail::indirection::registration< Implementation>( range::make( keys));
         }

         template< typename P>
         struct Registration
         {
         private:
            [[maybe_unused]] static bool m_dummy;
         };

         template< typename I> 
         [[maybe_unused]] bool Registration< I>::m_dummy = reader::registration< I>();

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
            void registration( writer::Creator&& creator, range_type keys);

            //! helper to convert _string view array_ to the known range_type (during the lifetime of the expression)
            template< typename T>
            auto registration( writer::Creator&& creator, T&& keys) 
               -> std::enable_if_t< std::is_same_v< std::remove_cvref_t< decltype( range::make( keys))>, range_type>>
            {
               registration( std::move( creator), range::make( keys));
            } 
            
         } // detail
         
         serialize::Writer from( std::string_view key);

         template< typename I> 
         auto create() { return detail::create< I>();}

         //! @returns all registred writer keys
         std::vector< std::string_view> keys();

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
            [[maybe_unused]] static bool m_dummy;
         };

         template< typename I> 
         [[maybe_unused]] bool Registration< I>::m_dummy = writer::registration< I>();



         namespace complete
         {
            inline auto format() 
            {
               return []( auto values, bool) -> std::vector< std::string>
               {
                  return common::algorithm::transform( writer::keys(), []( auto& view){ return std::string( view);});
               };
            }
         } // complete

      } // writer

   } // common::serialize::create
} // casual
