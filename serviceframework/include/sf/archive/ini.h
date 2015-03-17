/*
 * ini.h
 *
 *  Created on: Feb 11, 2015
 *      Author: kristone
 */

#ifndef SERVICEFRAMEWORK_INCLUDE_SF_ARCHIVE_INI_H_
#define SERVICEFRAMEWORK_INCLUDE_SF_ARCHIVE_INI_H_

#include "sf/archive/basic.h"


#include <sstream>
#include <tuple>

#include <map>
#include <vector>
#include <string>

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace ini
         {

            struct tree
            {
               std::multimap<std::string,tree> children;
               std::multimap<std::string,std::string> values;
            };


            class Load
            {
            public:

               typedef tree source_type;

               Load();
               ~Load();

               const tree& serialize( std::istream& stream);
               const tree& serialize( const std::string& ini);

               const tree& source() const;


               template<typename T>
               const tree& operator() ( T&& ini)
               {
                  return serialize( std::forward<T>( ini));
               }

            private:

               tree m_document;

            };


            namespace reader
            {

               class Implementation
               {
               public:

                  explicit Implementation( const tree& document);

                  std::tuple< std::size_t, bool> container_start( std::size_t size, const char* name);
                  void container_end( const char* name);

                  bool serialtype_start( const char* name);
                  void serialtype_end( const char* name);


                  template< typename T>
                  bool read( T& value, const char* const name)
                  {
                     if( value_start( name))
                     {
                        decode( *m_data_stack.back(), value);
                     }
                     else
                     {
                        return false;
                     }

                     value_end( name);

                     return true;
                  }

               private:

                  bool value_start( const char* name);
                  void value_end( const char* name);

                  //
                  // TODO: Add some error handling
                  //

                  template<typename T>
                  void decode( const std::string& data, T& value) const
                  {
                     std::istringstream stream( data);
                     stream >> value;
                  }

                  //
                  // A few overloads
                  //

                  void decode( const std::string& data, bool& value) const;
                  void decode( const std::string& data, char& value) const;
                  void decode( const std::string& data, std::string& value) const;
                  void decode( const std::string& data, std::vector<char>& value) const;

               private:

                  typedef std::string data;
                  std::vector<const data*> m_data_stack;
                  std::vector<const tree*> m_node_stack;

               }; // Implementation

            } // reader


            class Save
            {

            public:

               typedef tree target_type;

               Save();
               ~Save();

               void serialize( std::ostream& stream) const;
               void serialize( std::string& ini) const;

               tree& target();

               tree& operator() ()
               {
                  return target();
               }

            private:

               tree m_document;

            };


            namespace writer
            {

               class Implementation
               {

               public:

                  explicit Implementation( tree& document);

                  std::size_t container_start( const std::size_t size, const char* name);

                  void container_end( const char* name);

                  void serialtype_start( const char* name);

                  void serialtype_end( const char* name);

                  template< typename T>
                  void write( const T& data, const char* const name)
                  {
                     write( encode( data), name);
                  }

                  void write( std::string data, const char* name);

               private:


                  template<typename T>
                  //typename std::enabe_if<std::is_floating_point<T>::value, void>::type
                  std::string encode( const T& value) const
                  {
                     std::ostringstream stream;
                     stream << std::fixed << value;
                     return stream.str();
                  }

                  //
                  // A few overloads
                  //

                  std::string encode( const bool& value) const;
                  std::string encode( const char& value) const;
                  std::string encode( const std::vector<char>& value) const;

               private:

                  typedef const char name;
                  std::vector<name*> m_name_stack; // for containers
                  std::vector<tree*> m_node_stack;

               }; // Implementation

            } // writer

            namespace relaxed
            {
               typedef basic_reader< reader::Implementation, policy::Relaxed> Reader;
            }

            typedef basic_reader< reader::Implementation, policy::Strict > Reader;

            typedef basic_writer< writer::Implementation> Writer;

         } // ini

      } // archive

   } // sf

} // casual




#endif /* SERVICEFRAMEWORK_INCLUDE_SF_ARCHIVE_INI_H_ */
