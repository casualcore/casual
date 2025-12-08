//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#define CASUAL_NO_XATMI_UNDEFINE

#include "transaction/unittest/rm.h"
#include "transaction/context.h"


#include "casual/argument.h"
#include "common/code/xa.h"

#include "common/transaction/transaction.h"
#include "common/log.h"

#include "common/flag.h"
#include "common/exception/capture.h"
#include "common/algorithm/compare.h"
#include "common/chronology.h"



namespace casual
{
   namespace transaction::unittest::rm
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
                  common::code::xa open = common::code::xa::ok;
                  common::code::xa close = common::code::xa::ok;
                  common::code::xa start = common::code::xa::ok;
                  common::code::xa end = common::code::xa::ok;
                  common::code::xa prepare = common::code::xa::ok;
                  common::code::xa commit = common::code::xa::ok;
                  common::code::xa rollback= common::code::xa::ok;

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

               auto error( common::code::xa code)
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
               std::map< common::strong::resource::id, State> state;
            } // global

            namespace state
            {
               auto& get( common::strong::resource::id id, rm::state::Invoke invoked)
               {
                  auto& state = global::state[ id];
                  state.id = id;
                  state.invocations.push_back( invoked);
                  common::log::debug( "state ", state);
                  return state;
               }

               auto& clear( common::strong::resource::id id)
               {
                  auto& state = global::state[ id];
                  state = {};
                  state.id = id;
                  common::log::debug( "cleared state ", state);
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

         const State& get( common::strong::resource::id id)
         {
            return local::global::state.at( id);
         }

         void clear()
         {
            local::global::state.clear();
         }
      } // state


      void registration( common::strong::resource::id id)
      {
         auto& state = local::global::state.at( id);

         ::XID xid{};

         common::log::debug( "resource registration: ", transaction::context().resource_registration( id, &xid));

         state.transactions.current = common::transaction::ID{ xid };

      }

      int xa_open_entry( const char* c_openinfo, int rmid, long f)
      {
         common::Trace trace{ "xa_open_entry"};
         auto id = common::strong::resource::id{ rmid};
         auto openinfo = std::string{ c_openinfo};
         auto flags = common::flag::xa::Flag{ f};

         common::log::debug( "id: ", id, ", openinfo: ", openinfo, " flags: ", flags);

         // clear
         auto& state = local::state::clear( id);
         state.invocations.push_back( rm::state::Invoke::xa_open_entry);

         try
         {
            auto parse_result = []( common::code::xa& code)
            {
               return [&code]( const std::string& value) { 
                  code = common::code::xa{ common::string::from< int>( value)};
               };
            };

            auto sleep_option = []( auto& duration)
            {
               return [ &duration]( const std::string& value)
               {
                  duration = common::chronology::from::string( value);
               };
            };

            argument::parse( "mockup rm", {
               argument::Option( parse_result( state.result.open), {{ "--open"}}, ""),
               argument::Option( parse_result( state.result.close), {{ "--close"}}, ""),
               argument::Option( parse_result( state.result.start), {{ "--start"}}, ""),
               argument::Option( parse_result( state.result.end), {{ "--end"}}, ""),
               argument::Option( parse_result( state.result.prepare), {{ "--prepare"}}, ""),
               argument::Option( parse_result( state.result.commit), {{ "--commit"}}, ""),
               argument::Option( parse_result( state.result.rollback), {{ "--rollback"}}, ""),
               argument::Option( sleep_option( state.sleep_prepare), {{ "--sleep-prepare"}}, ""),
               argument::Option( sleep_option( state.sleep_commit), {{ "--sleep-commit"}}, ""),
               argument::Option( sleep_option( state.sleep_rollback), {{ "--sleep-rollback"}}, "")
            }, common::string::split( openinfo));
         }
         catch( ...)
         {
            common::log::debug( "failed to parse mockup openinfo: ", openinfo, " - ", common::exception::capture());
         }


         if( ! state.transactions.all.empty())
         {
            common::log::error( common::code::xa::resource_error, "xa_open_entry - id: ", id, " has associated transactions ", state.transactions.all);
            return state.error( common::code::xa::protocol);
         }

         common::log::debug( "xa_open_entry - openinfo: ", openinfo, ", id: ", id, " flags: ", flags);
         return std::to_underlying( state.result.open);
      }

      int xa_close_entry( const char* closeinfo, int rmid, long f)
      {
         common::Trace trace{ "xa_close_entry"};
         auto id = common::strong::resource::id{ rmid};
         auto flags = common::flag::xa::Flag{ f};

         common::log::debug( "id: ", id, " flags: ", flags);

         auto& state = local::state::get( id, rm::state::Invoke::xa_close_entry);

         if( ! state.transactions.all.empty())
         {
            common::log::error( common::code::xa::resource_error, "xa_close_entry - id: ", id, " has associated transactions ", state.transactions.all);
            return state.error( common::code::xa::protocol);
         }
         common::log::debug( "closeinfo: ", closeinfo, " id: ", id, " flags: ", flags);

         return std::to_underlying( state.result.close);
      }

      int xa_start_entry( XID* xid, int rmid, long f)
      {
         common::Trace trace{ "xa_start_entry"};
         auto id = common::strong::resource::id{ rmid};
         auto flags = common::flag::xa::Flag{ f};

         transaction::ID trid{ *xid};
         common::log::debug( "trid: ", trid, " id: ", id, " flags: ", flags);

         auto& state = local::state::get( id, rm::state::Invoke::xa_start_entry);

         if( state.transactions.current)
         {
            common::log::error( common::code::xa::resource_error, common::code::xa::protocol, ": xa_start_entry - a transaction is active - ", state.transactions.current);
            return state.error( common::code::xa::protocol);
         }

         if( state.result.start != common::code::xa::ok)
            return std::to_underlying( state.result.start);

         auto found = common::algorithm::find( state.transactions.all, trid);

         if( ! found)
         {
            state.transactions.all.emplace_back( trid);
         }
         else
         {
            if( ! common::flag::contains( flags, common::flag::xa::Flag::resume))
            {
               common::log::error( common::code::xa::resource_error, "XAER_PROTO: xa_start_entry - the transaction is suspended, but no TMRESUME in flags - ", state.transactions.current);
               return state.error( common::code::xa::protocol);
            }
         }

         state.transactions.current = trid;

         return std::to_underlying( state.result.start);
      }

      int xa_end_entry( XID* xid, int rmid, long f)
      {
         common::Trace trace{ "xa_end_entry"};
         auto id = common::strong::resource::id{ rmid};
         auto flags = common::flag::xa::Flag{ f};
         
         transaction::ID trid{ *xid};
         common::log::debug( "xid: ", trid, " id: ", id, " flags: ", flags);

         auto& state = local::state::get( id, rm::state::Invoke::xa_end_entry);

         constexpr auto required = common::flag::xa::Flag::success | common::flag::xa::Flag::suspend | common::flag::xa::Flag::fail;

         if( common::flag::count( flags & required) != 1)
         {
            common::log::error( common::code::xa::resource_error, common::code::xa::protocol, ": xa_end_entry - xa_end flags has to have exactly one of: ", required, ", flags: ", common::flag::xa::Flag{ flags});
            return state.error( common::code::xa::protocol);
         }

         if( state.transactions.current != trid)
         {
            common::log::error( common::code::xa::resource_error, common::code::xa::invalid_xid, ": xa_end_entry - transaction not associated with RM");
            return state.error( common::code::xa::invalid_xid);
         }

         state.transactions.current = transaction::ID{};

         if( ! common::flag::contains( flags, common::flag::xa::Flag::suspend))
         {
            if( auto found = common::algorithm::find( state.transactions.all, trid))
               state.transactions.all.erase( std::begin( found));
         }
         return std::to_underlying( state.result.end);
      }

      int xa_rollback_entry( XID* xid, int rmid, long f)
      {
         common::Trace trace{ "xa_rollback_entry"};
         auto id = common::strong::resource::id{ rmid};
         auto flags = common::flag::xa::Flag{ f};

         transaction::ID transaction{ *xid};
         common::log::debug( "xid: ", transaction, " id: ", id, " flags: ", flags);

         auto& state = local::state::get( id, rm::state::Invoke::xa_rollback_entry);

         if( state.sleep_rollback)
            common::process::sleep( *state.sleep_rollback);

         return std::to_underlying( state.result.rollback);
      }

      int xa_prepare_entry( XID* xid, int rmid, long f)
      {
         common::Trace trace{ "xa_prepare_entry"};
         auto id = common::strong::resource::id{ rmid};
         auto flags = common::flag::xa::Flag{ f};

         transaction::ID transaction{ *xid};
         common::log::debug( "xid: ", transaction, " id: ", id, " flags: ", flags);

         auto& state = local::state::get( id, rm::state::Invoke::xa_prepare_entry);

         if( state.sleep_prepare)
            common::process::sleep( *state.sleep_prepare);

         return std::to_underlying( state.result.prepare);
      }

      int xa_commit_entry( XID* xid, int rmid, long f)
      {
         common::Trace trace{ "xa_commit_entry"};
         auto id = common::strong::resource::id{ rmid};
         auto flags = common::flag::xa::Flag{ f};

         transaction::ID transaction{ *xid};
         common::log::debug( "xid: ", transaction, " id: ", id, " flags: ", flags);

         auto& state = local::state::get( id, rm::state::Invoke::xa_commit_entry);

         if( state.sleep_commit)
            common::process::sleep( *state.sleep_commit);

         if( state.result.commit == common::code::xa::ok)
         {
            if( state.transactions.current == transaction)
               state.transactions.current = transaction::ID{};

            auto found = common::algorithm::find( state.transactions.all, transaction);

            if( found)
               state.transactions.all.erase( std::begin( found));
         }

         return std::to_underlying( state.result.commit);
      }

      int xa_recover_entry( XID* xid, long count, int rmid, long f)
      {
         common::Trace trace{ "xa_recover_entry"};
         auto id = common::strong::resource::id{ rmid};
         auto flags = common::flag::xa::Flag{ f};

         transaction::ID transaction{ *xid};
         common::log::debug( "xid: ", transaction, " count: ", count, " id: ", id, " flags: ", flags);


         return 0;
      }

      int xa_forget_entry( XID* xid, int rmid, long f)
      {
         common::Trace trace{ "xa_forget_entry"};
         auto id = common::strong::resource::id{ rmid};
         auto flags = common::flag::xa::Flag{ f};

         transaction::ID transaction{ *xid};
         common::log::debug( "xid: ", transaction, " id: ", id, " flags: ", flags);

         return XA_OK;
      }

      int xa_complete_entry( int* handle, int* retval, int rmid, long f)
      {
         common::Trace trace{ "xa_complete_entry"};
         auto id = common::strong::resource::id{ rmid};
         auto flags = common::flag::xa::Flag{ f};

         common::log::debug( "handle:", handle, " retval: ", retval, " id: ", id, " flags: ", flags);

         return XA_OK;
      }

   } // transaction::unittest::rm
} // casual


extern "C"
{
   struct xa_switch_t casual_mockup_xa_switch_static{
      "casual mockup static XA",
      TMNOMIGRATE,
      0,
      &casual::transaction::unittest::rm::xa_open_entry,
      &casual::transaction::unittest::rm::xa_close_entry,
      &casual::transaction::unittest::rm::xa_start_entry,
      &casual::transaction::unittest::rm::xa_end_entry,
      &casual::transaction::unittest::rm::xa_rollback_entry,
      &casual::transaction::unittest::rm::xa_prepare_entry,
      &casual::transaction::unittest::rm::xa_commit_entry,
      &casual::transaction::unittest::rm::xa_recover_entry,
      &casual::transaction::unittest::rm::xa_forget_entry,
      &casual::transaction::unittest::rm::xa_complete_entry
   };

   struct xa_switch_t casual_mockup_xa_switch_dynamic{
      "casual mockup dynamic XA",
      TMNOMIGRATE | TMREGISTER,
      0,
      &casual::transaction::unittest::rm::xa_open_entry,
      &casual::transaction::unittest::rm::xa_close_entry,
      &casual::transaction::unittest::rm::xa_start_entry,
      &casual::transaction::unittest::rm::xa_end_entry,
      &casual::transaction::unittest::rm::xa_rollback_entry,
      &casual::transaction::unittest::rm::xa_prepare_entry,
      &casual::transaction::unittest::rm::xa_commit_entry,
      &casual::transaction::unittest::rm::xa_recover_entry,
      &casual::transaction::unittest::rm::xa_forget_entry,
      &casual::transaction::unittest::rm::xa_complete_entry
   };
} // extern "C"




