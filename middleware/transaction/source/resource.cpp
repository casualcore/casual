//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <utility>

#include "transaction/resource.h"


#include "common/environment/expand.h"
#include "common/algorithm/compare.h"

#include "common/log/line.h"
#include "common/flag.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "common/event/send.h"

#include <array>


// stream operator for the xa struct
static std::ostream& operator << ( std::ostream& out, const xa_switch_t& xa)
{
   auto flags = casual::common::flag::xa::resource::Flag{ xa.flags};
   return casual::common::stream::write( out,  "{ name: ", xa.name, ", flags: ", flags, ", version: ", xa.version, "}");
}


namespace casual
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

            common::code::xa convert( int code)
            {
               return static_cast<  common::code::xa>( code);
            }

            namespace log
            {
               template< typename... Ts>
               void line( Ts&&... ts)
               {
                  common::log::line( common::log::category::transaction, std::forward< Ts>( ts)...);
               }

               template< typename... Ts>
               auto code( common::code::xa code, resource::id rm, Ts&&... ts)
               {
                  if( code != common::code::xa::ok)
                     common::log::error( code, "resource: ", rm, " - ", std::forward< Ts>( ts)...);

                  return code;
               }

               template< typename... Ts>
               void event( std::string_view context, Ts&&... ts)
               {
                  common::log::line( common::log::category::event::transaction, context, std::forward< Ts>( ts)...);
                  
               }

            } // log


         } // <unnamed>
      } // local

      Resource::Resource( resource::Link link, resource::id id, std::string openinfo, std::string closeinfo)
         : m_key( std::move( link.key)), m_xa( link.xa), m_id(std::move( id)), m_openinfo( std::move( openinfo)), m_closeinfo( std::move( closeinfo))
      {
         if( ! m_xa)
            common::code::raise::error( common::code::casual::invalid_argument, "xa-switch is null");

         local::log::line( "associated resource: ", *this);
      }


      common::code::xa Resource::start( const transaction::ID& transaction, Flag flags) noexcept
      {
         local::log::line( "start resource: ", m_id, " transaction: ", transaction, " flags: ", flags);

         auto result = reopen_guard( [&]()
         { 
            return local::convert( m_xa->xa_start_entry( local::non_const_xid( transaction), m_id.value(), std::to_underlying( flags)));
         });

         // this is an extra fallback/try to mitigate possible race-conditions when 
         // A a-calls B and C ( B, C with same rm) within same transaction and synchronisation is done with TM
         // still the last one to get reply might do xa_start first (depending on OS context switches and so on...) 
         if( result == common::code::xa::duplicate_xid && ! common::flag::contains( flags, Flag::join))
         {
            // Transaction is already associated with this thread of control, we try to join instead
            local::log::line( result, " - action: try to join instead");

            flags |= Flag::join;
            result = local::convert( m_xa->xa_start_entry( local::non_const_xid( transaction), m_id.value(), std::to_underlying( flags)));
         }

         local::log::event( "resource-start|", m_id, '|', transaction, '|', result);

         return local::log::code( result, m_id, "failed to start trid: ", transaction, ", flags: ", flags);
      }

      common::code::xa Resource::end( const transaction::ID& transaction, Flag flags) noexcept
      {
         local::log::line( "end resource: ", m_id, ", transaction: ", transaction, ", flags: ", flags);

         auto result = reopen_guard( [&]()
         {
            return local::convert( m_xa->xa_end_entry( local::non_const_xid( transaction), m_id.value(), std::to_underlying( flags)));
         });

         local::log::event( "resource-end|", m_id, '|', transaction, '|', result);

         return local::log::code( result, m_id, "failed to end trid: ", transaction, ", flags: ", flags);
      }

      common::code::xa Resource::open( Flag flags) noexcept
      {
         auto info = common::environment::expand( m_openinfo);
         local::log::line( "open resource: ", m_id, ", openinfo: ", info, ", flags: ", flags);

         auto result = local::convert( m_xa->xa_open_entry( info.c_str(), m_id.value(), std::to_underlying( flags)));

         // we send an event if we fail to open resource
         if( result != common::code::xa::ok)
            common::event::error::send( result, "failed to open resource: ", m_id, " '", m_xa->name, "'");

         local::log::event( "resource-open|", m_id, '|', result);

         return result;
      }

      common::code::xa Resource::close( Flag flags) noexcept
      {
         auto info = common::environment::expand( m_closeinfo);
         local::log::line( "close resource: ", m_id, ", closeinfo: ", info, ", flags: ", flags);

         auto result = local::convert( m_xa->xa_close_entry( info.c_str(), m_id.value(), std::to_underlying( flags)));

         local::log::event( "resource-close|", m_id, '|', result);

         return local::log::code( result, m_id, "failed to close - flags: ", flags);
      }

      common::code::xa Resource::prepare( const transaction::ID& transaction, Flag flags) noexcept
      {
         local::log::line( "prepare resource: ", m_id, " transaction: ", transaction, " flags: ", flags);

         auto result = reopen_guard( [&]()
         {
            return local::convert( m_xa->xa_prepare_entry( local::non_const_xid( transaction), m_id.value(), std::to_underlying( flags)));
         });

         if( result == common::code::xa::protocol)
         {
            // we need to check if we've already prepared the transaction to this physical RM.
            // that is, we know we haven't in this domain, but another domain could have prepared
            // this transaction to the same "resource-server" that both domains _have connections to_
            if( prepared( transaction))
            {
               local::log::line( common::code::xa::read_only, " trid already prepared: ", m_id, " trid: ", transaction, " flags: ", flags);
               return common::code::xa::read_only;
            }
         }

         local::log::event( "resource-prepare|", m_id, '|', transaction, '|', result);

         local::log::line( result, " prepare rm: ", m_id, " trid: ", transaction, " flags: ", flags);

         return result;
      }

      common::code::xa Resource::commit( const transaction::ID& transaction, Flag flags) noexcept
      {
         local::log::line( "commit resource: ", m_id, " transaction: ", transaction, " flags: ", flags);

         auto result = reopen_guard( [&]()
         {
            return local::convert( m_xa->xa_commit_entry( local::non_const_xid( transaction), m_id.value(), std::to_underlying( flags)));
         });

         local::log::code( result, m_id, "error during commit - xid: ", transaction.xid);

         local::log::event( "resource-commit|", m_id, '|', transaction, '|', result);

         return result;
      }

      common::code::xa Resource::rollback( const transaction::ID& transaction, Flag flags) noexcept
      {
         local::log::line( "rollback resource: ", m_id, " transaction: ", transaction, " flags: ", flags);

         auto result = reopen_guard( [&]()
         {
            return local::convert( m_xa->xa_rollback_entry( local::non_const_xid( transaction), m_id.value(), std::to_underlying( flags)));
         });

         local::log::code( result, m_id, " error during rollback - xid: ", transaction.xid);

         local::log::event( "resource-rollback|", m_id, '|', transaction, '|', result);

         return result;
      }

      bool Resource::dynamic() const noexcept
      {
         return common::flag::contains( common::flag::xa::resource::Flag( m_xa->flags), common::flag::xa::resource::Flag::dynamic);
      }

      bool Resource::migrate() const noexcept
      {
         return common::flag::contains( common::flag::xa::resource::Flag( m_xa->flags), common::flag::xa::resource::Flag::no_migrate);
      }

      common::code::xa Resource::reopen()
      {
         local::log::line( "reopen resource: ", m_id);

         // we don't care if close "fails".
         close();
         return open();
      }

      template< typename F>
      common::code::xa Resource::reopen_guard( F&& functor)
      {
         auto result = functor();

         if( common::algorithm::compare::none( result, common::code::xa::resource_fail, common::code::xa::resource_error))
            return result;
         
         common::log::error( result, "failed to interact with resource ", m_id, " - action: try to reopen the resource");
                     
         // we try to reopen the resource, and apply the functor again

         if( reopen() != common::code::xa::ok)
            return result;  // did not work, we return the original code

         return functor();
      }

      bool Resource::prepared( const transaction::ID& transaction)
      {
         std::array< XID, platform::batch::transaction::recover> xids;

         int count = xids.size();
         common::flag::xa::Flag flags = common::flag::xa::Flag::start_scan;

         while( count == std::ssize( xids))
         {
            count = m_xa->xa_recover_entry( xids.data(), xids.size(), m_id.value(), std::to_underlying( flags));

            if( count < 0)
            {
               common::log::error( common::code::xa::resource_error, local::convert( count), " failed to invoke xa_recover - rm: ", m_id);
               return false;
            }

            if( common::algorithm::find( common::range::make( std::begin( xids), count), transaction.xid))
            {
               // we found it. Make sure to end the scan if there are more.
               if( count == std::ssize( xids))
               {
                  m_xa->xa_recover_entry(
                     xids.data(), 1, m_id.value(),
                     std::to_underlying( common::flag::xa::Flag::end_scan));
               }

               return true;
            }
            flags = common::flag::xa::Flag::no_flags;
         }

         return false;
      }


      std::ostream& operator << ( std::ostream& out, const Resource& resource)
      {
         return common::stream::write( out, "{ key: ", resource.m_key,
            ", id: ", resource.m_id,
            ", openinfo: ", resource.m_openinfo,
            ", closeinfo: ", resource.m_closeinfo,
            ", xa: ", *resource.m_xa, 
            '}');
      }

   } // transaction

} // casual
