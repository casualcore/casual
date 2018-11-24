//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/platform.h"
#include "common/buffer/type.h"

#include <unordered_map>
#include <string>
#include <iosfwd>


namespace casual
{
   namespace buffer
   {
      namespace field
      {
         namespace internal
         {
            //!
            //! Repository
            //!
            //! @{
            const std::unordered_map<std::string,int>& name_to_type();
            const std::unordered_map<int,std::string>& type_to_name();

            const std::unordered_map<std::string,long>& name_to_id();
            const std::unordered_map<long,std::string>& id_to_name();
            //! @}


            //!
            //! Export/Import
            //!
            //! @{
            namespace payload
            {
               void stream( common::buffer::Payload buffer, std::ostream& stream, const std::string& protocol);
               common::buffer::Payload stream( std::istream& stream, const std::string& protocol);
            } // payload

            void stream( const char* buffer, std::ostream& stream, const std::string& protocol);
            char* stream( std::istream& stream, const std::string& protocol);

            //!@}

            //!
            //! Adds a fielded buffer
            //!
            //! @returns the handle to the added buffer
            //!
            char* add( common::platform::binary::type buffer);

         } // internal

      } // field

   } // buffer

} // casual





