//!
//! casual
//!

#ifndef CASUAL_COMMON_TRANSACTION_RESOURCE_H_
#define CASUAL_COMMON_TRANSACTION_RESOURCE_H_


#include "tx.h"

#include "common/platform.h"
#include "common/error/code/xa.h"
#include "common/flag/xa.h"

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

            using id_type = common::platform::resource::id::type;
            using code = error::code::xa;
            using Flag = flag::xa::Flag;
            using Flags = flag::xa::Flags;

            Resource( std::string key, xa_switch_t* xa, int id, std::string openinfo, std::string closeinfo);
            Resource( std::string key, xa_switch_t* xa);


            code start( const Transaction& transaction, Flags flags);
            code end( const Transaction& transaction, Flags flags);

            code open( Flags flags = Flag::no_flags);
            code close( Flags flags = Flag::no_flags);

            code commit( const Transaction& transaction, Flags flags);
            code rollback( const Transaction& transaction, Flags flags);

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
