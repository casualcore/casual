//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/header.h"

#include "common/algorithm/container.h"
#include "common/log.h"
#include "common/serialize/line.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

namespace casual
{
   using namespace common;
   namespace header
   {
      inline namespace v1
      {
         
         Field::Field( std::string variable)
            : m_data{ std::move( variable)}
         {
            algorithm::container::erase_if( m_data, []( auto value)
            {
               return std::isspace( value);
            });
         }

         Field::Field( std::string_view name, std::string_view value)
            : Field{ string::compose( name, ':', value)}
         {}

         std::string_view Field::name() const &
         { 
            return { std::begin( m_data), std::find( std::begin( m_data), std::end( m_data), ':')};
         }

         std::string_view Field::value() const &
         {  
            if( auto found = algorithm::find( m_data, ':'))
               return { std::begin( found) + 1, std::end( found)};

            return {};
         }

         bool operator == ( const Field& lhs, std::string_view rhs) 
         {
            auto case_insensitive_equal = []( auto lhs, auto rhs)
            {
               return std::tolower( lhs) == std::tolower( rhs);
            };

            return algorithm::equal( lhs.name(), rhs, case_insensitive_equal);
         }

         Fields::Fields( std::vector< header::Field> fields)
            : m_fields{ std::move( fields)}
         {}
         
         void Fields::add( header::Field field)
         {
            if( auto found = algorithm::find( m_fields, field.name()))
               *found = std::move( field);
            else
               m_fields.push_back( std::move( field));
         }

         bool Fields::contains( std::string_view key) const
         {
            return algorithm::contains( m_fields, key);
         }

         const header::Field& Fields::at( std::string_view key) const
         {
            if( auto found = find( key))
               return *found;

            code::raise::error( code::casual::invalid_argument, "failed to find key in header::Fields: ", key);
         }

         const header::Field* Fields::find( std::string_view key) const
         {
            if( auto found = algorithm::find( m_fields, key))
               return found.data();

            return nullptr;
         }


         Fields operator + ( Fields lhs, const Fields& rhs)
         {
            lhs += rhs;
            return lhs;
         }

         Fields& operator += ( Fields& lhs, const Fields& rhs)
         {
            algorithm::container::append( rhs.m_fields, lhs.m_fields);
            return lhs;
         }
 
         
         std::string flatten( const Fields& fields)
         {
            if( fields.empty())
               return {};

            // TODO: optimize, we could pre-calculate the size

            return common::string::join( fields, '\n');
         }

         
         Fields parse( std::string_view flattened)
         {
            Fields result;

            for( auto&& line : flattened | std::views::split( '\n'))
               result.add( Field{ std::string{ std::begin( line), std::end( line)}});
               
            return result;
         }

      } // v1
   } // header
} // casual

