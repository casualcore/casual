//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/service/header.h"

#include "common/algorithm/container.h"
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

