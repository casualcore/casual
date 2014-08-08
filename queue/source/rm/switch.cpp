//!
//! switch.cpp
//!
//! Created on: Jul 7, 2014
//!     Author: Lazan
//!

#include "queue/rm/switch.h"


#include "common/transaction_id.h"
#include "common/internal/log.h"

#include "xa.h"


namespace casual
{
   namespace queue
   {
      namespace xa
      {
         int xa_open_entry( const char* openinfo, int rmid, long flags)
         {
            common::log::internal::transaction << "xa_open_entry - openinfo: " << openinfo << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }
         int xa_close_entry( const char* closeinfo, int rmid, long flags)
         {
            common::log::internal::transaction << "xa_close_entry - closeinfo: " << closeinfo << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }
         int xa_start_entry( XID* xid, int rmid, long flags)
         {
            common::transaction::ID transaction{ *xid};
            common::log::internal::transaction << "xa_start_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }

         int xa_end_entry( XID* xid, int rmid, long flags)
         {
            common::transaction::ID transaction{ *xid};
            common::log::internal::transaction << "xa_end_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }

         int xa_rollback_entry( XID* xid, int rmid, long flags)
         {
            common::transaction::ID transaction{ *xid};
            common::log::internal::transaction << "xa_rollback_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }

         int xa_prepare_entry( XID* xid, int rmid, long flags)
         {
            common::transaction::ID transaction{ *xid};
            common::log::internal::transaction << "xa_prepare_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }

         int xa_commit_entry( XID* xid, int rmid, long flags)
         {
            common::transaction::ID transaction{ *xid};
            common::log::internal::transaction << "xa_commit_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;

            return XA_OK;
         }

         int xa_recover_entry( XID* xid, long count, int rmid, long flags)
         {
            common::transaction::ID transaction{ *xid};
            common::log::internal::transaction << "xa_recover_entry - xid: " << transaction << " count: " << count << " rmid: " << rmid << " flags: " << flags << std::endl;

            return XA_OK;
         }

         int xa_forget_entry( XID* xid, int rmid, long flags)
         {
            common::transaction::ID transaction{ *xid};
            common::log::internal::transaction << "xa_forget_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;

            return XA_OK;
         }

         int xa_complete_entry( int* handle, int* retval, int rmid, long flags)
         {
            common::log::internal::transaction << "xa_complete_entry - handle:" << handle << " retval: " << retval << " rmid: " << rmid << " flags: " << flags << std::endl;

            return XA_OK;
         }
      } // xa
   } // queue
} // casual


extern "C"
{
   struct xa_switch_t casual_queue_xa_switch_dynamic{
      "casual-queue-rm-dynamic",
      TMNOMIGRATE | TMREGISTER,
      0,
      &casual::queue::xa::xa_open_entry,
      &casual::queue::xa::xa_close_entry,
      &casual::queue::xa::xa_start_entry,
      &casual::queue::xa::xa_end_entry,
      &casual::queue::xa::xa_rollback_entry,
      &casual::queue::xa::xa_prepare_entry,
      &casual::queue::xa::xa_commit_entry,
      &casual::queue::xa::xa_recover_entry,
      &casual::queue::xa::xa_forget_entry,
      &casual::queue::xa::xa_complete_entry
   };

}

