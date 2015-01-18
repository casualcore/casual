//!
//! archive_jsaon.h
//!
//! Created on: Jul 10, 2013
//!     Author: Lazan
//!

#ifndef ARCHIVE_JSON_H_
#define ARCHIVE_JSON_H_


#include "sf/reader_policy.h"
#include "sf/archive/basic.h"
#include "sf/platform.h"

#include "json-c/json.h"


#include <iostream>
#include <fstream>
#include <iterator>
#include <string>

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace json
         {
            namespace reader
            {

               struct Buffer
               {
                  Buffer( const std::string& file)
                  {
                     std::ifstream in( file);
                     m_string.assign( std::istream_iterator< char>( in), std::istream_iterator< char>());
                  }

                  const char* archiveBuffer()
                  {
                     return m_string.c_str();
                  }

               private:
                  std::string m_string;
               };

               class Implementation
               {
               public:

                  enum class State
                  {
                     unknown,
                     container,
                     serializable,
                     missing,
                  };

                  Implementation( const char* buffer);
                  ~Implementation();

                  std::tuple< std::size_t, bool> container_start( std::size_t size, const char* name);
                  void container_end( const char*);

                  bool serialtype_start( const char* name);
                  void serialtype_end( const char*);


                  template< typename T>
                  bool read( T& value, const char* name)
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
                           //
                           // "unnamed", indicate we're in a container
                           // we pop
                           //
                           m_stack.pop_back();
                        }
                     }

                     if( ! node)
                     {
                        return false;
                     }

                     set( node, value);
                     return true;
                  }


               private:


                  void set( json_object* object, bool& value);
                  void set( json_object* object, short& value);
                  void set( json_object* object, long& value);
                  void set( json_object* object, long long& value);
                  void set( json_object* object, float& value);
                  void set( json_object* object, double& value);
                  void set( json_object* object, std::string& value);
                  void set( json_object* object, char& value);
                  void set( json_object* object, platform::binary_type& value);


                  json_object* m_root;
                  std::vector< json_object*> m_stack;
               };
            } // reader

            namespace writer
            {
               class Implementation
               {
               public:

                  Implementation( json_object*& root);
                  ~Implementation();

                  std::size_t container_start( std::size_t size, const char* name);
                  void container_end( const char*);

                  void serialtype_start( const char* name);
                  void serialtype_end( const char*);

                  template< typename T>
                  void write( T&& value, const char* name)
                  {
                     writeValue( value, name);
                  }


               private:

                  template< typename F, typename T>
                  void createAndAdd( F function, T&& value, const char* name);

                  void writeValue( const bool value, const char* name);
                  void writeValue( const char value, const char* name);
                  void writeValue( const short value, const char* name);
                  void writeValue( const long value, const char* name);
                  void writeValue( const long long value, const char* name);
                  void writeValue( const float value, const char* name);
                  void writeValue( const double value, const char* name);
                  void writeValue( const std::string& value, const char* name);
                  void writeValue( const platform::binary_type& value, const char* name);


                  json_object*& m_root;
                  std::vector< json_object*> m_stack;
               };

            } // writer

            template< typename P>
            using basic_reader = archive::basic_reader< reader::Implementation, P>;


            using Reader = basic_reader< policy::Strict>;

            namespace relaxed
            {
               using Reader = basic_reader< policy::Relaxed>;
            }


            typedef basic_writer< writer::Implementation> Writer;

         } // json
      } // archive
   } // sf
} // casual




#endif /* ARCHIVE_JSAON_H_ */
