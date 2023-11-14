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
#include "common/algorithm/compare.h"
#include "common/chronology.h"



namespace casual
{
   namespace common::unittest::rm
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

            namespace state
            {
               struct Result
               {
                  code::xa open = code::xa::ok;
                  code::xa close = code::xa::ok;
                  code::xa start = code::xa::ok;
                  code::xa end = code::xa::ok;
                  code::xa prepare = code::xa::ok;
                  code::xa commit = code::xa::ok;
                  code::xa rollback= code::xa::ok;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( open);
                     CASUAL_SERIALIZE( close);
                     CASUAL_SERIALIZE( start);
                     CASUAL_SERIALIZE( end);
                     CASUAL_SERIALIZE( prepare);
                     CASUAL_SERIALIZE( commit);
                     CASUAL_SERIALIZE( rollback);
                  )
               };
               
            } // state

            struct State : rm::State 
            {               
               state::Result result;

               auto error( code::xa code)
               {
                  errors.push_back( code);
                  return std::to_underlying( code);
               }

               CASUAL_LOG_SERIALIZE(
                  rm::State::serialize( archive);
                  CASUAL_SERIALIZE( result);
               );
               
            };

            namespace global
            {
               std::map< strong::resource::id, State> state;
            } // global

            namespace state
            {
               auto& get( strong::resource::id id, rm::state::Invoke invoked)
               {
                  auto& state = global::state[ id];
                  state.id = id;
                  state.invocations.push_back( invoked);
                  log::line( log, "state ", state);
                  return state;
               }

               auto& clear( strong::resource::id id)
               {
                  auto& state = global::state[ id];
                  state = {};
                  state.id = id;
                  log::line( log, "cleared state ", state);
                  return state;

               }
            } // state



         } // <unnamed>
      } // local

      namespace state
      {
         std::string_view description( Invoke value) noexcept
         {
            switch( value)
            {
               case Invoke::xa_close_entry: return "xa_close_entry";
               case Invoke::xa_commit_entry: return "xa_commit_entry";
               case Invoke::xa_complete_entry: return "xa_complete_entry";
               case Invoke::xa_end_entry: return "xa_end_entry";
               case Invoke::xa_forget_entry: return "xa_forget_entry";
               case Invoke::xa_open_entry: return "xa_open_entry";
               case Invoke::xa_prepare_entry: return "xa_prepare_entry";
               case Invoke::xa_recover_entry: return "xa_recover_entry";
               case Invoke::xa_rollback_entry: return "xa_rollback_entry";
               case Invoke::xa_start_entry: return "xa_start_entry";
            }
            return "<unknown>";
         }

         const State& get( strong::resource::id id)
         {
            return local::global::state.at( id);
         }

         void clear()
         {
            local::global::state.clear();
         }
      } // state


      void registration( strong::resource::id id)
      {
         auto& state = local::global::state.at( id);

         log::line( log, "resource registration: ", transaction::context().resource_registration( id, &state.transactions.current.xid));

      }

      int xa_open_entry( const char* c_openinfo, int rmid, long flags)
      {
         common::Trace trace{ "xa_open_entry"};
         auto id = strong::resource::id{ rmid};
         auto openinfo = std::string{ c_openinfo};

         log::line( log, "id: ", id, ", openinfo: ", openinfo, " flags: ", flag::xa::Flags{ flags});

         // clear
         auto& state = local::state::clear( id);
         state.invocations.push_back( rm::state::Invoke::xa_open_entry);

         try
         {
            auto parse_result = []( code::xa& code)
            {
               return [&code]( const std::string& value) { 
                  code = code::xa{ string::from< int>( value)};
               };
            };

            auto sleep_option = []( auto& duration)
            {
               return [ &duration]( const std::string& value)
               {
                  duration = common::chronology::from::string( value);
               };
            };

            argument::Parse{ "mockup rm",
               argument::Option( parse_result( state.result.open), { "--open"}, ""),
               argument::Option( parse_result( state.result.close), { "--close"}, ""),
               argument::Option( parse_result( state.result.start), { "--start"}, ""),
               argument::Option( parse_result( state.result.end), { "--end"}, ""),
               argument::Option( parse_result( state.result.prepare), { "--prepare"}, ""),
               argument::Option( parse_result( state.result.commit), { "--commit"}, ""),
               argument::Option( parse_result( state.result.rollback), { "--rollback"}, ""),
               argument::Option( sleep_option( state.sleep_prepare), { "--sleep-prepare"}, ""),
               argument::Option( sleep_option( state.sleep_commit), { "--sleep-commit"}, ""),
               argument::Option( sleep_option( state.sleep_rollback), { "--sleep-rollback"}, "")
            }( common::string::split( openinfo));
         }
         catch( ...)
         {
            log::line( log, "failed to parse mockup openinfo: ", openinfo, " - ", exception::capture());
         }


         if( ! state.transactions.all.empty())
         {
            log::line( log::category::error, "xa_open_entry - id: ", id, " has associated transactions ", state.transactions.all);
            return state.error( code::xa::protocol);
         }

         log::line( log, "xa_open_entry - openinfo: ", openinfo, ", id: ", id, " flags: ", flags);
         return std::to_underlying( state.result.open);
      }

      int xa_close_entry( const char* closeinfo, int rmid, long flags)
      {
         common::Trace trace{ "xa_close_entry"};
         auto id = strong::resource::id{ rmid};
         log::line( log, "id: ", id, " flags: ", flag::xa::Flags{ flags});

         auto& state = local::state::get( id, rm::state::Invoke::xa_close_entry);

         if( ! state.transactions.all.empty())
         {
            log::line( log::category::error, "xa_close_entry - id: ", id, " has associated transactions ", state.transactions.all);
            return state.error( code::xa::protocol);
         }
         log::line( log, "closeinfo: ", closeinfo, " id: ", id, " flags: ", flags);

         return std::to_underlying( state.result.close);
      }

      int xa_start_entry( XID* xid, int rmid, long flags)
      {
         common::Trace trace{ "xa_start_entry"};
         auto id = strong::resource::id{ rmid};

         transaction::ID trid{ *xid};
         log::line( log, "trid: ", trid, " id: ", id, " flags: ", flag::xa::Flags{ flags});

         auto& state = local::state::get( id, rm::state::Invoke::xa_start_entry);

         if( state.transactions.current)
         {
            log::line( log::category::error, code::xa::protocol, ": xa_start_entry - a transaction is active - ", state.transactions.current);
            return state.error( code::xa::protocol);
         }

         if( state.result.start != code::xa::ok)
            return std::to_underlying( state.result.start);

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

         return std::to_underlying( state.result.start);
      }

      int xa_end_entry( XID* xid, int rmid, long flags)
      {
         common::Trace trace{ "xa_end_entry"};
         auto id = strong::resource::id{ rmid};
         
         transaction::ID trid{ *xid};
         log::line( log, "xid: ", trid, " id: ", id, " flags: ", flag::xa::Flags{ flags});

         auto& state = local::state::get( id, rm::state::Invoke::xa_end_entry);

         constexpr auto required = common::flags::compose( flag::xa::Flag::success, flag::xa::Flag::suspend, flag::xa::Flag::fail);

         if( ( flag::xa::Flags{ flags} & required).bits() != 1)
         {
            log::line( log::category::error, code::xa::protocol, ": xa_end_entry - xa_end flags has to have exactly one of: ", required, ", flags: ", flag::xa::Flags{ flags});
            return state.error( code::xa::protocol);
         }

         if( state.transactions.current != trid)
         {
            log::line( log::category::error, code::xa::invalid_xid, ": xa_end_entry - transaction not associated with RM");
            return state.error( code::xa::invalid_xid);
         }

         state.transactions.current = transaction::ID{};

         if( ! common::has::flag< TMSUSPEND>( flags))
         {
            if( auto found = algorithm::find( state.transactions.all, trid))
               state.transactions.all.erase( std::begin( found));
         }
         return std::to_underlying( state.result.end);
      }

      int xa_rollback_entry( XID* xid, int rmid, long flags)
      {
         common::Trace trace{ "xa_rollback_entry"};
         auto id = strong::resource::id{ rmid};

         transaction::ID transaction{ *xid};
         log::line( log, "xid: ", transaction, " id: ", id, " flags: ", flag::xa::Flags{ flags});

         auto& state = local::state::get( id, rm::state::Invoke::xa_rollback_entry);

         if( state.sleep_rollback)
            process::sleep( *state.sleep_rollback);

         return std::to_underlying( state.result.rollback);
      }

      int xa_prepare_entry( XID* xid, int rmid, long flags)
      {
         common::Trace trace{ "xa_prepare_entry"};
         auto id = strong::resource::id{ rmid};

         transaction::ID transaction{ *xid};
         log::line( log, "xid: ", transaction, " id: ", id, " flags: ", flag::xa::Flags{ flags});

         auto& state = local::state::get( id, rm::state::Invoke::xa_prepare_entry);

         if( state.sleep_prepare)
            process::sleep( *state.sleep_prepare);

         return std::to_underlying( state.result.prepare);
      }

      int xa_commit_entry( XID* xid, int rmid, long flags)
      {
         common::Trace trace{ "xa_commit_entry"};
         auto id = strong::resource::id{ rmid};

         transaction::ID transaction{ *xid};
         log::line( log, "xid: ", transaction, " id: ", id, " flags: ", flag::xa::Flags{ flags});

         auto& state = local::state::get( id, rm::state::Invoke::xa_commit_entry);

         if( state.sleep_commit)
            process::sleep( *state.sleep_commit);

         if( state.result.commit == code::xa::ok)
         {
            if( state.transactions.current == transaction)
               state.transactions.current = transaction::ID{};

            auto found = algorithm::find( state.transactions.all, transaction);

            if( found)
               state.transactions.all.erase( std::begin( found));
         }

         return std::to_underlying( state.result.commit);
      }

      int xa_recover_entry( XID* xid, long count, int rmid, long flags)
      {
         common::Trace trace{ "xa_recover_entry"};
         auto id = strong::resource::id{ rmid};

         transaction::ID transaction{ *xid};
         log::line( log, "xid: ", transaction, " count: ", count, " id: ", id, " flags: ", flag::xa::Flags{ flags});


         return 0;
      }

      int xa_forget_entry( XID* xid, int rmid, long flags)
      {
         common::Trace trace{ "xa_forget_entry"};
         auto id = strong::resource::id{ rmid};

         transaction::ID transaction{ *xid};
         log::line( log, "xid: ", transaction, " id: ", id, " flags: ", flag::xa::Flags{ flags});

         return XA_OK;
      }

      int xa_complete_entry( int* handle, int* retval, int rmid, long flags)
      {
         common::Trace trace{ "xa_complete_entry"};
         auto id = strong::resource::id{ rmid};

         log::line( log, "handle:", handle, " retval: ", retval, " id: ", id, " flags: ", flag::xa::Flags{ flags});

         return XA_OK;
      }

   } // common::unittest::rm
} // casual


extern "C"
{
   struct xa_switch_t casual_mockup_xa_switch_static{
      "casual mockup static XA",
      TMNOMIGRATE,
      0,
      &casual::common::unittest::rm::xa_open_entry,
      &casual::common::unittest::rm::xa_close_entry,
      &casual::common::unittest::rm::xa_start_entry,
      &casual::common::unittest::rm::xa_end_entry,
      &casual::common::unittest::rm::xa_rollback_entry,
      &casual::common::unittest::rm::xa_prepare_entry,
      &casual::common::unittest::rm::xa_commit_entry,
      &casual::common::unittest::rm::xa_recover_entry,
      &casual::common::unittest::rm::xa_forget_entry,
      &casual::common::unittest::rm::xa_complete_entry
   };

   struct xa_switch_t casual_mockup_xa_switch_dynamic{
      "casual mockup dynamic XA",
      TMNOMIGRATE | TMREGISTER,
      0,
      &casual::common::unittest::rm::xa_open_entry,
      &casual::common::unittest::rm::xa_close_entry,
      &casual::common::unittest::rm::xa_start_entry,
      &casual::common::unittest::rm::xa_end_entry,
      &casual::common::unittest::rm::xa_rollback_entry,
      &casual::common::unittest::rm::xa_prepare_entry,
      &casual::common::unittest::rm::xa_commit_entry,
      &casual::common::unittest::rm::xa_recover_entry,
      &casual::common::unittest::rm::xa_forget_entry,
      &casual::common::unittest::rm::xa_complete_entry
   };
} // extern "C"




