//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/platform.h"
#include "common/transaction/id.h"
#include "common/algorithm.h"
#include "common/transaction/id.h"

#include "common/exception/xatmi.h"

#include "common/string.h"

#include <vector>

namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace descriptor
         {
            using type = platform::descriptor::type;

            template< typename Information>
            struct basic_information : Information
            {
               using Information::Information;

               basic_information( descriptor::type descriptor, bool active)
                : active( active), descriptor( descriptor) {}

               bool active = false;
               descriptor::type descriptor;
               common::Uuid correlation;

               friend bool operator == ( descriptor::type cd, const basic_information& d) { return cd == d.descriptor;}
               friend bool operator == ( const basic_information& d, descriptor::type cd) { return cd == d.descriptor;}

               friend std::ostream& operator << ( std::ostream& out, const basic_information& value)
               {
                  return out << "{ id: " << value.descriptor
                        << ", active: " << value.active
                        << ", correlation: " << value.correlation
                        << ", " << static_cast< const Information&>( value)
                        << '}';
               }
            };

            template< typename Information>
            struct Holder
            {
               using descriptor_type = basic_information< Information>;

               //! Reserves a descriptor and associates it to message-correlation
               descriptor_type& reserve( const Uuid& correlation);

               void unreserve( descriptor::type descriptor);

               descriptor_type& get( descriptor::type descriptor);

               platform::size::type size() const { return m_descriptors.size();}
               bool empty() const { return m_descriptors.empty();}

               bool active() const
               {
                  return ! algorithm::find_if( m_descriptors, []( const auto& d){
                     return d.active;
                  }).empty();
               }

               friend std::ostream& operator << ( std::ostream& out, const Holder& value)
               {
                  return out << "{ descriptors: " << value.m_descriptors
                        << '}';
               }

            private:
               descriptor_type& reserve()
               {
                  auto found = algorithm::find_if( m_descriptors, predicate::negate( std::mem_fn( &descriptor_type::active)));

                  if( found)
                  {
                     found->active = true;
                     //found->timeout.timeout = std::chrono::microseconds{ 0};
                     return *found;
                  }
                  else
                  {
                     m_descriptors.emplace_back( m_descriptors.size() + 1, true);
                     return m_descriptors.back();
                  }
               }

               std::vector< descriptor_type> m_descriptors;
            };

            template< typename I>
            typename Holder< I>::descriptor_type& Holder< I>::reserve( const Uuid& correlation)
            {
               auto& descriptor = reserve();

               descriptor.correlation = correlation;

               return descriptor;
            }


            template< typename I>
            void Holder< I>::unreserve( descriptor::type descriptor)
            {
               auto found = algorithm::find( m_descriptors, descriptor);

               if( found)
               {
                  found->active = false;
               }
               else
               {
                  throw exception::xatmi::invalid::Descriptor{ string::compose( "invalid descriptor: ", descriptor)};
               }
            }

            template< typename I>
            typename Holder< I>::descriptor_type& Holder< I>::get( descriptor::type descriptor)
            {
               auto found = algorithm::find( m_descriptors, descriptor);
               if( found && found->active)
               {
                  return *found;
               }
               throw exception::xatmi::invalid::Descriptor{ string::compose( "invalid call descriptor: ", descriptor)};
            }



         } // descriptor

      } // service
   } // common


} // casual


