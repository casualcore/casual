//!
//! mockup.cpp
//!
//! Created on: Aug 1, 2013
//!     Author: Lazan
//!

#include "common/mockup.h"

namespace casual
{
   namespace common
   {
      namespace mockup
      {

         int xa_open_entry( const char* openinfo, int rmid, long flags)
         {
            return XA_OK;
         }
         int xa_close_entry( const char *, int, long)
         {
            return XA_OK;
         }
         int xa_start_entry( XID* xid, int rmid, long flags)
         {
            return XA_OK;
         }

         int xa_end_entry( XID* xid, int rmid, long flags)
         {
            return XA_OK;
         }

         int xa_rollback_entry( XID* xid, int rmid, long flags)
         {
            return XA_OK;
         }

         int xa_prepare_entry( XID* xid, int rmid, long flags)
         {
            return XA_OK;
         }

         int xa_commit_entry( XID* xid, int rmid, long flags)
         {
            return XA_OK;
         }

         int xa_recover_entry( XID* xid, long count, int rmid, long flags)
         {
            return XA_OK;
         }

         int xa_forget_entry( XID* xid, int rmid, long flags)
         {
            return XA_OK;
         }

         int xa_complete_entry(int *, int *, int, long)
         {
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


