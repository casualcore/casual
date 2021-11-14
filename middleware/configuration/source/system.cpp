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

         std::string default_missplaced_file =  common::environment::directory::install() / "configuration" / "resources.*";

         // Try to find configuration file
         auto glob = common::environment::variable::get( 
            common::environment::variable::name::system::configuration, 
            common::environment::variable::get( common::environment::variable::name::resource::configuration, default_missplaced_file));

         return get( glob);

      }

   } // configuration::system

} // casual

