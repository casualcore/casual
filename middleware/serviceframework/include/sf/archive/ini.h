//!
//! casual
//!

#ifndef SERVICEFRAMEWORK_INCLUDE_SF_ARCHIVE_INI_H_
#define SERVICEFRAMEWORK_INCLUDE_SF_ARCHIVE_INI_H_

#include "sf/archive/basic.h"


#include <tuple>

#include <map>
#include <vector>
#include <string>

#include <iosfwd>

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

               const tree& operator() () const noexcept;

               const tree& operator() ( std::istream& stream);
               const tree& operator() ( const std::string& ini);
               const tree& operator() ( const char* ini, std::size_t size);

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
                     if( ! value_start( name))
                     {
                        return false;
                     }

                     read( value);
                     value_end( name);

                     return true;
                  }

               private:

                  bool value_start( const char* name);
                  void value_end( const char* name);

                  void read( bool& value) const;
                  void read( short& value) const;
                  void read( long& value) const;
                  void read( long long& value) const;
                  void read( float& value) const;
                  void read( double& value) const;
                  void read( char& value) const;
                  void read( std::string& value) const;
                  void read( std::vector<char>& value) const;

               private:

                  typedef std::string data;
                  std::vector<const data*> m_data_stack;
                  std::vector<const tree*> m_node_stack;

               }; // Implementation

            } // reader


            class Save
            {

            public:

               Save();
               ~Save();

               tree& operator() () noexcept;

               void operator() ( std::ostream& ini) const;
               void operator() ( std::string& ini) const;

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
                  std::string encode( const T& value) const
                  {
                     return std::to_string( value);
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
