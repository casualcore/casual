//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_HEADER_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_HEADER_H_


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

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & key;
                     archive & value;
                  })

                  friend bool operator == (  const Field& lhs, const Field& rhs);
                  friend std::ostream& operator << ( std::ostream& out, const Field& value);

               };


               std::vector< header::Field>& fields();
               void fields( std::vector< header::Field> fields);


               //!
               //! @param key to find
               //! @return true if field with @p key exists
               //!
               bool exists( const std::string& key);


               //!
               //!
               //! @param key to be found
               //! @return the value associated with the key
               //! @throws exception::system::invalid::Argument if key is not found.
               //!
               const std::string& get( const std::string& key);

               template< typename T>
               T get( const std::string& key)
               {
                  return from_string< T>( get( key));
               }


               //!
               //!
               //! @param key to be found
               //! @return the value associated with the key, if not found, @p default_value is returned
               //!
               std::string get( const std::string& key, const std::string& default_value);

               template< typename T>
               T get( const std::string& key, const std::string& default_value)
               {
                  return from_string< T>( get( key, default_value));
               }

               namespace replace
               {

                  //!
                  //! If key is found, value is replaced, otherwise field
                  //! is appended to fields.
                  //!
                  //! @param field
                  void add( Field field);

               } // replace


               //!
               //! clears the header.
               //!
               void clear();
            }
         } // header
      } // service
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_HEADER_H_
