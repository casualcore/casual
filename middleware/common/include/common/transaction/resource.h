//!
//! casual
//!

#ifndef CASUAL_COMMON_TRANSACTION_RESOURCE_H_
#define CASUAL_COMMON_TRANSACTION_RESOURCE_H_


#include "tx.h"

#include "common/strong/id.h"
#include "common/code/xa.h"
#include "common/flag/xa.h"

#include <string>
#include <ostream>

namespace casual
{
   namespace common
   {
      namespace transaction
      {
         class ID;

         namespace resource
         {
            struct Link
            {
               Link( std::string key, xa_switch_t* xa);

               std::string key;
               xa_switch_t* xa = nullptr;
            };
         } // resource

         struct Resource
         {

            using id_type = common::strong::resource::id;
            using code = code::xa;
            using Flag = flag::xa::Flag;
            using Flags = flag::xa::Flags;

            Resource( resource::Link link, id_type id, std::string openinfo, std::string closeinfo);


            code start( const transaction::ID& transaction, Flags flags);
            code end( const transaction::ID& transaction, Flags flags);

            code open( Flags flags = Flag::no_flags);
            code close( Flags flags = Flag::no_flags);

            code prepare( const transaction::ID& transaction, Flags flags);

            code commit( const transaction::ID& transaction, Flags flags);
            code rollback( const transaction::ID& transaction, Flags flags);

            bool dynamic() const;

            inline const std::string& key() const { return m_key;}
            inline const id_type id() const { return m_id;}


            friend std::ostream& operator << ( std::ostream& out, const Resource& resource);
            friend bool operator == ( const Resource& lhs, id_type rhs) { return lhs.m_id == rhs;}
            friend bool operator == ( id_type lhs, const Resource& rhs) { return lhs == rhs.m_id;}

         private:
            bool prepared( const transaction::ID& transaction);


            std::string m_key;
            xa_switch_t* m_xa;
            id_type m_id;

            std::string m_openinfo;
            std::string m_closeinfo;
         };

      } // transaction
   } //common


} // casual

#endif // RESOURCE_H_
