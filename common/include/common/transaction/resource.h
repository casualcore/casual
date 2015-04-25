//!
//! resource.h
//!
//! Created on: Oct 20, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_TRANSACTION_RESOURCE_H_
#define CASUAL_COMMON_TRANSACTION_RESOURCE_H_


#include "tx.h"

#include "common/platform.h"

#include <string>
#include <ostream>

namespace casual
{
   namespace common
   {
      namespace transaction
      {
         class Transaction;

         struct Resource
         {
            using id_type = common::platform::resource::id_type;

            Resource( std::string key, xa_switch_t* xa, int id, std::string openinfo, std::string closeinfo);
            Resource( std::string key, xa_switch_t* xa);


            int start( const Transaction& transaction, long flags);
            int end( const Transaction& transaction, long flags);

            int open( long flags);
            int close( long flags);

            bool dynamic() const;

            std::string key;
            xa_switch_t* xa_switch;
            id_type id = 0;

            std::string openinfo;
            std::string closeinfo;




            friend std::ostream& operator << ( std::ostream& out, const Resource& resource);
            friend bool operator == ( const Resource& lhs, id_type rhs) { return lhs.id == rhs;}
            friend bool operator == ( id_type lhs, const Resource& rhs) { return lhs == rhs.id;}
         };

      } // transaction
   } //common


} // casual

#endif // RESOURCE_H_
