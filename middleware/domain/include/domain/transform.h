//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_TRANSFORM_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_TRANSFORM_H_

#include "domain/manager/admin/vo.h"
#include "domain/manager/state.h"

namespace casual
{
   namespace config
   {
      namespace domain
      {
         struct Domain;
      } // domain
   } // config
   namespace domain
   {
      namespace transform
      {

         manager::admin::vo::State state( const manager::State& state);





         manager::State state( const config::domain::Domain& domain);

         namespace configuration
         {
            namespace transaction
            {
               common::message::domain::configuration::transaction::Resource resource( const manager::state::Group::Resource& value);

               struct Resource
               {
                  auto operator() ( const manager::state::Group::Resource& value) const -> decltype( resource( value))
                  {
                     return resource( value);
                  }
               };

            } // transaction


         } // configuration



         namespace task
         {


         } // task

      } // transform

   } // domain


} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_TRANSFORM_H_
