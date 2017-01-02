//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_INTERNAL_TRANSACTION_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_INTERNAL_TRANSACTION_H_

#include "common/marshal/marshal.h"

#include "configuration/transaction.h"

#include <string>
#include <vector>

namespace casual
{
   namespace configuration
   {
      namespace internal
      {
         namespace transaction
         {
            struct Manager
            {
               std::string path;
               std::string database;

               CASUAL_CONST_CORRECT_MARSHAL
               (
                  archive & path;
                  archive & database;
               )
            };


            struct Resource
            {
               std::size_t id = 0;
               std::string key;
               std::size_t instances = 0;

               std::string openinfo;
               std::string closeinfo;

               CASUAL_CONST_CORRECT_MARSHAL
               (
                  archive & id;
                  archive & key;
                  archive & instances;
                  archive & openinfo;
                  archive & closeinfo;
               )
            };

            struct Transaction
            {
               Manager manager;
               std::vector< Resource> resources;

               CASUAL_CONST_CORRECT_MARSHAL
               (
                  archive & manager;
                  archive & resources;
               )
            };


            casual::configuration::transaction::Transaction transform( const Transaction& value);
            Transaction transform( const casual::configuration::transaction::Transaction& value);

         } // transaction
      } // internal
   } // configuration
} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_INTERNAL_TRANSACTION_H_
