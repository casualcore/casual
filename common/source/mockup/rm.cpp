//!
//! rm.cpp
//!
//! Created on: May 29, 2014
//!     Author: Lazan
//!

#include "common/mockup/rm.h"
#include "common/transaction/id.h"
#include "common/transaction/transaction.h"
#include "common/internal/log.h"

#include "xa.h"


namespace casual
{
   namespace common
   {
      namespace mockup
      {
         namespace local
         {
            namespace
            {
               std::map< int, std::vector< transaction::Transaction>> rms;


               struct Active
               {
                  template< typename T>
                  bool operator () ( T&& value) const
                  {
                     return ! value.suspended;
                  }
               };

            } // <unnamed>
         } // local

         int xa_open_entry( const char* openinfo, int rmid, long flags)
         {
            auto& transactions = local::rms[ rmid];

            if( ! transactions.empty())
            {
               log::error << "xa_open_entry - rmid: " << rmid << " has associated transactions " << range::make( transactions) << std::endl;
               return XAER_PROTO;
            }

            log::internal::transaction << "xa_open_entry - openinfo: " << openinfo << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }
         int xa_close_entry( const char* closeinfo, int rmid, long flags)
         {
            auto& transactions = local::rms[ rmid];

            if( ! transactions.empty())
            {
               log::error << "xa_close_entry - rmid: " << rmid << " has associated transactions " << range::make( transactions) << std::endl;
               return XAER_PROTO;
            }
            log::internal::transaction << "xa_close_entry - closeinfo: " << closeinfo << " rmid: " << rmid << " flags: " << flags << std::endl;

            return XA_OK;
         }
         int xa_start_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID trid{ *xid};
            log::internal::transaction << "xa_start_entry - trid: " << trid << " rmid: " << rmid << " flags: " << flags << std::endl;

            auto& transactions = local::rms[ rmid];


            auto active = range::partition( transactions, local::Active{});

            if( std::get< 0>( active))
            {
               log::error << error::xa::error( XAER_PROTO) << " xa_start_entry - a transaction is active - " << *std::get< 0>( active) << std::endl;
               return XAER_PROTO;
            }

            auto found = range::find( transactions, trid);

            if( found)
            {
               found->suspended = false;
            }
            else
            {
               transactions.emplace_back( trid);
            }

            return XA_OK;
         }

         int xa_end_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID trid{ *xid};
            log::internal::transaction << "xa_end_entry - xid: " << trid << " rmid: " << rmid << " flags: " << flags << std::endl;

            auto& transactions = local::rms[ rmid];

            auto found = range::find( transactions, trid);

            if( found)
            {
               transactions.erase( found.first);
            }
            else
            {
               log::error << error::xa::error( XAER_INVAL) << " xa_end_entry - transaction not associated with RM" << std::endl;
               return XAER_INVAL;
            }

            return XA_OK;
         }

         int xa_rollback_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log::internal::transaction << "xa_rollback_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }

         int xa_prepare_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log::internal::transaction << "xa_prepare_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }

         int xa_commit_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log::internal::transaction << "xa_commit_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;

            return XA_OK;
         }

         int xa_recover_entry( XID* xid, long count, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log::internal::transaction << "xa_recover_entry - xid: " << transaction << " count: " << count << " rmid: " << rmid << " flags: " << flags << std::endl;

            return XA_OK;
         }

         int xa_forget_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log::internal::transaction << "xa_forget_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;

            return XA_OK;
         }

         int xa_complete_entry( int* handle, int* retval, int rmid, long flags)
         {
            log::internal::transaction << "xa_complete_entry - handle:" << handle << " retval: " << retval << " rmid: " << rmid << " flags: " << flags << std::endl;

            return XA_OK;
         }
      } // mockup
   } // common
} // casual


extern "C"
{
   struct xa_switch_t casual_mockup_xa_switch_static{
      "Casual Mockup XA",
      TMNOMIGRATE,
      0,
      &casual::common::mockup::xa_open_entry,
      &casual::common::mockup::xa_close_entry,
      &casual::common::mockup::xa_start_entry,
      &casual::common::mockup::xa_end_entry,
      &casual::common::mockup::xa_rollback_entry,
      &casual::common::mockup::xa_prepare_entry,
      &casual::common::mockup::xa_commit_entry,
      &casual::common::mockup::xa_recover_entry,
      &casual::common::mockup::xa_forget_entry,
      &casual::common::mockup::xa_complete_entry
   };

}




