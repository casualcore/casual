//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/service/header.h"

#include "common/algorithm.h"
#include "common/exception/system.h"
#include "common/log.h"

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
               namespace local
               {
                  namespace
                  {
                     template< typename F>
                     auto find( F& fields, const std::string& key) // -> decltype( range::make( header::fields()))
                     {
                        return algorithm::find_if( fields, [&]( const Field& f){
                           return f.equal( key);
                        });
                     }

                     template< typename F>
                     decltype( auto) find_at( F& fields, const std::string& key) // -> decltype( range::make( header::fields()))
                     {
                        auto found = find( fields, key);
                        
                        if( found) 
                           return *found;
   
                        throw exception::system::invalid::Argument{ "service header key not found: " + key};
                     }

                  } // <unnamed>
               } // local


               bool Field::equal( const std::string& value) const
               {
                  return algorithm::equal( key, value, 
                     []( auto a, auto b){ return std::tolower(a) == std::tolower(b);});
               }

               std::string Field::http() const
               {
                  return key + ": " + value;
               }

               bool Fields::exists( const std::string& key) const
               {
                  return local::find( *this, key);
               }

               const std::string& Fields::at( const std::string& key) const
               {
                  return local::find_at( *this, key).value;
               }

               std::string Fields::at( const std::string& key, const std::string& default_value) const
               {
                  auto found = local::find( *this, key);

                  if( found)
                  {
                     return found->value;
                  }
                  return default_value;
               }

               std::string& Fields::operator[] ( const std::string& key )
               {
                  auto found = local::find( *this, key);
                  
                  if( found)
                  {
                     return found->value;
                  }

                  fields_type::emplace_back( key, "");
                  return fields_type::back().value;
               }

               const std::string& Fields::operator[]( const std::string& key ) const
               {
                  return at( key);
               }

               bool operator == (  const Field& lhs, const Field& rhs)
               {
                  return lhs.equal( rhs.key);
               }

               std::ostream& operator << ( std::ostream& out, const Field& value)
               {
                  return out << "{ key: " << value.key << ", value: " << value.value << '}';
               }

               header::Fields& fields()
               {
                  static header::Fields fields;
                  return fields;
               }

               void fields( header::Fields header)
               {
                  log::debug << "header: " << range::make( header) << '\n';
                  fields() = std::move( header);
               }

            } // v1
         } // header
      } // service
   } // common
} // casual

