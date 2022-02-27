//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest/rm.h"
#include "common/unittest/log.h"
#include "common/argument.h"
#include "common/code/xa.h"


#include "common/transaction/transaction.h"
#include "common/transaction/context.h"
#include "common/flag.h"
#include "common/exception/capture.h"



namespace casual
{
   namespace common
   {
      namespace unittest
      {
         namespace local
         {
            namespace
            {

               struct Active
               {
                  template< typename T>
                  bool operator () ( T&& value) const
                  {
                     return ! value.suspended;
                  }
               };

               struct State : rm::State 
               {
                  State() = default;
                  struct
                  {
                     code::xa open = code::xa::ok;
                     code::xa close = code::xa::ok;
                     code::xa start = code::xa::ok;
                     code::xa end = code::xa::ok;
                     code::xa prepare = code::xa::ok;
                     code::xa commit = code::xa::ok;
                     code::xa rollback= code::xa::ok;

                  } result;

                  auto error( code::xa code)
                  {
                     errors.push_back( code);
                     return cast::underlying( code);
                  }
               };

               namespace global
               {
                  std::map< strong::resource::id, State> state;
               } // global

               auto& state( int id, rm::State::Invoke invoked)
               {  
                  auto& state = global::state[ strong::resource::id{ id}];
                  state.invocations.push_back( invoked);
                  return state;
               }

            } // <unnamed>
         } // local

         namespace rm
         {
            std::ostream& operator << ( std::ostream& out, State::Invoke value)
            {
               using Enum = State::Invoke;
               switch( value)
               {
                  case Enum::xa_close_entry: return out << "xa_close_entry";
                  case Enum::xa_commit_entry: return out << "xa_commit_entry";
                  case Enum::xa_complete_entry: return out << "xa_complete_entry";
                  case Enum::xa_end_entry: return out << "xa_end_entry";
                  case Enum::xa_forget_entry: return out << "xa_forget_entry";
                  case Enum::xa_open_entry: return out << "xa_open_entry";
                  case Enum::xa_prepare_entry: return out << "xa_prepare_entry";
                  case Enum::xa_recover_entry: return out << "xa_recover_entry";
                  case Enum::xa_rollback_entry: return out << "xa_rollback_entry";
                  case Enum::xa_start_entry: return out << "xa_start_entry";
               }
               assert( ! "invalid State::Invoke");
            }

            void registration( strong::resource::id id)
            {
               auto& state = local::global::state.at( id);

               transaction::context().resource_registration( id, &state.transactions.current.xid);

            }

            const State& state( strong::resource::id id)
            {
               return local::global::state.at( id);
            }

            void clear()
            {
               local::global::state.clear();
            }
            
         } // rm

         int xa_open_entry( const char* c_openinfo, int rmid, long flags)
         {
            auto& state = local::state( rmid, rm::State::Invoke::xa_open_entry);

            // clear
            state = local::State{};
            state.invocations.push_back( rm::State::Invoke::xa_open_entry);


            std::string openinfo( c_openinfo);

            try
            {
               auto parse_result = []( code::xa& code)
               {
                  return [&code]( const std::string& value) { 
                     code = code::xa{ string::from< int>( value)};
                  };
               };

               argument::Parse{ "mockup rm",
                  argument::Option( parse_result( state.result.open), { "--open"}, ""),
                  argument::Option( parse_result( state.result.close), { "--close"}, ""),
                  argument::Option( parse_result( state.result.start), { "--start"}, ""),
                  argument::Option( parse_result( state.result.end), { "--end"}, ""),
                  argument::Option( parse_result( state.result.prepare), { "--prepare"}, ""),
                  argument::Option( parse_result( state.result.commit), { "--commit"}, ""),
                  argument::Option( parse_result( state.result.rollback), { "--rollback"}, "")
               }( common::string::split( openinfo));
            }
            catch( ...)
            {
               log::line( log, "failed to parse mockup openinfo: ", openinfo, " - ", exception::capture());
            }


            if( ! state.transactions.all.empty())
            {
               log::line( log::category::error, "xa_open_entry - rmid: ", rmid, " has associated transactions ", state.transactions.all);
               return state.error( code::xa::protocol);
            }

            log::line( log, "xa_open_entry - openinfo: ", openinfo, " rmid: ", rmid, " flags: ", flags);
            return cast::underlying( state.result.open);
         }
         int xa_close_entry( const char* closeinfo, int rmid, long flags)
         {
            auto& state = local::state( rmid, rm::State::Invoke::xa_close_entry);

            if( ! state.transactions.all.empty())
            {
               log::line( log::category::error, "xa_close_entry - rmid: ", rmid, " has associated transactions ", state.transactions.all);
               return state.error( code::xa::protocol);
            }
            log::line( log, "xa_close_entry - closeinfo: ", closeinfo, " rmid: ", rmid, " flags: ", flags);

            return cast::underlying( state.result.close);
         }
         int xa_start_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID trid{ *xid};
            log::line( log, "xa_start_entry - trid: ", trid, " rmid: ", rmid, " flags: ", flags);

            auto& state = local::state( rmid, rm::State::Invoke::xa_start_entry);

            if( state.transactions.current)
            {
               log::line( log::category::error, "XAER_PROTO: xa_start_entry - a transaction is active - ", state.transactions.current);
               return state.error( code::xa::protocol);
            }

            if( state.result.start != code::xa::ok)
               return cast::underlying( state.result.start);

            auto found = algorithm::find( state.transactions.all, trid);

            if( ! found)
            {
               state.transactions.all.emplace_back( trid);
            }
            else
            {
               if( ! common::has::flag< TMRESUME>( flags))
               {
                  log::line( log::category::error, "XAER_PROTO: xa_start_entry - the transaction is suspended, but no TMRESUME in flags - ", state.transactions.current);
                  return state.error( code::xa::protocol);
               }
            }

            state.transactions.current = trid;

            return cast::underlying( state.result.start);
         }

         int xa_end_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID trid{ *xid};
            log::line( log, "xa_end_entry - xid: ", trid, " rmid: ", rmid, " flags: ", flags);

            auto& state = local::state( rmid, rm::State::Invoke::xa_end_entry);

            if( state.transactions.current != trid)
            {
               log::line( log::category::error, "XAER_NOTA: xa_end_entry - transaction not associated with RM");
               return state.error( code::xa::invalid_xid);
            }

            state.transactions.current = transaction::ID{};

            if( ! common::has::flag< TMSUSPEND>( flags))
            {
               if( auto found = algorithm::find( state.transactions.all, trid))
                  state.transactions.all.erase( std::begin( found));
            }
            return cast::underlying( state.result.end);
         }

         int xa_rollback_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log::line( log, "xa_rollback_entry - xid: ", transaction, " rmid: ", rmid, " flags: ", flags);

            auto& state = local::state( rmid, rm::State::Invoke::xa_rollback_entry);

            return cast::underlying( state.result.rollback);
         }

         int xa_prepare_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log::line( log, "xa_prepare_entry - xid: ", transaction, " rmid: ", rmid, " flags: ", flags);

            auto& state = local::state( rmid, rm::State::Invoke::xa_prepare_entry);

            return cast::underlying( state.result.prepare);
         }

         int xa_commit_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log::line( log, "xa_commit_entry - xid: ", transaction, " rmid: ", rmid, " flags: ", flags);

            auto& state = local::state( rmid, rm::State::Invoke::xa_commit_entry);

            if( state.result.commit == code::xa::ok)
            {
               if( state.transactions.current == transaction)
                  state.transactions.current = transaction::ID{};

               auto found = algorithm::find( state.transactions.all, transaction);

               if( found)
               {
                  state.transactions.all.erase( std::begin( found));
               }
            }

            return cast::underlying( state.result.commit);
         }

         int xa_recover_entry( XID* xid, long count, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log::line( log, "xa_recover_entry - xid: ", transaction, " count: ", count, " rmid: ", rmid, " flags: ", flags);


            return 0;
         }

         int xa_forget_entry( XID* xid, int rmid, long flags)
         {
            transaction::ID transaction{ *xid};
            log::line( log, "xa_forget_entry - xid: ", transaction, " rmid: ", rmid, " flags: ", flags);

            return XA_OK;
         }

         int xa_complete_entry( int* handle, int* retval, int rmid, long flags)
         {
            log::line( log, "xa_complete_entry - handle:", handle, " retval: ", retval, " rmid: ", rmid, " flags: ", flags);

            return XA_OK;
         }


      } // unittest
   } // common
} // casual


extern "C"
{
   struct xa_switch_t casual_mockup_xa_switch_static{
      "casual mockup static XA",
      TMNOMIGRATE,
      0,
      &casual::common::unittest::xa_open_entry,
      &casual::common::unittest::xa_close_entry,
      &casual::common::unittest::xa_start_entry,
      &casual::common::unittest::xa_end_entry,
      &casual::common::unittest::xa_rollback_entry,
      &casual::common::unittest::xa_prepare_entry,
      &casual::common::unittest::xa_commit_entry,
      &casual::common::unittest::xa_recover_entry,
      &casual::common::unittest::xa_forget_entry,
      &casual::common::unittest::xa_complete_entry
   };

   struct xa_switch_t casual_mockup_xa_switch_dynamic{
      "casual mockup dynamic XA",
      TMNOMIGRATE | TMREGISTER,
      0,
      &casual::common::unittest::xa_open_entry,
      &casual::common::unittest::xa_close_entry,
      &casual::common::unittest::xa_start_entry,
      &casual::common::unittest::xa_end_entry,
      &casual::common::unittest::xa_rollback_entry,
      &casual::common::unittest::xa_prepare_entry,
      &casual::common::unittest::xa_commit_entry,
      &casual::common::unittest::xa_recover_entry,
      &casual::common::unittest::xa_forget_entry,
      &casual::common::unittest::xa_complete_entry
   };
} // extern "C"




