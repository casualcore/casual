//!
//! casual
//!

#ifndef CASUAL_SF_ARCHIVE_XML_H_
#define CASUAL_SF_ARCHIVE_XML_H_

#include "sf/archive/basic.h"

#include <pugixml.hpp>


#include <iosfwd>
#include <string>
#include <vector>
#include <utility>
#include <tuple>


namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace xml
         {
            //
            // This implementation uses pugixml 1.2
            //
            // There are some flaws in this implementation and we're waiting
            // for libpugixml-dev with 1.4 so they can be fixed
            //
            // The 1.4/1.5 do offer a slight different API, so we need to adapt
            // to it to fix the flaws
            //


            class Load
            {
            public:

               typedef pugi::xml_document source_type;

               Load();
               ~Load();

               const pugi::xml_document& operator() () const noexcept;

               const pugi::xml_document& operator() ( std::istream& stream);
               const pugi::xml_document& operator() ( const std::string& xml);
               const pugi::xml_document& operator() ( const char* xml, std::size_t size);
               const pugi::xml_document& operator() ( const char* xml);

            private:

               pugi::xml_document m_document;

            };


            namespace reader
            {

               class Implementation
               {
               public:

                  //!
                  //! @param node Normally a pugi::xml_document
                  //!
                  //! @note Any possible document has to outlive the reader
                  //!
                  explicit Implementation( pugi::xml_node node );

                  std::tuple< std::size_t, bool> container_start( std::size_t size, const char* name);

                  void container_end( const char* name);

                  bool serialtype_start( const char* name);

                  void serialtype_end( const char* name);


                  template< typename T>
                  bool read( T& value, const char* const name)
                  {
                     if( ! start( name))
                        return false;

                     read( value);
                     end( name);

                     return true;
                  }

               private:

                  bool start( const char* name);
                  void end( const char* name);

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

                  // 'vector' instead of 'stack' to use some algorithms
                  std::vector<pugi::xml_node> m_stack;

               }; // Implementation

            } // reader


            class Save
            {

            public:

               typedef pugi::xml_document target_type;

               Save();
               ~Save();

               pugi::xml_document& operator() () noexcept;

               void operator() ( std::ostream& xml) const;
               void operator() ( std::string& xml) const;


            private:

               pugi::xml_document m_document;

            };


            namespace writer
            {

               class Implementation
               {

               public:

                  //!
                  //! @param node Normally a pugi::xml_document
                  //!
                  //! @note Any possible document has to outlive the reader
                  //!
                  explicit Implementation( pugi::xml_node node);

                  std::size_t container_start( const std::size_t size, const char* name);

                  void container_end( const char* name);

                  void serialtype_start( const char* name);

                  void serialtype_end( const char* name);

                  template< typename T>
                  void write( const T& value, const char* const name)
                  {
                     start( name);
                     write( value);
                     end( name);
                  }

               private:

                  void start( const char* name);
                  void end( const char* name);

                  template<typename T>
                  void write( const T& value)
                  {
                     m_stack.back().text().set( std::to_string( value).c_str());
                  }

                  //
                  // A few overloads
                  //

                  void write( const bool& value);
                  void write( const char& value);
                  void write( const std::string& value);
                  void write( const std::vector<char>& value);

               private:

                  std::vector<pugi::xml_node> m_stack;

               }; // Implementation

            } // writer

            namespace relaxed
            {
               typedef basic_reader< reader::Implementation, policy::Relaxed> Reader;
            }

            typedef basic_reader< reader::Implementation, policy::Strict > Reader;

            typedef basic_writer< writer::Implementation> Writer;


         } // xml
      } // archive
   } // sf
} // casual



#endif /* CASUAL_SF_ARCHIVE_XML_H_ */
