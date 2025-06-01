//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "casual/transaction/id.h"

#include "transaction/resource/link.h"


#include "common/transaction/id.h"
#include "common/strong/id.h"
#include "common/code/xa.h"
#include "common/flag/xa.h"

#include "tx.h"

#include <string>
#include <ostream>


namespace casual
{
   namespace transaction
   {

      struct Resource
      {
         using Flag = common::flag::xa::Flag;

         Resource( resource::Link link, common::strong::resource::id id, std::string openinfo, std::string closeinfo);
         
         common::code::xa start( const transaction::ID& transaction, Flag flags) noexcept;
         common::code::xa end( const transaction::ID& transaction, Flag flags) noexcept;

         common::code::xa open( Flag flags = Flag::no_flags) noexcept;
         common::code::xa close( Flag flags = Flag::no_flags) noexcept;

         common::code::xa prepare( const transaction::ID& transaction, Flag flags) noexcept;

         common::code::xa commit( const transaction::ID& transaction, Flag flags) noexcept;
         common::code::xa rollback( const transaction::ID& transaction, Flag flags) noexcept;

         bool dynamic() const noexcept;

         inline const std::string& key() const noexcept { return m_key;}
         inline resource::id id() const noexcept { return m_id;}
         inline std::string_view name() const noexcept { return m_xa->name;}

         bool migrate() const noexcept;
      

         friend std::ostream& operator << ( std::ostream& out, const Resource& resource);
         friend bool operator == ( const Resource& lhs, resource::id rhs) { return lhs.m_id == rhs;}

      private:
         //! tries to reopen the resource
         common::code::xa reopen();

         //! if `functor` returns xa::resource_fail, reopen the resource, 
         //! and apply the `functor` again
         template< typename F>
         common::code::xa reopen_guard( F&& functor);

         bool prepared( const transaction::ID& transaction);


         std::string m_key;
         xa_switch_t* m_xa;
         resource::id m_id;

         std::string m_openinfo;
         std::string m_closeinfo;
      };

   } //common::transaction
} // casual
