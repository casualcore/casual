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

            namespace example
            {
               namespace implementation
               {
                  namespace local
                  {
                     namespace
                     {
                        namespace random
                        {
                           std::default_random_engine& engine()
                           {
                              static std::random_device device;
                              static std::default_random_engine engine{ device()};
                              return engine;
                           }

                           template< typename T>
                           T random()
                           {
                              static std::uniform_int_distribution< T> distribution(
                                    std::numeric_limits< T>::min() / 5,
                                    std::numeric_limits< T>::max() / 5);

                              return distribution( engine());
                           }

                           template<>
                           char random<char>()
                           {
                              static std::uniform_int_distribution< char> distribution(
                                    40,
                                    127);

                              return distribution( engine());
                           }

                        } // random
                     } // <unnamed>
                  } // local

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




                  void Prepare::pod( bool& value) { value = true;}
                  void Prepare::pod( char& value) { value = local::random::random< char>();}
                  void Prepare::pod( short& value) { value = local::random::random< short>();}
                  void Prepare::pod( long& value) { value = local::random::random< long>();}
                  void Prepare::pod( long long& value) { value = local::random::random< long long>();}
                  void Prepare::pod( float& value) { value = local::random::random<short>();}
                  void Prepare::pod( double& value) { value = local::random::random<long>();}
                  void Prepare::pod( std::string& value) { value = "casual";}
                  void Prepare::pod( platform::binary_type& value)
                  {
                     if( value.empty())
                     {
                        auto uuid = common::uuid::make();

                        value.assign( std::begin( uuid.get()), std::end( uuid.get()));
                     }
                  }

               } // implementation

            } // example

         } // service
      } // archive
   } // sf
} // casual
