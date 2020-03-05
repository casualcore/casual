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
            namespace writer
            {
               namespace detail
               {
                  template< typename Implementation>
                  auto create()
                  {
                     return serialize::Writer::emplace< Implementation>();
                  }

               } // detail

               struct Creator
               {
                  template< typename Implementation>
                  static Creator construct() 
                  {
                     return Creator{ Tag{}, []()
                     {
                        return writer::detail::create< Implementation>();
                     }};
                  } 

                  inline serialize::Writer create() const { return m_creator();}

               private:
                  
                  struct Tag{};

                  // using `Tag` to make sure we don't interfere with move/copy-ctor/assignment
                  template< typename C>
                  explicit Creator( Tag, C&& creator) 
                     : m_creator{ std::forward< C>( creator)} {}

                  common::function< serialize::Writer() const> m_creator;
               };

               static_assert( std::is_copy_constructible< Creator>::value, "");
               static_assert( std::is_copy_assignable< Creator>::value, "");

            } // writer
         } // create
      } // serialize
   } // common
} // casual
