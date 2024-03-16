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
   namespace common::transaction
   {
      class ID;

      struct Resource
      {
         using Flag = flag::xa::Flag;

         Resource( resource::Link link, strong::resource::id id, std::string openinfo, std::string closeinfo);
         
         code::xa start( const transaction::ID& transaction, Flag flags) noexcept;
         code::xa end( const transaction::ID& transaction, Flag flags) noexcept;

         code::xa open( Flag flags = Flag::no_flags) noexcept;
         code::xa close( Flag flags = Flag::no_flags) noexcept;

         code::xa prepare( const transaction::ID& transaction, Flag flags) noexcept;

         code::xa commit( const transaction::ID& transaction, Flag flags) noexcept;
         code::xa rollback( const transaction::ID& transaction, Flag flags) noexcept;

         bool dynamic() const noexcept;

         inline const std::string& key() const noexcept { return m_key;}
         inline strong::resource::id id() const noexcept { return m_id;}
         inline std::string_view name() const noexcept { return m_xa->name;}

         bool migrate() const noexcept;
      

         friend std::ostream& operator << ( std::ostream& out, const Resource& resource);
         friend bool operator == ( const Resource& lhs, strong::resource::id rhs) { return lhs.m_id == rhs;}
         friend bool operator == ( strong::resource::id lhs, const Resource& rhs) { return lhs == rhs.m_id;}

      private:
         //! tries to reopen the resource
         code::xa reopen();

         //! if `functor` returns xa::resource_fail, reopen the resource, 
         //! and apply the `functor` again
         template< typename F>
         code::xa reopen_guard( F&& functor);

         bool prepared( const transaction::ID& transaction);


         std::string m_key;
         xa_switch_t* m_xa;
         strong::resource::id m_id;

         std::string m_openinfo;
         std::string m_closeinfo;
      };

   } //common::transaction
} // casual
