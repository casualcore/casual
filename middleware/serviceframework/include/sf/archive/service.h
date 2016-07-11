//!
//! casual
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
            namespace describe
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
                     void write( T&& value, const char* name)
                     {
                        m_stack.back()->emplace_back( name, sf::service::model::type::traits< T>::category());
                     }

                  private:
                     std::vector< types_t*> m_stack;
                  };



                  struct Prepare
                  {

                     bool serialtype_start( const char*);

                     std::tuple< std::size_t, bool> container_start( std::size_t size, const char*);

                     //! @{
                     //! No op
                     void container_end( const char*);
                     void serialtype_end( const char*);
                     //! @}


                     template< typename T>
                     bool read( T& value, const char*)
                     {
                        return true;
                     }
                  };

               } // implementation


               using Prepare = basic_reader< implementation::Prepare, policy::Relaxed>;

               using Writer = basic_writer< implementation::Writer>;



               //!
               //! To help with unittesting and such.
               //!
               struct Wrapper
               {
                  Wrapper( std::vector< sf::service::Model::Type>& types) : m_writer( types) {}

                  template< typename T>
                  Wrapper& operator << ( T&& value)
                  {
                     Prepare prepare;
                     prepare >> value;

                     m_writer << std::forward< T>( value);

                     return *this;
                  }

                  template< typename T>
                  Wrapper& operator & ( T&& value)
                  {
                     return *this << std::forward< T>( value);
                  }
               private:
                  Writer m_writer;
               };

            } // describe


            namespace example
            {
               namespace implementation
               {
                  //!
                  //! "prepares" the object model with "random" values
                  //!
                  struct Prepare
                  {

                     bool serialtype_start( const char*);

                     std::tuple< std::size_t, bool> container_start( std::size_t size, const char*);

                     //! @{
                     //! No op
                     void container_end( const char*);
                     void serialtype_end( const char*);
                     //! @}


                     template< typename T>
                     bool read( T& value, const char*)
                     {
                        pod( value);
                        return true;
                     }

                     void pod( bool& value);
                     void pod( char& value);
                     void pod( short& value);
                     void pod( long& value);
                     void pod( long long& value);
                     void pod( float& value);
                     void pod( double& value);
                     void pod( std::string& value);
                     void pod( platform::binary_type& value);
                  };

               } // implementation

               using Prepare = basic_reader< implementation::Prepare, policy::Relaxed>;

            } // example

         } // service
      } // archive
   } // sf
} // casual

#endif // SERVICE_H_
