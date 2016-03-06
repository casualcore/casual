//!
//! casual_archive_yaml_policy.h
//!
//! Created on: Oct 31, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_ARCHIVE_YAML_POLICY_H_
#define CASUAL_ARCHIVE_YAML_POLICY_H_

#include "sf/archive/basic.h"
//#include "sf/reader_policy.h"

#include <yaml-cpp/yaml.h>
#include <yaml-cpp/binary.h>

#include <iosfwd>
#include <stack>

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace yaml
         {

            class Load
            {
            public:

               typedef YAML::Node source_type;

               Load();
               ~Load();

               const YAML::Node& serialize( std::istream& stream);
               const YAML::Node& serialize( const std::string& yaml);
               // TODO: make this a binary::Stream instead
               const YAML::Node& serialize( const char* yaml);

               const YAML::Node& source() const;

               template<typename T>
               const YAML::Node& operator() ( T&& yaml)
               {
                  return serialize( std::forward<T>( yaml));
               }


            private:

               YAML::Node m_document;

            };


            namespace reader
            {

               class Implementation
               {
               public:

                  Implementation( const YAML::Node& node);

                  std::tuple< std::size_t, bool> container_start( std::size_t size, const char* name);

                  void container_end( const char* name);

                  bool serialtype_start( const char* name);

                  void serialtype_end( const char* name);

                  template< typename T>
                  bool read( T& value, const char* const name)
                  {
                     const bool result = start( name);

                     if( result)
                     {
                        if( m_stack.back()->Type() == YAML::NodeType::Null)
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
                     }

                     end( name);

                     return result;
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
                  void read( platform::binary_type& value) const;

               private:

                  std::vector< const YAML::Node*> m_stack;

               };

            } // reader


            class Save
            {
            public:

               typedef YAML::Emitter target_type;

               Save();
               ~Save();

               void serialize( std::ostream& stream) const;
               void serialize( std::string& yaml) const;
               // TODO: make a binary::Stream overload

               YAML::Emitter& target();

               YAML::Emitter& operator() ()
               {
                  return target();
               }


            private:

               YAML::Emitter m_emitter;

            };



            namespace writer
            {

               class Implementation
               {

               public:

                  typedef YAML::Emitter buffer_type;

                  Implementation( YAML::Emitter& output);

                  std::size_t container_start( std::size_t size, const char* name);
                  void container_end( const char* name);

                  void serialtype_start( const char* name);
                  void serialtype_end( const char* name);

                  template< typename T>
                  void write( const T& value, const char* name)
                  {
                     if( name)
                     {
                        m_output << YAML::Key << name;
                        m_output << YAML::Value;
                     }
                     writeValue( value);
                  }

               private:

                  //
                  // TODO: Make sure strings are UTF-8-encoded ?
                  //

                  template< typename T>
                  void writeValue( const T& value)
                  {
                     m_output << value;
                  }

                  void writeValue( const platform::binary_type& value)
                  {
                     // TODO: Is this conformant ?
                     const unsigned char* data = reinterpret_cast< const unsigned char*>( value.data());
                     YAML::Binary binary( data, value.size());
                     m_output << binary;
                  }

               private:

                  YAML::Emitter& m_output;
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

         } // yaml
      } // archive
   } // sf
} // casual



#endif /* CASUAL_ARCHIVE_YAML_POLICY_H_ */
