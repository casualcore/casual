//!
//! json.cpp
//!
//! Created on: Jan 18, 2015
//!     Author: Lazan
//!

#include "sf/archive/json.h"


#include "common/transcode.h"

#include <iterator>

namespace casual
{
   namespace sf
   {

      namespace archive
      {
         namespace json
         {

            Load::Load() : m_object( nullptr, json_object_put) {}
            Load::~Load() = default;

            void Load::serialize( std::istream& stream)
            {
               const std::string json{ std::istream_iterator< char>( stream), std::istream_iterator< char>()};
               serialize( json);
            }

            void Load::serialize( const std::string& json)
            {
               serialize( json.c_str());
            }

            void Load::serialize( const char* const json)
            {
               json_tokener_error result{ json_tokener_success};
               m_object.reset( json_tokener_parse_verbose( json, &result));

               if( result != json_tokener_success)
               {
                  throw exception::archive::invalid::Document{ json_tokener_error_desc( result)};
               }
            }

            json_object* Load::source() const
            {
               return m_object.get();
            }


            namespace reader
            {

               Implementation::Implementation( json_object* const object) : m_stack{ object} {}
               Implementation::~Implementation() = default;

               std::tuple< std::size_t, bool> Implementation::container_start( std::size_t size, const char* name)
               {

                  auto node = m_stack.back();

                  if( node)
                  {
                     if( name)
                     {
                        node = json_object_object_get( node, name);
                     }
                     else
                     {
                        m_stack.pop_back();
                     }
                  }

                  if( ! node)
                  {
                     return std::make_tuple( 0, false);
                  }


                  size = json_object_array_length( node);

                  for( int index = size; index > 0; --index)
                  {
                     m_stack.push_back( json_object_array_get_idx( node, index - 1));
                  }

                  return std::make_tuple( size, true);
               }

               void Implementation::container_end( const char*)
               {
               }

               bool Implementation::serialtype_start( const char* name)
               {
                  if( name)
                  {
                     m_stack.push_back( json_object_object_get( m_stack.back(), name));
                  }

                  return m_stack.back() != nullptr;
               }

               void Implementation::serialtype_end( const char*)
               {
                  m_stack.pop_back();
               }


               void Implementation::set( json_object* object, bool& value) { value = json_object_get_boolean( object); }
               void Implementation::set( json_object* object, short& value) { value = json_object_get_int( object); }
               void Implementation::set( json_object* object, long& value) { value = json_object_get_int64( object); }
               void Implementation::set( json_object* object, long long& value) { value = json_object_get_int64( object); }
               void Implementation::set( json_object* object, float& value) { value = json_object_get_double( object); }
               void Implementation::set( json_object* object, double& value) { value = json_object_get_double( object); }

               void Implementation::set( json_object* object, char& value)
               {
                  const auto string = common::transcode::utf8::decode( json_object_get_string( object));
                  if( !string.empty())
                  {
                     value = string[ 0];
                  }
               }
               void Implementation::set( json_object* object, std::string& value)
               {
                  value = common::transcode::utf8::decode( json_object_get_string( object));
               }

               void Implementation::set( json_object* object, platform::binary_type& value)
               {
                  value = common::transcode::base64::decode( json_object_get_string( object));
               }


            } // reader


            Save::Save() : m_object( json_object_new_object(), json_object_put) {}
            Save::~Save() = default;

            void Save::serialize( std::ostream& stream) const
            {
               //
               // TODO: Add some error-handling
               //

               stream << json_object_to_json_string_ext( m_object.get(), JSON_C_TO_STRING_PRETTY);
            }

            void Save::serialize( std::string& json) const
            {
               //
               // TODO: Add some error-handling
               //

               json = json_object_to_json_string_ext( m_object.get(), JSON_C_TO_STRING_PRETTY);
            }

            json_object* Save::target() const
            {
               return m_object.get();
            }


            namespace writer
            {

               Implementation::Implementation( json_object* const root) : m_stack{ root} {}

               Implementation::~Implementation() = default;

               std::size_t Implementation::container_start( std::size_t size, const char* const name)
               {
                  auto parent = m_stack.back();

                  auto array = json_object_new_array();

                  if( name)
                  {
                     json_object_object_add( parent, name, array);
                  }
                  else
                  {
                     //
                     // We are in a container already.
                     //
                     json_object_array_add( parent, array);
                  }

                  m_stack.push_back( array);

                  return size;
               }

               void Implementation::container_end( const char*)
               {
                  m_stack.pop_back();
               }


               void Implementation::serialtype_start( const char* const name)
               {
                  auto parent = m_stack.back();

                  auto object = json_object_new_object();

                  if( name)
                  {
                     json_object_object_add( parent, name, object);
                  }
                  else
                  {
                     //
                     // We are in a container already.
                     //
                     json_object_array_add( parent, object);
                  }

                  m_stack.push_back( object);

               }
               void Implementation::serialtype_end( const char*)
               {
                  m_stack.pop_back();
               }


               template< typename F, typename T>
               void Implementation::createAndAdd( F function, T&& value, const char* const name)
               {
                  json_object* next = (*function)( value);

                  if( name)
                  {
                     json_object_object_add( m_stack.back(), name, next);
                  }
                  else
                  {
                     //
                     // We're in a container
                     //
                     json_object_array_add( m_stack.back(), next);
                  }
               }

               void Implementation::writeValue( const bool value, const char* const name) { createAndAdd( &json_object_new_boolean, value, name);}
               void Implementation::writeValue( const char value, const char* const name) { writeValue( std::string( 1, value), name); }
               void Implementation::writeValue( const short value, const char* const name) { createAndAdd( &json_object_new_int, value, name); }
               void Implementation::writeValue( const long value, const char* const name) { createAndAdd( &json_object_new_int64, value, name); }
               void Implementation::writeValue( const long long value, const char* const name) { createAndAdd( &json_object_new_int64, value, name);}
               void Implementation::writeValue( const float value, const char* const name) { createAndAdd( &json_object_new_double, value, name);}
               void Implementation::writeValue( const double value, const char* const name) {createAndAdd( &json_object_new_double, value, name);}

               void Implementation::writeValue( const std::string& value, const char* const name)
               {
                  createAndAdd( &json_object_new_string, common::transcode::utf8::encode( value).c_str(), name);
               }

               void Implementation::writeValue( const platform::binary_type& value, const char* const name)
               {
                  createAndAdd( &json_object_new_string, common::transcode::base64::encode( value).c_str(), name);
               }

            } // writer

         } // json
      } // archive

   } // sf



} // casual
