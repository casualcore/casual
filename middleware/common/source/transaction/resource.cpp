//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <utility>

#include "common/transaction/resource.h"
#include "common/transaction/id.h"
#include "common/environment/expand.h"

#include "common/log/category.h"
#include "common/flag.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "common/event/send.h"

#include <array>

namespace casual
{
   namespace common
   {
      namespace transaction
      {

         namespace local
         {
            namespace
            {
               XID* non_const_xid( const transaction::ID& transaction)
               {
                  return const_cast< XID*>( &transaction.xid);
               }

               Resource::code convert( int code)
               {
                  return static_cast<  Resource::code>( code);
               }


            } // <unnamed>
         } // local

         Resource::Resource( resource::Link link, strong::resource::id id, std::string openinfo, std::string closeinfo)
            : m_key( std::move( link.key)), m_xa( link.xa), m_id(std::move( id)), m_openinfo( std::move( openinfo)), m_closeinfo( std::move( closeinfo))
         {
            if( ! m_xa)
               common::code::raise::error( common::code::casual::invalid_argument, "xa-switch is null");

            log::line( log::category::transaction, "associated resource: ", *this);
         }


         Resource::code Resource::start( const transaction::ID& transaction, Flags flags) noexcept
         {
            log::line( log::category::transaction, "start resource: ", m_id, " transaction: ", transaction, " flags: ", flags);

            if( flags.exist( Flag::join) && migrate())
               flags = ( flags - Flag::join) + Flag::resume;

            auto result = reopen_guard( [&]()
            { 
               return local::convert( m_xa->xa_start_entry( local::non_const_xid( transaction), m_id.value(), flags.underlaying()));
            });

            if( result == code::duplicate_xid && ! flags.exist( Flag::join))
            {
               // Transaction is already associated with this thread of control, we try to join instead
               log::line( log::category::transaction, result, " - action: try to join instead");

               flags |= Flag::join;

               result = local::convert( m_xa->xa_start_entry( local::non_const_xid( transaction), m_id.value(), flags.underlaying()));
            }

            if( result != code::ok)
               log::line( log::category::error, result, " failed to start resource - ", m_id, " - trid: ", transaction);

            return result;
         }

         Resource::code Resource::end( const transaction::ID& transaction, Flags flags) noexcept
         {
            log::line( log::category::transaction, "end resource: ", m_id, " transaction: ", transaction, " flags: ", flags);

            // if the resource support migrate, we add migrate to flags
            if( flags.exist( Flag::suspend) && migrate())
               flags |= Flag::migrate;

            auto result = reopen_guard( [&]()
            {
               return local::convert( m_xa->xa_end_entry( local::non_const_xid( transaction), m_id.value(), flags.underlaying()));
            });

            if( result != code::ok)
               log::line( log::category::error, result, " failed to end resource - ", m_id, " - trid: ", transaction);

            return result;
         }

         Resource::code Resource::open( Flags flags) noexcept
         {
            log::line( log::category::transaction, "open resource: ", m_id, " openinfo: ", m_openinfo, " flags: ", flags);

            auto info = common::environment::expand( m_openinfo);

            auto result = local::convert( m_xa->xa_open_entry( info.c_str(), m_id.value(), flags.underlaying()));

            // we send an event if we fail to open resource
            if( result != code::ok)
               common::event::error::send( result, "failed to open resource: ", m_id, " '", m_xa->name, "'");

            return result;
         }

         Resource::code Resource::close( Flags flags) noexcept
         {
            log::line( log::category::transaction, "close resource: ", m_id, " closeinfo: ", m_closeinfo, " flags: ", flags);

            auto info = common::environment::expand( m_closeinfo);

            auto result = local::convert( m_xa->xa_close_entry( info.c_str(), m_id.value(), flags.underlaying()));

            if( result != code::ok)
               log::line( log::category::error, result, " failed to close resource: ", m_id, " '", m_xa->name, "'");

            return result;
         }

         Resource::code Resource::prepare( const transaction::ID& transaction, Flags flags) noexcept
         {
            auto result = reopen_guard( [&]()
            {
               return local::convert( m_xa->xa_prepare_entry( 
                  local::non_const_xid( transaction), m_id.value(), flags.underlaying()));
            });

            if( result == common::code::xa::protocol)
            {
               // we need to check if we've already prepared the transaction to this physical RM.
               // that is, we now we haven't in this domain, but another domain could have prepared
               // this transaction to the same "resource-server" that both domains _have connections to_
               if( prepared( transaction))
               {
                  log::line( log::category::transaction, common::code::xa::read_only, " trid already prepared: ", m_id, " trid: ", transaction, " flags: ", flags);
                  return common::code::xa::read_only;
               }
            }

            log::line( log::category::transaction, result, " prepare rm: ", m_id, " trid: ", transaction, " flags: ", flags);

            return result;
         }

         Resource::code Resource::commit( const transaction::ID& transaction, Flags flags) noexcept
         {
            log::line( log::category::transaction, "commit resource: ", m_id, " flags: ", flags);

            return reopen_guard( [&]()
            {
               return local::convert( m_xa->xa_commit_entry( local::non_const_xid( transaction), m_id.value(), flags.underlaying()));
            });
         }

         Resource::code Resource::rollback( const transaction::ID& transaction, Flags flags) noexcept
         {
            log::line( log::category::transaction, "rollback resource: ", m_id, " flags: ", flags);

            return reopen_guard( [&]()
            {
               return local::convert( m_xa->xa_rollback_entry( local::non_const_xid( transaction), m_id.value(), flags.underlaying()));
            });
         }

         bool Resource::dynamic() const noexcept
         {
            return static_cast< flag::xa::resource::Flags>( m_xa->flags).exist( flag::xa::resource::Flag::dynamic);
         }

         bool Resource::migrate() const noexcept
         {
            return ! flag::xa::resource::Flags{ m_xa->flags}.exist( flag::xa::resource::Flag::no_migrate);
         }

         Resource::code Resource::reopen()
         {
            log::line( log::category::transaction, "reopen resource: ", m_id);

            // we don't care if close "fails".
            close();
            return open();
         }

         template< typename F>
         Resource::code Resource::reopen_guard( F&& functor)
         {
            auto result = functor();

            if( result != code::resource_fail)
               return result;
            
            log::line( log::category::error, result, " failed to interact with resource ", m_id, " - action: try to reopen the resource");
                        
            // we try to reopen the resource, and apply the functor again

            if( reopen() != code::ok)
               return result;  // did not work, we return the original code

            return functor();
         }

         bool Resource::prepared( const transaction::ID& transaction)
         {
            std::array< XID, platform::batch::transaction::recover> xids;

            int count = xids.size();
            flag::xa::Flags flags = flag::xa::Flag::start_scan;

            while(  count == range::size( xids))
            {
               count = m_xa->xa_recover_entry( xids.data(), xids.size(), m_id.value(), flags.underlaying());

               if( count < 0)
               {
                  log::line( log::category::error, local::convert( count), " failed to invoke xa_recover - rm: ", m_id);
                  return false;
               }

               if( common::algorithm::find( range::make( std::begin( xids), count), transaction.xid))
               {
                  // we found it. Make sure to end the scan if there are more.
                  if( count == range::size( xids))
                  {
                     m_xa->xa_recover_entry(
                        xids.data(), 1, m_id.value(),
                        cast::underlying( flag::xa::Flag::end_scan));
                  }

                  return true;
               }
               flags = flag::xa::Flag::no_flags;
            }

            return false;
         }


         std::ostream& operator << ( std::ostream& out, const Resource& resource)
         {
            return out << "{ key: " << resource.m_key 
               << ", id: " << resource.m_id 
               << ", openinfo: " << resource.m_openinfo
               << ", closeinfo: " << resource.m_closeinfo 
               << ", xa: { name: " << resource.m_xa->name
               << ", flags: " << resource.m_xa->flags
               << ", version: " << resource.m_xa->version
               << "}}";
         }


      } //transaction
   } // common

} // casual
