//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "serviceframework/service/protocol/describe.h"

#include "casual/platform.h"

namespace casual
{
   namespace serviceframework::service::protocol::describe
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
                  constexpr static auto archive_properties() { return common::serialize::archive::Property::named | common::serialize::archive::Property::no_consume;}
                  
                  using types_t = std::vector< serviceframework::service::Model::Type>;

                  Writer( types_t& types) : m_stack{ &types} {}

                  platform::size::type container_start( platform::size::type size, const char* name)
                  {
                     auto& current = *m_stack.back();

                     current.emplace_back( name, serviceframework::service::model::type::Category::container);

                     m_stack.push_back( &current.back().attributes);

                     return 1;
                  }

                  void container_end( const char*)
                  {
                     m_stack.pop_back();
                  }
                  
                  void composite_start( const char* name)
                  {
                     auto& current = *m_stack.back();

                     current.emplace_back( name, serviceframework::service::model::type::Category::composite);

                     m_stack.push_back( &current.back().attributes);
                  }

                  void composite_end( const char*)
                  {
                     m_stack.pop_back();
                  }

                  template<typename T>
                  void write( T&& value, const char* name)
                  {
                     m_stack.back()->emplace_back( name, serviceframework::service::model::type::traits< T>::category());
                  }

               private:
                  std::vector< types_t*> m_stack;
               };



               struct Prepare
               {
                  constexpr static auto archive_properties() { return common::serialize::archive::Property::order;}
                  bool composite_start( const char*) { return true;}

                  std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char*)
                  {
                     if( size == 0)
                        return std::make_tuple( 1, true);

                     return std::make_tuple( size, true);
                  }

                  void container_end( const char*) { /*no op*/}
                  void composite_end( const char*) { /*no op*/}

                  template< typename T>
                  bool read( T& value, const char*)
                  {
                     return true;
                  }
               };

            } // implementation

         } // <unnamed>
      } // local

      
      common::serialize::Reader prepare() { return common::serialize::Reader::emplace< local::implementation::Prepare>();}
      
      common::serialize::Writer writer( std::vector< serviceframework::service::Model::Type>& types)
      {
         return common::serialize::Writer::emplace< local::implementation::Writer>( types);
      }

   } // serviceframework::service::protocol::describe
} // casual
