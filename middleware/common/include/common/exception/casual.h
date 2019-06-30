//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/code/casual.h"
#include "common/exception/common.h"

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace casual 
         {

            using exception = common::exception::base_error< code::casual>;

            template< code::casual error>
            using base = common::exception::basic_error< exception, error>;
            
            using Shutdown = base< code::casual::shutdown>;

            namespace invalid
            {
               
               
               using configuration_base = base< code::casual::invalid_configuration>;
               struct Configuration : configuration_base
               {
                  using detail_type = std::vector< std::string>;
                  
                  Configuration( const std::string& message, detail_type details) 
                     : configuration_base( message), m_details( std::move( details)) {}

                  Configuration( const std::string& message) : configuration_base( message) {}
                  

                  inline const detail_type& details() const { return m_details;} 
                  
               private:
                  detail_type m_details;
               };

               using Version = base< code::casual::invalid_version>;

               using Node = base< code::casual::invalid_node>;
               using Document = base< code::casual::invalid_document>;
               
            } // invalid

         } // casual 
      } // exception 
   } // common
} // casual


