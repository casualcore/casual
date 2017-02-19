//!
//! casual
//!

#ifndef ARCHIVE_JSON_H_
#define ARCHIVE_JSON_H_


#include "sf/reader_policy.h"
#include "sf/archive/basic.h"
#include "sf/platform.h"

// TODO: Move this to makefile
#define RAPIDJSON_HAS_STDSTRING 1

#include <rapidjson/document.h>


#include <iosfwd>
#include <string>

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace json
         {

            class Load
            {

            public:

               typedef rapidjson::Document source_type;

               Load();
               ~Load();

               const rapidjson::Document& operator() () const noexcept;

               const rapidjson::Document& operator() ( std::istream& stream);
               const rapidjson::Document& operator() ( const std::string& json);
               const rapidjson::Document& operator() ( const char* json, std::size_t size);
               const rapidjson::Document& operator() ( const char* json);


            private:

               rapidjson::Document m_document;

            };


            namespace reader
            {

               class Implementation
               {
               public:

                  explicit Implementation( const rapidjson::Value& object);
                  ~Implementation();

                  std::tuple< std::size_t, bool> container_start( std::size_t size, const char* name);
                  void container_end( const char* name);

                  bool serialtype_start( const char* name);
                  void serialtype_end( const char* name);


                  template< typename T>
                  bool read( T& value, const char* name)
                  {
                     if( start( name))
                     {
                        if( m_stack.back()->IsNull())
                        {
                           //
                           // Act (somehow) relaxed
                           //

                           value = T{};
                        }
                        else
                        {
                           read( value);
                        }
                        end( name);

                        return true;
                     }

                     return false;
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
                  void read( std::string& value) const;
                  void read( char& value) const;
                  void read( platform::binary::type& value) const;

               private:

                  std::vector<const rapidjson::Value*> m_stack;
               };
            } // reader

            class Save
            {

            public:

               Save();
               ~Save();

               rapidjson::Document& operator() () noexcept;

               void operator() ( std::ostream& json) const;
               void operator() ( std::string& json) const;


            private:

               rapidjson::Document m_document;

            };


            namespace writer
            {
               class Implementation
               {
               public:

                  explicit Implementation( rapidjson::Document& document);
                  Implementation( rapidjson::Value& object, rapidjson::Document::AllocatorType& allocator);
                  ~Implementation();

                  std::size_t container_start( std::size_t size, const char* name);
                  void container_end( const char* name);

                  void serialtype_start( const char* name);
                  void serialtype_end( const char* name);

                  template< typename T>
                  void write( const T& value, const char* name)
                  {
                     start( name);
                     write( value);
                     end( name);
                  }


               private:

                  void start( const char* name);
                  void end( const char* name);

                  void write( const bool value);
                  void write( const char value);
                  void write( const short value);
                  void write( const long value);
                  void write( const long long value);
                  void write( const float value);
                  void write( const double value);
                  void write( const std::string& value);
                  void write( const platform::binary::type& value);


               private:

                  rapidjson::Document::AllocatorType& m_allocator;
                  std::vector< rapidjson::Value*> m_stack;
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
