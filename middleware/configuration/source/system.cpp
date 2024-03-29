//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/system.h"
#include "configuration/model/load.h"

#include "common/file.h"
#include "common/environment.h"


namespace casual
{
   using namespace common;


   namespace configuration::system
   {

      model::system::Model get( const std::string& glob)
      {
         return configuration::model::load( file::find( glob)).system;
      }

      model::system::Model get()
      {

         std::string default_misplaced_file =  common::environment::directory::install() / "configuration" / "resources.*";

         // Try to find configuration file
         auto glob = common::environment::variable::get( 
            common::environment::variable::name::system::configuration).value_or(
               common::environment::variable::get( 
                  common::environment::variable::name::resource::configuration).value_or( default_misplaced_file));

         return get( glob);

      }

   } // configuration::system

} // casual

