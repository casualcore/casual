//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/strong/id.h"
#include <string>

namespace casual
{
   namespace common::execution
   {

      using type = strong::execution::id;

      //! Sets current execution id
      void id( const type& id);

      //! sets a new random execution id.
      void reset();

      //! Gets current execution id
      const type& id();

      namespace service
      {
         //! Sets the current service
         void name( const std::string& service);

         //! Gets the current service (if any)
         const std::string& name();

         //! clear the service name
         void clear();

         namespace parent
         {
            //! Sets the current parent service
            void name( const std::string& service);

            //! Gets the current parent service (if any)
            const std::string& name();

            void clear();

         } // parent
      } // service

   } // common::execution
} // casual
