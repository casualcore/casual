//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "sf/archive/service.h"

#include "sf/platform.h"


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
               namespace local
               {
                  namespace
                  {
                     namespace implementation
                     {
                        class Writer
                        {
                        public:

                           using types_t = std::vector< sf::service::Model::Type>;

                           Writer( types_t& types) : m_stack{ &types}
                           {
                           }

                           platform::size::type container_start( const platform::size::type size, const char* name)
                           {
                              auto& current = *m_stack.back();

                              current.emplace_back( name, sf::service::model::type::Category::container);

                              m_stack.push_back( &current.back().attribues);

                                 return 1;
                           }

                           void container_end( const char*)
                           {
                              m_stack.pop_back();
                           }

                           
                           void serialtype_start( const char* name)
                           {
                              auto& current = *m_stack.back();

                              current.emplace_back( name, sf::service::model::type::Category::composite);

                              m_stack.push_back( &current.back().attribues);
                           }

                           void serialtype_end( const char*)
                           {
                              m_stack.pop_back();
                           }

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
                           bool serialtype_start( const char*) { return true;}

                           std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char*)
                           {
                              if( size == 0)
                              {
                                 return std::make_tuple( 1, true);
                              }

                              return std::make_tuple( size, true);
                           }

                           void container_end( const char*) { /*no op*/}
                           void serialtype_end( const char*) { /*no op*/}

                           template< typename T>
                           bool read( T& value, const char*)
                           {
                              return true;
                           }
                        };

                     } // implementation

                  } // <unnamed>
               } // local

               
               archive::Reader prepare() { return archive::Reader::emplace< local::implementation::Prepare>();}
               
               archive::Writer writer( std::vector< sf::service::Model::Type>& types)
               {
                  return archive::Writer::emplace< local::implementation::Writer>( types);
               }

            } // describe
         } // service
      } // archive
   } // sf
} // casual
