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
                     auto find( F& fields, const std::string& key)
                     {
                        return algorithm::find_if( fields, [&]( const Field& f){
                           return f.equal( key);
                        });
                     }

                     template< typename F>
                     decltype( auto) find_at( F& fields, const std::string& key)
                     {  
                        if( auto found = find( fields, key))
                           return *found;
                        
                        code::raise::log( code::casual::invalid_argument, "service header key not found: ", key);
                     }

                  } // <unnamed>
               } // local

               bool operator == (  const Field& lhs, const Field& rhs)
               {
                  return lhs.equal( rhs.key);
               }

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
                  return ! local::find( *this, key).empty();
               }

               const std::string& Fields::at( const std::string& key) const
               {
                  return local::find_at( *this, key).value;
               }

               std::string Fields::at( const std::string& key, const std::string& optional) const
               {
                  if( auto found = local::find( *this, key))
                     return found->value;
                
                  return optional;
               }

	       std::optional< std::string> Fields::find( const std::string& key) const
               {
                  if( auto found = local::find( *this, key))
                     return { found->value};

                  return {};
               }

               std::string& Fields::operator[] ( const std::string& key )
               {
                  if( auto found = local::find( *this, key))
                     return found->value;

                  fields_type::emplace_back( key, "");
                  return fields_type::back().value;
               }

               const std::string& Fields::operator[]( const std::string& key ) const
               {
                  return at( key);
               }

               Fields operator + ( const Fields& lhs, const Fields& rhs)
               {
                  auto result = lhs;
                  algorithm::append( rhs, result);
                  return result;
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
         } // header
      } // service
   } // common
} // casual

