//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/service/header.h"

#include "common/algorithm.h"
#include "common/log.h"
#include "common/serialize/line.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

namespace casual
{
   namespace common::service::header
   {
      inline namespace v1
      {
         namespace local
         {
            namespace
            {
               template< typename F>
               auto find( F& fields, std::string_view key)
               {
                  return algorithm::find( fields, key);
               }

               template< typename F>
               decltype( auto) find_at( F& fields, std::string_view key)
               {  
                  if( auto found = find( fields, key))
                     return *found;
                  
                  code::raise::error( code::casual::invalid_argument, "service header key not found: ", key);
               }

            } // <unnamed>
         } // local

         Field::Field( std::string_view field)
         {

            if( auto found = algorithm::find( field, ':'))
            {
               auto key_range = string::trim( range::make( std::begin( field), std::begin( found)));
               key.assign( std::begin( key_range), std::end( key_range));
               auto value_range = string::trim( ++found);
               value.assign( std::begin( value_range), std::end( value_range));
            }
            else 
               code::raise::error( code::casual::invalid_argument, "requires format '<key>[ ]?:[ ]?<value>'");
         }

         bool operator == (  const Field& lhs, const Field& rhs)
         {
            return lhs == rhs.key;
         }

         bool operator == (  const Field& lhs, std::string_view key)
         {
            return algorithm::equal( lhs.key, key, []( auto a, auto b)
            { 
               return std::tolower(a) == std::tolower(b);
            });
         }

         std::string Field::http() const
         {
            return string::compose( key, ": ", value);
         }

         bool Fields::exists( std::string_view key) const
         {
            return ! local::find( *this, key).empty();
         }

         const std::string& Fields::at( std::string_view key) const
         {
            return local::find_at( *this, key).value;
         }

         std::string Fields::at( std::string_view key, std::string_view optional) const
         {
            if( auto found = local::find( *this, key))
               return found->value;
            
            return std::string{ optional};
         }

         std::optional< std::string> Fields::find( std::string_view key) const
         {
            if( auto found = local::find( *this, key))
               return { found->value};

            return {};
         }

         std::string& Fields::operator[] ( std::string_view key )
         {
            if( auto found = local::find( *this, key))
               return found->value;

            return fields_type::emplace_back( std::string{ key}, "").value;
         }

         const std::string& Fields::operator[]( std::string_view key ) const
         {
            return at( key);
         }

         Fields operator + ( Fields lhs, const Fields& rhs)
         {
            lhs += rhs;
            return lhs;
         }

         Fields& operator += ( Fields& lhs, const Fields& rhs)
         {
            algorithm::append( rhs, lhs);
            return lhs;
         }


         header::Fields& fields()
         {
            static header::Fields fields;
            return fields;
         }

         void fields( header::Fields header)
         {
            log::line( verbose::log, "header: ", header);
            fields() = std::move( header);
         }

      } // v1
   } // common::service::header
} // casual

