//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/serialize/macro.h"
#include "common/string.h"

#include <string>
#include <vector>
#include <iosfwd>

namespace casual
{
   namespace common::service::header
   {
      inline namespace v1
      {
         struct Field
         {
            Field() = default;

            //! @pre expects the format `<key>[ ]?:[ ]?<value>`
            explicit Field( std::string_view field);

            inline Field( std::string key, std::string value) 
               : key{ std::move( key)}, value{ std::move( value)} {}

            std::string key;
            std::string value;

            //! @returns <key> : <value>  
            std::string http() const;

            friend bool operator == ( const Field& lhs, const Field& rhs);

            friend bool operator == ( const Field& lhs, std::string_view key);
            inline friend bool operator == ( std::string_view key, const Field& rhs) { return rhs == key;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( key);
               CASUAL_SERIALIZE( value);
            )
         };

         
         struct Fields : std::vector< header::Field>
         {
            using fields_type = std::vector< header::Field>;
            using fields_type::fields_type;

            //! @param key to find
            //! @return true if field with @p key exists
            bool exists( std::string_view key) const; 
         
            //! @param key to be found
            //! @return the value associated with the key
            //! @throws exception::system::invalid::Argument if key is not found.
            const std::string& at( std::string_view key) const;

            template< typename T>
            T at( std::string_view key) const
            {
               return string::from< T>( at( key));
            }

            //! @param key to be found
            //! @return the value associated with the key, if not found, @p optional is returned
            std::string at( std::string_view key, std::string_view optional) const;
         
            template< typename T>
            T at( std::string_view key, const std::string& optional) const
            {
               return string::from< T>( at( key, optional));
            }

            std::optional< std::string> find( std::string_view key) const;

            //! Same semantics as std::map[]  
            std::string& operator[] ( std::string_view key);

            //! Same semantics as at
            const std::string& operator[]( std::string_view key ) const;
            

            inline fields_type& container() { return *this;}
            inline const fields_type& container() const { return *this;}

            friend Fields operator + ( Fields lhs, const Fields& rhs);
            friend Fields& operator += ( Fields& lhs, const Fields& rhs);
         };


         Fields& fields();
         void fields( Fields fields);

      }
   } // common::service::header
} // casual


