//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once




#include "tx.h"

#include "common/transaction/resource/link.h"
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

         struct Resource
         {
            using code = code::xa;
            using Flag = flag::xa::Flag;
            using Flags = flag::xa::Flags;

            Resource( resource::Link link, strong::resource::id id, std::string openinfo, std::string closeinfo);


            code start( const transaction::ID& transaction, Flags flags);
            code end( const transaction::ID& transaction, Flags flags);

            code open( Flags flags = Flag::no_flags);
            code close( Flags flags = Flag::no_flags);

            code prepare( const transaction::ID& transaction, Flags flags);

            code commit( const transaction::ID& transaction, Flags flags);
            code rollback( const transaction::ID& transaction, Flags flags);

            bool dynamic() const;

            inline const std::string& key() const { return m_key;}
            inline strong::resource::id id() const { return m_id;}


            friend std::ostream& operator << ( std::ostream& out, const Resource& resource);
            friend bool operator == ( const Resource& lhs, strong::resource::id rhs) { return lhs.m_id == rhs;}
            friend bool operator == ( strong::resource::id lhs, const Resource& rhs) { return lhs == rhs.m_id;}

         private:
            bool prepared( const transaction::ID& transaction);


            std::string m_key;
            xa_switch_t* m_xa;
            strong::resource::id m_id;

            std::string m_openinfo;
            std::string m_closeinfo;
         };

      } // transaction
   } //common
} // casual
