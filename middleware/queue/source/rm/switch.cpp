//!
//! casual
//!

#include "queue/rm/switch.h"

#include "queue/common/environment.h"

#include "common/transaction/id.h"
#include "common/internal/log.h"
#include "common/trace.h"
#include "common/message/transaction.h"
#include "common/communication/ipc.h"

#include "xa.h"


namespace casual
{
   namespace queue
   {
      namespace rm
      {
         namespace local
         {
            namespace
            {
               int& id()
               {
                  static int singleton = 0;
                  return singleton;
               }
            } // <unnamed>
         } // local

         int id()
         {
            return local::id();
         }

         void id( int id)
         {
            local::id() = id;
         }

      } // rm

      namespace xa
      {
         namespace local
         {
            namespace
            {
               template< typename request_message, typename reply_message>
               struct Call
               {
                  int operator() ( const common::transaction::ID& transaction, int rmid, long flags) const
                  {
                     {
                        request_message request;
                        request.trid = transaction;
                        request.resource = rmid;
                        request.process = common::process::handle();
                        request.flags = flags;

                        common::communication::ipc::blocking::send( common::communication::ipc::queue::broker::device(), request);
                     }

                     reply_message reply;

                     common::communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), reply);

                     return reply.state;
                  }
               };


            } // <unnamed>
         } // local


         int xa_open_entry( const char* openinfo, int rmid, long flags)
         {
            common::log::internal::transaction << "xa_open_entry - openinfo: " << openinfo << " rmid: " << rmid << " flags: " << flags << std::endl;

            //!
            //! @attention We can't reference environment::broker::queue::id(), since it's blocking and the queue-broker-ipc-queue
            //! is probably not advertised yet.
            //!
            rm::id( rmid);

            return XA_OK;
         }
         int xa_close_entry( const char* closeinfo, int rmid, long flags)
         {
            common::log::internal::transaction << "xa_close_entry - closeinfo: " << closeinfo << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }
         int xa_start_entry( XID* xid, int rmid, long flags)
         {
            common::Trace trace{ "queue::xa::xa_start_entry", common::log::internal::transaction};

            common::transaction::ID transaction{ *xid};
            common::log::internal::transaction << "xa_start_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }

         int xa_end_entry( XID* xid, int rmid, long flags)
         {
            common::Trace trace{ "queue::xa::xa_end_entry", common::log::internal::transaction};

            common::transaction::ID transaction{ *xid};
            common::log::internal::transaction << "xa_end_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }




         int xa_rollback_entry( XID* xid, int rmid, long flags)
         {
            common::Trace trace{ "queue::xa::xa_rollback_entry", common::log::internal::transaction};

            common::transaction::ID transaction{ *xid};
            common::log::internal::transaction << "xa_rollback_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;

            using Call = local::Call< common::message::transaction::resource::rollback::Request, common::message::transaction::resource::rollback::Reply>;

            return Call()( transaction, rmid, flags);
         }

         int xa_prepare_entry( XID* xid, int rmid, long flags)
         {
            common::Trace trace{ "queue::xa::xa_prepare_entry", common::log::internal::transaction};

            common::transaction::ID transaction{ *xid};
            common::log::internal::transaction << "xa_prepare_entry NO-OP - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;

            return XA_OK;
         }

         int xa_commit_entry( XID* xid, int rmid, long flags)
         {
            common::Trace trace{ "queue::xa::xa_commit_entry", common::log::internal::transaction};

            common::transaction::ID transaction{ *xid};
            common::log::internal::transaction << "xa_commit_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;


            using Call = local::Call< common::message::transaction::resource::commit::Request, common::message::transaction::resource::commit::Reply>;

            return Call()( transaction, rmid, flags);
         }

         int xa_recover_entry( XID* xid, long count, int rmid, long flags)
         {
            common::Trace trace{ "queue::xa::xa_recover_entry", common::log::internal::transaction};

            // TODO: ask casual-queue-broker
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

