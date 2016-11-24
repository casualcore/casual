//!
//!
//!

#include "sf/archive/service.h"
#include "sf/log.h"

#include <random>

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
                  Writer::Writer( std::vector< sf::service::Model::Type>& types) : m_stack{ &types}
                  {

                  }

                  Writer::Writer( Writer&&) = default;


                  Writer::~Writer() = default;

                  std::size_t Writer::container_start( const std::size_t size, const char* name)
                  {
                    auto& current = *m_stack.back();

                    current.emplace_back( name, sf::service::model::type::Category::container);

                    m_stack.push_back( &current.back().attribues);

                     return 1;
                  }

                  void Writer::container_end( const char*)
                  {
                     m_stack.pop_back();
                  }


                  void Writer::serialtype_start( const char* name)
                  {
                     auto& current = *m_stack.back();

                     current.emplace_back( name, sf::service::model::type::Category::composite);

                     m_stack.push_back( &current.back().attribues);

                  }

                  void Writer::serialtype_end( const char*)
                  {
                     m_stack.pop_back();
                  }


                  bool Prepare::serialtype_start( const char*) { return true;}

                  std::tuple< std::size_t, bool> Prepare::container_start( std::size_t size, const char*)
                  {
                     if( size == 0)
                     {
                        return std::make_tuple( 1, true);
                     }

                     return std::make_tuple( size, true);
                  }

                  void Prepare::container_end( const char*) { /*no op*/}
                  void Prepare::serialtype_end( const char*) { /*no op*/}


               } // implementation

            } // describe

         } // service
      } // archive
   } // sf
} // casual
