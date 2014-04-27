//!
//! mockup.cpp
//!
//! Created on: Aug 1, 2013
//!     Author: Lazan
//!

#include "common/mockup.h"

#include "common/internal/log.h"
#include "common/transaction_id.h"


#include <map>

namespace casual
{
   namespace common
   {
      namespace mockup
      {
         namespace local
         {
            struct Resources
            {
               using queue_id_type = common::platform::queue_id_type;

               static Resources& instance()
               {
                  static Resources singleton;
                  return singleton;
               }


               void clear()
               {
                  queues.clear();
               }

               void add( queue_id_type id, const common::ipc::message::Complete& message)
               {
                  common::ipc::message::Complete temp;
                  temp.complete = message.complete;
                  temp.correlation = message.correlation;
                  temp.payload = message.payload;
                  temp.type = message.type;

                  queues[ id].push_back( std::move( temp));
               }

               std::vector< common::ipc::message::Complete> get( queue_id_type id)
               {
                  std::vector< common::ipc::message::Complete> result;

                  auto& messages = queues[ id];

                  if( ! messages.empty())
                  {
                     result.push_back( std::move( messages.front()));
                     messages.pop_front();
                  }
                  return result;
               }

               std::vector< common::ipc::message::Complete> get( queue_id_type id, queue::message_type_type type)
               {
                  std::vector< common::ipc::message::Complete> result;

                  auto& messages = queues[ id];

                  auto findIter = std::find_if(
                        std::begin( messages),
                        std::end( messages),
                        [=]( const common::ipc::message::Complete& message) { return message.type == type;});

                  if( findIter != std::end( messages))
                  {
                     result.push_back( std::move( *findIter));
                     messages.erase( findIter);
                  }

                  return result;

               }

            private:

               typedef std::map< common::platform::queue_id_type, std::deque< common::ipc::message::Complete>> queue_type;

               queue_type queues;

               Resources() {};

            };
         } // local

         void queue::clearAllQueues()
         {
            local::Resources::instance().clear();
         }


         namespace ipc
         {

            bool Queue::operator () ( const message_type& message, const long flags) const
            {
               local::Resources::instance().add( m_id, std::move( message));

               return true;
            }

            std::vector< Queue::message_type> Queue::operator () ( const long flags)
            {
               return local::Resources::instance().get( m_id);
            }


            std::vector< Queue::message_type> Queue::operator () ( message_type::message_type_type type, const long flags)
            {
               return local::Resources::instance().get( m_id, type);
            }


         } // ipc



         int xa_open_entry( const char* openinfo, int rmid, long flags)
         {
            log::internal::transaction << "xa_open_entry - openinfo: " << openinfo << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }
         int xa_close_entry( const char* closeinfo, int rmid, long flags)
         {
            log::internal::transaction << "xa_close_entry - closeinfo: " << closeinfo << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }
         int xa_start_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log::internal::transaction << "xa_start_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;
            return XA_OK;
         }

         int xa_end_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log::internal::transaction << "xa_end_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << std::endl;
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


