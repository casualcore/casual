//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/mockup/rm.h"
#include "common/mockup/log.h"
#include "common/argument.h"

#include "common/transaction/id.h"
#include "common/transaction/transaction.h"
#include "common/flag.h"
#include "common/exception/handle.h"

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


               struct State
               {
                  struct Transactions
                  {
                     transaction::ID current;
                     std::vector< transaction::ID> all;

                  } transactions;

                  int xa_open_return = XA_OK;
                  int xa_close_return = XA_OK;
                  int xa_start_return = XA_OK;
                  int xa_end_return = XA_OK;
                  int xa_prepare_return = XA_OK;
                  int xa_commit_return = XA_OK;
                  int xa_rollback_return = XA_OK;

               };

               std::map< int, State> state;


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

         int xa_open_entry( const char* c_openinfo, int rmid, long flags)
         {
            auto& state = local::state[ rmid];

            // clear
            state = local::State{};

            std::string openinfo( c_openinfo);

            try
            {
               argument::Parse parse{ "mockup rm",
                  argument::Option( std::tie( state.xa_open_return), { "--open"}, ""),
                  argument::Option( std::tie( state.xa_close_return), { "--close"}, ""),
                  argument::Option( std::tie( state.xa_start_return), { "--start"}, ""),
                  argument::Option( std::tie( state.xa_end_return), { "--end"}, ""),
                  argument::Option( std::tie( state.xa_prepare_return), { "--prepare"}, ""),
                  argument::Option( std::tie( state.xa_commit_return), { "--commit"}, ""),
                  argument::Option( std::tie( state.xa_rollback_return), { "--rollback"}, "")
               };
               
               parse( common::string::split( openinfo));
            }
            catch( ...)
            {
               log::line( log, "failed to parse mockup openinfo: ", openinfo);
            }


            if( ! state.transactions.all.empty())
            {
               log::line( log::category::error, "xa_open_entry - rmid: ", rmid, " has associated transactions ", state.transactions.all);
               return XAER_PROTO;
            }

            log::line( log, "xa_open_entry - openinfo: ", openinfo, " rmid: ", rmid, " flags: ", flags);
            return state.xa_open_return;
         }
         int xa_close_entry( const char* closeinfo, int rmid, long flags)
         {
            auto& state = local::state[ rmid];

            if( ! state.transactions.all.empty())
            {
               log::line( log::category::error, "xa_close_entry - rmid: ", rmid, " has associated transactions ", state.transactions.all);
               return XAER_PROTO;
            }
            log << "xa_close_entry - closeinfo: " << closeinfo << " rmid: " << rmid << " flags: " << flags << '\n';

            return state.xa_close_return;
         }
         int xa_start_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID trid{ *xid};
            log << "xa_start_entry - trid: " << trid << " rmid: " << rmid << " flags: " << flags << '\n';

            auto& state = local::state[ rmid];

            if( state.transactions.current)
            {
               log::category::error  << "XAER_PROTO: xa_start_entry - a transaction is active - " << state.transactions.current << '\n';
               return XAER_PROTO;
            }

            auto found = algorithm::find( state.transactions.all, trid);

            if( ! found)
            {
               state.transactions.all.emplace_back( trid);
            }
            else
            {
               if( ! common::has::flag< TMRESUME>( flags))
               {
                  log::category::error << "XAER_PROTO: xa_start_entry - the transaction is suspended, but no TMRESUME in flags - " << state.transactions.current << '\n';
                  return XAER_PROTO;
               }
            }

            state.transactions.current = trid;

            return state.xa_start_return;
         }

         int xa_end_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID trid{ *xid};
            log << "xa_end_entry - xid: " << trid << " rmid: " << rmid << " flags: " << flags << '\n';

            auto& state = local::state[ rmid];

            if( state.transactions.current != trid)
            {
               log::category::error << "XAER_INVAL: xa_end_entry - transaction not current with RM" << '\n';
               return XAER_INVAL;
            }

            state.transactions.current = transaction::ID{};

            if( ! common::has::flag< TMSUSPEND>( flags))
            {
               auto found = algorithm::find( state.transactions.all, trid);

               if( found)
               {
                  state.transactions.all.erase( std::begin( found));
               }
            }
            return state.xa_end_return;
         }

         int xa_rollback_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log << "xa_rollback_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << '\n';

            auto& state = local::state[ rmid];

            return state.xa_rollback_return;
         }

         int xa_prepare_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log << "xa_prepare_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << '\n';

            auto& state = local::state[ rmid];

            return state.xa_prepare_return;
         }

         int xa_commit_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log << "xa_commit_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << '\n';

            auto& state = local::state[ rmid];

            if( state.xa_commit_return == XA_OK)
            {
               if( state.transactions.current == transaction)
                  state.transactions.current = transaction::ID{};

               auto found = algorithm::find( state.transactions.all, transaction);

               if( found)
               {
                  state.transactions.all.erase( std::begin( found));
               }
            }

            return state.xa_commit_return;
         }

         int xa_recover_entry( XID* xid, long count, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log << "xa_recover_entry - xid: " << transaction << " count: " << count << " rmid: " << rmid << " flags: " << flags << '\n';


            return 0;
         }

         int xa_forget_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log << "xa_forget_entry - xid: " << transaction << " rmid: " << rmid << " flags: " << flags << '\n';

            return XA_OK;
         }

         int xa_complete_entry( int* handle, int* retval, int rmid, long flags)
         {
            log << "xa_complete_entry - handle:" << handle << " retval: " << retval << " rmid: " << rmid << " flags: " << flags << '\n';

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




