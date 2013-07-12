//!
//! archive_jsaon.h
//!
//! Created on: Jul 10, 2013
//!     Author: Lazan
//!

#ifndef ARCHIVE_JSON_H_
#define ARCHIVE_JSON_H_


#include "sf/reader_policy.h"
#include "sf/basic_archive.h"

#include "json-c/json.h"


#include <iostream>

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


               template< typename P>
               class Implementation
               {
               public:

                  typedef P policy_type;


                  Implementation( const char* buffer) : m_root( json_tokener_parse( buffer))
                  {

                     if( m_root)
                     {
                        m_stack.push( m_root);
                     }
                     else
                     {
                        //m_policy.initalization();
                     }
                     m_state.push( cRoot);
                  }

                  ~Implementation()
                  {
                     //
                     // "free" the crap
                     //
                     json_object_put( m_root);
                  }

                  void handle_start( const char* name)
                  {
                     m_currentRole = name;
                  }

                  void handle_end( const char* name) {  /*no op*/}

                  std::size_t handle_container_start( std::size_t size)
                  {

                     json_object* next = json_object_object_get( m_stack.top(), m_currentRole);

                     if( json_object_is_type( next, json_type_array))
                     {
                        size = json_object_array_length( next);

                        //
                        // push all items to the stack, in reverse order
                        //
                        for( int index = size; index > 0; --index)
                        {
                           m_stack.push( json_object_array_get_idx( next, index - 1));
                        }
                     }
                     else
                     {
                        size = m_policy.container( m_currentRole);
                     }

                     m_state.push( cContainer);

                     return size;
                  }

                  void handle_container_end()
                  {
                     m_state.pop();
                  }

                  void handle_serialtype_start()
                  {
                     if( m_state.top() != cContainer)
                     {
                        json_object* next = json_object_object_get( m_stack.top(), m_currentRole);

                        m_stack.push( next);

                        if( ! next)
                        {
                           m_policy.serialtype( m_currentRole);
                        }
                     }
                     m_state.push( cComposite);
                  }

                  void handle_serialtype_end()
                  {
                     m_stack.pop();
                     m_state.pop();
                  }


                  template< typename T>
                  void read( T& value)
                  {
                     if( json_object_is_type(  m_stack.top(), json_type_object))
                     {
                        //
                        // It's composite, get value from it...
                        //
                        json_object* next = json_object_object_get( m_stack.top(), m_currentRole);

                        readValue( next, value);
                     }
                     else
                     {
                        //
                        // It's a value, get it...
                        // This can only happen if there is an array with 'pods'
                        //
                        readValue( m_stack.top(), value);

                        m_stack.pop();
                     }
                  }


               private:


                  void set( json_object* object, bool& value) { value = json_object_get_boolean( object); }

                  void set( json_object* object, short& value) { value = json_object_get_int( object); }
                  void set( json_object* object, long& value) { value = json_object_get_int64( object); }
                  void set( json_object* object, long long& value) { value = json_object_get_int64( object); }
                  void set( json_object* object, float& value) { value = json_object_get_double( object); }
                  void set( json_object* object, double& value) { value = json_object_get_int64( object); }
                  void set( json_object* object, std::string& value) { value = json_object_get_string( object); }

                  void set( json_object* object, char& value)
                  {
                     const char* string = json_object_get_string( object);
                     if( string)
                     {
                        value = string[ 0];
                     }
                  }

                  void set( json_object* object, common::binary_type& value) { /* has to be base64 */ }


                  template< typename T>
                  void readValue( json_object* object, T& value)
                  {
                     if( object)
                     {
                        set( object, value);
                     }
                     else
                     {
                        m_policy.value( m_currentRole, value);
                     }

                  }

                  enum State
                  {
                     cRoot,
                     cComposite,
                     cContainer,
                     cElement
                  };

                  policy_type m_policy;
                  json_object* m_root;
                  std::stack< json_object*> m_stack;
                  std::stack< State> m_state;
                  const char* m_currentRole = nullptr;

               };
            } // reader

            namespace writer
            {
               class Implementation
               {
               public:

                  Implementation( json_object*& root) : m_root( root)
                  {
                     if( ! m_root)
                     {
                        m_root = json_object_new_object();
                     }
                     m_stack.push( m_root);
                  }

                  ~Implementation()
                  {
                     json_object_put( m_root);
                  }

                  void handle_start( const char* name)
                  {
                     m_currentRole = name;
                  }
                  void handle_end( const char* name) {}

                  std::size_t handle_container_start( std::size_t size)
                  {
                     createAndAdd( &json_object_new_array);
                     return size;
                  }

                  void handle_container_end()
                  {
                     m_stack.pop();
                  }


                  void handle_serialtype_start()
                  {
                     createAndAdd( &json_object_new_object);
                  }
                  void handle_serialtype_end()
                  {
                     m_stack.pop();
                  }

                  template< typename T>
                  void write( T&& value)
                  {
                     writeValue( value);
                  }


               private:

                  template< typename F>
                  void createAndAdd( F function)
                  {
                     json_object* next = (*function)();

                     if( json_object_get_type( m_stack.top()) == json_type_array)
                     {
                        json_object_array_add( m_stack.top(), next);
                     }
                     else
                     {
                        json_object_object_add( m_stack.top(), m_currentRole, next);
                     }
                     m_stack.push( next);
                  }

                  template< typename F, typename T>
                  void createAndAdd( F function, T&& value)
                  {
                     json_object* next = (*function)( value);

                     if( json_object_get_type( m_stack.top()) == json_type_array)
                     {
                        json_object_array_add( m_stack.top(), next);
                     }
                     else
                     {
                        json_object_object_add( m_stack.top(), m_currentRole, next);
                     }
                  }

                  void writeValue( const bool value) { createAndAdd( &json_object_new_boolean, value);}
                  void writeValue( const char value) { writeValue( std::string{ value});}
                  void writeValue( const short value) { createAndAdd( &json_object_new_int, value); }
                  void writeValue( const long value) { createAndAdd( &json_object_new_int64, value); }
                  void writeValue( const long long value) { createAndAdd( &json_object_new_int64, value);}
                  void writeValue( const float value) { createAndAdd( &json_object_new_double, value);}
                  void writeValue( const double value) {createAndAdd( &json_object_new_double, value);}
                  void writeValue( const std::string& value) { createAndAdd( &json_object_new_string, value.c_str());}
                  void writeValue( const common::binary_type& value) { /* no op - need base64 */ }


                  json_object*& m_root;
                  std::stack< json_object*> m_stack;
                  const char* m_currentRole = nullptr;


               };

            } // writer

            typedef basic_reader< reader::Implementation< policy::reader::Strict> > Reader;

            namespace relaxed
            {
               typedef basic_reader< reader::Implementation< policy::reader::Relaxed> > Reader;
            }


            typedef basic_writer< writer::Implementation> Writer;

         } // json
      } // archive
   } // sf
} // casual




#endif /* ARCHIVE_JSAON_H_ */
