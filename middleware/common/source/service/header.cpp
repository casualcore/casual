//!
//! casual 
//!

#include "common/service/header.h"

#include "common/algorithm.h"
#include "common/exception.h"

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
                     auto find( const std::string& key) -> decltype( range::make( header::fields()))
                     {
                        return range::find_if( fields(), [&]( const Field& f){
                           return f.key == key;
                        });
                     }

                  } // <unnamed>
               } // local

               bool operator == (  const Field& lhs, const Field& rhs)
               {
                  return lhs.key == rhs.key;
               }

               std::ostream& operator << ( std::ostream& out, const Field& value)
               {
                  return out << "{ key: " << value.key << ", value: " << value.value << '}';
               }

               std::vector< header::Field>& fields()
               {
                  static std::vector< header::Field> fields;
                  return fields;
               }

               void fields( std::vector< header::Field> header)
               {
                  log::internal::debug << "header: " << range::make( header) << '\n';
                  fields() = std::move( header);
               }

               bool exists( const std::string& key)
               {
                  return local::find( key);
               }


               const std::string& get( const std::string& key)
               {
                  auto found = local::find( key);

                  if( found)
                  {
                     return found->value;
                  }
                  throw exception::invalid::Argument{ "service header key not found", CASUAL_NIP( key)};
               }

               std::string get( const std::string& key, const std::string& default_value)
               {
                  auto found = local::find( key);

                  if( found)
                  {
                     return found->value;
                  }
                  return default_value;
               }

               namespace replace
               {
                  void add( Field field)
                  {
                     auto found = range::find( fields(), field);

                     if( found)
                     {
                        *found = std::move( field);
                     }
                     else
                     {
                        fields().push_back( std::move( field));
                     }
                  }
               } // replace

               void clear()
               {
                  fields().clear();
               }
            }
         } // header
      } // service
   } // common
} // casual

