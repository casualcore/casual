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
            Field( std::string variable);
            Field( std::string_view name, std::string_view value);

            inline std::string_view name() const noexcept
            { 
               return { std::begin( m_data), std::find( std::begin( m_data), std::end( m_data), ':')};
            }
            inline std::string_view value() const noexcept
            {  
               if( auto found = algorithm::find( m_data, ':'))
                  return { std::begin( found) + 1, std::end( found)};

               return {};
            }

            inline friend bool operator == ( const Field& lhs, std::string_view rhs) { return lhs.name() == rhs;}

            inline const std::string& string() const noexcept { return m_data;}

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
            //! @throws exception::system::invalid::Argument if name is not found.
            const header::Field& at( std::string_view name) const;

            const header::Field* find( std::string_view name) const;

            friend Fields operator + ( Fields lhs, const Fields& rhs);
            friend Fields& operator += ( Fields& lhs, const Fields& rhs);

            inline void clear() { m_fields.clear();}
            inline bool empty() const noexcept { return m_fields.empty();}
            inline platform::size::type size() const noexcept { return m_fields.size();}

            inline auto begin() const noexcept { return std::begin( m_fields);}
            inline auto end() const noexcept { return std::end( m_fields);}

            CASUAL_FORWARD_SERIALIZE( m_fields);

         private:
            std::vector< header::Field> m_fields;
         };


         Fields& fields();
         void fields( Fields fields);

      }
   } // common::service::header
} // casual


