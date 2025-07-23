//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/serialize/macro.h"
#include "common/string.h"
#include "common/buffer/type.h"

#include <string>
#include <vector>
#include <iosfwd>

namespace casual
{
   namespace header
   {
      inline namespace v1
      {
         struct Field 
         {
            Field() = default;
            explicit Field( std::string variable);
            Field( std::string_view name, std::string_view value);

            //! @returns the name part of the field
            std::string_view name() const;
            //! @returns the value part of the field
            std::string_view value() const;

            //! @returns true of the name of the field is case-insensitive equal to @p rhs
            friend bool operator == ( const Field& lhs, std::string_view rhs);

            //! @returns true of the lhs.name() is case-insensitive equal to rhs.name()
            friend bool operator == ( const Field& lhs, const Field& rhs);

            inline const std::string& string() const & { return m_data;}

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

            friend Fields operator + ( Fields lhs, const Fields& rhs);
            friend Fields& operator += ( Fields& lhs, const Fields& rhs);

            inline void clear() { m_fields.clear();}
            inline bool empty() const noexcept { return m_fields.empty();}
            inline platform::size::type size() const noexcept { return m_fields.size();}

            inline auto begin() const noexcept { return std::begin( m_fields);}
            inline auto end() const noexcept { return std::end( m_fields);}

            inline friend bool operator == ( const Fields& lhs, const Fields& rhs) = default;

            CASUAL_FORWARD_SERIALIZE( m_fields);

         private:
            std::vector< header::Field> m_fields;
         };
         

      } // v1
   } // header
} // casual


