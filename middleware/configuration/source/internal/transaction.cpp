//!
//! casual 
//!

#include "configuration/internal/transaction.h"

#include "common/algorithm.h"

namespace casual
{

   namespace configuration
   {
      namespace internal
      {
         namespace transaction
         {

            casual::configuration::transaction::Transaction transform( const Transaction& value)
            {
               return {};
            }

            Transaction transform( const casual::configuration::transaction::Transaction& value)
            {
               Transaction result;

               result.manager.path = common::coalesce( value.manager.path, result.manager.path);
               result.manager.database = common::coalesce( value.manager.database, result.manager.database);

               auto transform_resource = []( const casual::configuration::transaction::Resource& value){
                  Resource result;

                  result.key = value.key;
                  result.instances = std::stoul( common::coalesce(  value.instances, "0"));

                  return result;
               };


               common::range::transform( value.resources, result.resources, transform_resource);

               return result;
            }

         } // transaction
      } // internal
   } // configuration
} // casual
