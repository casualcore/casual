//!
//! service.h
//!
//! Created on: Mar 14, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_SF_ARCHIVE_SERVICE_H_
#define CASUAL_SF_ARCHIVE_SERVICE_H_

#include "sf/service/model.h"

#include "sf/platform.h"
#include "sf/archive/basic.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace service
         {
            namespace implementation
            {
               class Writer
               {
               public:

                  using types_t = std::vector< sf::service::Model::Type>;

                  Writer( std::vector< sf::service::Model::Type>& types);
                  Writer( Writer&&);


                  ~Writer();

                  std::size_t container_start( const std::size_t size, const char* name);
                  void container_end( const char*);

                  void serialtype_start( const char* name);
                  void serialtype_end( const char*);

                  template<typename T>
                  void write( const T& value, const char* name)
                  {
                     m_stack.back()->emplace_back( name, type_traits< T>::value);
                  }


               private:

                  template< typename T>
                  struct type_traits;



                  std::vector< types_t*> m_stack;
               };

               template<> struct Writer::type_traits< std::string> { enum { value = sf::service::Model::Type::type_string};};
               template<> struct Writer::type_traits< platform::binary_type> { enum { value = sf::service::Model::Type::type_binary};};


               template<> struct Writer::type_traits< char> { enum { value = sf::service::Model::Type::type_char};};
               template<> struct Writer::type_traits< bool> { enum { value = sf::service::Model::Type::type_boolean};};

               template<> struct Writer::type_traits< long> { enum { value = sf::service::Model::Type::type_integer};};
               template<> struct Writer::type_traits< int> { enum { value = sf::service::Model::Type::type_integer};};
               template<> struct Writer::type_traits< short> { enum { value = sf::service::Model::Type::type_integer};};
               template<> struct Writer::type_traits< long long> { enum { value = sf::service::Model::Type::type_integer};};

               template<> struct Writer::type_traits< float> { enum { value = sf::service::Model::Type::type_float};};
               template<> struct Writer::type_traits< double> { enum { value = sf::service::Model::Type::type_float};};


               struct Prepare
               {

                  bool serialtype_start( const char*) { return true;}

                  std::tuple< std::size_t, bool> container_start( std::size_t size, const char*)
                  {
                     if( size == 0)
                     {
                        return std::make_tuple( 1, true);
                     }

                     return std::make_tuple( size, true);
                  }

                  //! @{
                  //! No op
                  void container_end( const char*) { /*no op*/}
                  void serialtype_end( const char*) { /*no op*/}
                  //! @}


                  template< typename T>
                  bool read( T& value, const char*)
                  {
                     return true;
                  }
               };

            } // implementation


            struct Writer
            {
               using writer_type = basic_writer< implementation::Writer>;
               using prepare_type = basic_reader< implementation::Prepare, policy::Relaxed>;

               Writer( std::vector< sf::service::Model::Type>& types) : m_writer( types) {}

               template< typename T>
               Writer& operator << ( T&& value)
               {
                  prepare_type prepare;
                  prepare >> value;

                  m_writer << std::forward< T>( value);

                  return *this;
               }

               template< typename T>
               Writer& operator & ( T&& value)
               {
                  return *this << std::forward< T>( value);
               }

            private:

               writer_type m_writer;
            };


         } // service
      } // archive
   } // sf
} // casual

#endif // SERVICE_H_
