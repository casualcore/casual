//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/model/load.h"
#include "configuration/model/transform.h"
#include "configuration/model.h"
#include "configuration/user/load.h"
#include "configuration/common.h"


namespace casual
{
   using namespace common;
   namespace configuration
   {
      namespace model
      {

         configuration::Model load( const std::vector< std::string>& files)
         {
            Trace trace{ "configuration::model::load"};
            log::line( verbose::log, "files: ", files);

            auto model = model::transform( user::load( files));

            log::line( verbose::log, "model: ", model);

            return model;
         }

      } // model
   } // configuration

} // casual