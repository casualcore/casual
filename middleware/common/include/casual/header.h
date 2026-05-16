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
   inline namespace v1
   {
      namespace header
      {
         struct Field 
         {
            Field() = default;
            explicit Field( std::string variable);
            Field( std::string_view name, std::string_view value);

            //! @returns the name part of the field
            std::string_view name() const &;
            //! @returns the value part of the field
            std::string_view value() const &;

            //! @returns true of the name of the field is case-insensitive equal to @p rhs
            friend bool operator == ( const Field& lhs, std::string_view rhs);
         
            inline friend bool operator == ( const Field& lhs, const Field& rhs) = default;
            inline friend auto operator <=> ( const Field& lhs, const Field& rhs) = default;

            inline const std::string& string() const & { return m_data;}
            inline std::string extract() && { return std::move( m_data);}

            inline friend std::ostream& operator << ( std::ostream& out, const Field& field) { return out << field.m_data;}

            CASUAL_FORWARD_SERIALIZE( m_data);

         private:
            std::string m_data;
         };

         
         struct Fields
         {
            Fields() = default;
            Fields( std::vector< header::Field> fields);

            void add( header::Field field);

            //! @param name to find
            //! @return true if field with @p name exists
            bool contains( std::string_view name) const; 
         
            //! @param name to be found
            //! @return the value associated with the name
            //! @throws casual::common::code::casual::invalid_argument if the name is not found
            const header::Field& at( std::string_view name) const;

            //! @returns the field with @p name or nullptr if not found
            const header::Field* find( std::string_view name) const;

            //! @returns and removes the field with @p name or nullopt if not found
            std::optional< header::Field> extract( std::string_view name);

            friend Fields operator + ( Fields lhs, const Fields& rhs);
            friend Fields& operator += ( Fields& lhs, const Fields& rhs);

            inline void clear() { m_fields.clear();}
            inline bool empty() const noexcept { return m_fields.empty();}
            inline platform::size::type size() const noexcept { return m_fields.size();}

            inline auto begin() const noexcept { return std::begin( m_fields);}
            inline auto begin() noexcept { return std::begin( m_fields);}
            inline auto end() const noexcept { return std::end( m_fields);}
            inline auto end() noexcept { return std::end( m_fields);}

            inline friend bool operator == ( const Fields&, const Fields&) = default;
            inline friend auto operator <=> ( const Fields&, const Fields&) = default;

            CASUAL_FORWARD_SERIALIZE( m_fields);

         private:
            std::vector< header::Field> m_fields;
         };
         
      } // header

      struct Header
      {
         header::Fields fields;

         inline friend bool operator == ( const Header&, const Header&) = default;
         inline friend auto operator <=> ( const Header&, const Header&) = default;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( fields);
         )
      };

      namespace header
      {
         //! @returns a flattened representation of the fields, separated by '\n'
         std::string flatten( const Header& header);
         //! @returns parsed fields from the flattened representation
         Header parse( std::string_view flattened);

         Header transform( const std::vector< std::string>& fields);
         Header transform( std::vector< std::string>&& fields);

         std::vector< std::string> transform( const Header& header);
         std::vector< std::string> transform( Header&& header);
         
      } // header


   } // v1
   
} // casual



