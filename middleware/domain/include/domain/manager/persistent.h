//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "domain/manager/state.h"



namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace persistent
         {
            namespace state
            {

               void save( const State&);
               void save( const State&, const std::string& file);

               State load( const std::string& file);
               State load();

            } // state


         } // persistent

      } // manager
   } // domain


} // casual


