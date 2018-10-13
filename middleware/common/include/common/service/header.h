//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/marshal/marshal.h"
#include "common/string.h"

#include <string>
#include <vector>
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace header
         {
            inline namespace v1
            {
               struct Field
               {
                  Field() = default;
                  Field( std::string key, std::string value) : key{ std::move( key)}, value{ std::move( value)} {}

                  std::string key;
                  std::string value;

                  //!
                  //! case insensitive equal on key
                  //!
                  bool equal( const std::string& key) const;

                  //! @returns <key> : <value>  
                  std::string http() const;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & key;
                     archive & value;
                  })

                  friend bool operator == (  const Field& lhs, const Field& rhs);
                  friend std::ostream& operator << ( std::ostream& out, const Field& value);

               };

               
               struct Fields : std::vector< header::Field>
               {
                  using fields_type = std::vector< header::Field>;
                  using fields_type::fields_type;

                  //!
                  //! @param key to find
                  //! @return true if field with @p key exists
                  //!
                  bool exists( const std::string& key) const; 
               
                  //!
                  //!
                  //! @param key to be found
                  //! @return the value associated with the key
                  //! @throws exception::system::invalid::Argument if key is not found.
                  //!
                  const std::string& at( const std::string& key) const;

                  template< typename T>
                  T at( const std::string& key) const
                  {
                     return from_string< T>( at( key));
                  }


                  //!
                  //!
                  //! @param key to be found
                  //! @return the value associated with the key, if not found, @p default_value is returned
                  //!
                  std::string at( const std::string& key, const std::string& default_value) const;
               
                  template< typename T>
                  T at( const std::string& key, const std::string& default_value) const
                  {
                     return from_string< T>( at( key, default_value));
                  }

                  optional< const std::string&> find( const std::string& key) const;

                  //!
                  //! Same semantics as std::map[]  
                  //!
                  std::string& operator[] ( const std::string& key );

                  //!
                  //! Same semantics as at
                  //!
                  const std::string& operator[]( const std::string& key ) const;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & container();
                  })

                  inline fields_type& container() { return *this;}
                  inline const fields_type& container() const { return *this;}

                  friend Fields operator + ( const Fields& lhs, const Fields& rhs);
                  friend Fields& operator += ( Fields& lhs, const Fields& rhs);
               };


               Fields& fields();
               void fields( Fields fields);

            }
         } // header
      } // service
   } // common
} // casual


