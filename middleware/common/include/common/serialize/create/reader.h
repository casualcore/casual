//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/archive.h"
#include "common/serialize/policy.h"
#include "common/exception/system.h"

#include "casual/platform.h"

#include <string>
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace create
         {
            namespace reader
            {
               struct Creator
               {
                  template< typename Implementation, template< class> class Policy>
                  static Creator construct() 
                  {
                     return Creator{ Tag{}, []( auto& destination)
                     {
                        return serialize::Reader::emplace< Policy< Implementation>>( destination);
                     }};
                  } 

                  inline serialize::Reader create( std::istream& stream) const  { return m_implementation->create( stream);}
                  inline serialize::Reader create( const platform::binary::type& data) const { return m_implementation->create( data);}

               private:

                  struct Tag{}; 

                  // using `Tag` to make sure we don't interfere with move/copy-ctor/assignment
                  template< typename C>
                  explicit Creator( Tag, C&& creator) 
                     : m_implementation{ std::make_shared< model< C>>( std::forward< C>( creator))} {}

                     
                  struct concept 
                  {
                     virtual ~concept() = default;
                     virtual serialize::Reader create( std::istream& stream) const  = 0;
                     virtual serialize::Reader create( const platform::binary::type& data) const = 0;
                  };

                  template< typename create_type> 
                  struct model : concept
                  {
                     model( create_type&& creator) : m_creator( std::move( creator)) {}

                     serialize::Reader create( std::istream& stream) const override { return m_creator( stream);}
                     serialize::Reader create( const platform::binary::type& data) const override { return m_creator( data);}

                  private:
                     create_type m_creator;
                  };

                  std::shared_ptr< const concept> m_implementation;
               };

               static_assert( std::is_copy_constructible< Creator>::value, "");
               static_assert( std::is_copy_assignable< Creator>::value, "");

            } // reader
         } // create
      } // serialize
   } // common
} // casual
