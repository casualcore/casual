//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "transaction/context.h"
#include "transaction/common.h"

#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/environment.h"
#include "common/environment/normalize.h"
#include "common/execution/context.h"
#include "common/process.h"
#include "common/instance.h"
#include "common/log.h"
#include "common/algorithm.h"
#include "common/code/raise.h"
#include "common/code/tx.h"
#include "common/code/xa.h"
#include "common/code/casual.h"
#include "common/code/convert.h"
#include "common/message/transaction.h"
#include "common/event/send.h"

#include "casual/overloaded.h"
#include "casual/assert.h"

#include <map>
#include <algorithm>

namespace casual
{

   namespace transaction
   {   
      std::string_view description( Control value) noexcept
      {
         switch( value)
         {
            case Control::unchained: return "unchained";
            case Control::chained: return "chained";
            case Control::stacked: return "stacked";
         }
         return "<unknown>";
      };

      namespace commit
      {
         std::string_view description( Return value) noexcept
         {
            switch( value)
            {
               case Return::completed: return "completed";
               case Return::logged: return "logged";
            }
            return "<unknown>";
         }

      } // commit

      Context& Context::instance()
      {
         static Context singleton;
         return singleton;
      }

      Context::Context()
      {
         // initialize the views
         std::tie( m_resources.dynamic, m_resources.fixed) =
            common::algorithm::partition( m_resources.all, []( auto& r){ return r.dynamic();});
      }

      Context::~Context()
      {
         if( ! m_transactions.empty())
            common::log::line( common::log::stream::get( "error"), code::casual::invalid_semantics, " transactions not consumed: ", m_transactions.size());
      }

      Context& Context::clear()
      {
         instance() = Context{};
         return instance();
      }

      bool Context::empty() const noexcept
      {
         return m_transactions.empty();
      }

      Transaction& Context::current()
      {
         if( m_transactions.empty() || m_transactions.back().suspended())
            return m_empty;

         return m_transactions.back();
      }

      bool Context::associated( const common::strong::correlation::id& correlation)
      {
         return common::algorithm::any_of( m_transactions, [&correlation]( auto& transaction)
         {
            return transaction.state == Transaction::State::active && transaction.associated( correlation);
         });
      }

      std::vector< common::strong::correlation::id> Context::associated() const
      {
         std::vector< common::strong::correlation::id> result;

         for( const auto& transaction : m_transactions)
            common::algorithm::container::append( transaction.correlations(), result);

         return result;
      }

      namespace local
      {
         namespace
         {
            namespace resource
            {
               auto configuration( std::vector< std::string> names)
               {
                  Trace trace{ "transaction::local::resource::configuration"};

                  common::message::transaction::configuration::alias::Request request{ common::process::handle()};
                  request.alias = common::instance::alias();
                  request.resources = std::move( names);

                  auto result = common::environment::normalize( 
                     common::communication::ipc::call( common::communication::instance::outbound::transaction::manager::device(), request)).resources;

                  common::log::debug( "result: ", result);

                  return result;
               }

            } // resource
         } // <unnamed>
      } // local

      void Context::configure( std::vector< resource::Link> resources)
      {
         Trace trace{ "transaction::Context::configure"};

         if( resources.empty())
            return;

         // there are different semantics if the resource has a specific name
         // if so, we strictly correlate to that name, if not we go with the more general key

         auto [ named, unnamed] = common::algorithm::stable::partition( resources, []( auto& r){ return ! r.name.empty();});

         auto transform_name = []( auto& resource){ return resource.name;};

         auto configuration = local::resource::configuration( common::algorithm::transform( named, transform_name));

         auto transform_resource = [ &configuration]( auto predicate)
         {
            return [ &configuration, predicate = std::move( predicate)]( auto& resource)
            {  
               auto is_configuration = [&]( auto& configuration){ return predicate( resource, configuration);};

               if( auto found = common::algorithm::find_if( configuration, is_configuration))
               {
                  // consume the resource configuration (make it impossible to reuse it...)
                  auto value = common::algorithm::container::extract( configuration, std::begin( found));

                  return Resource{
                     resource,
                     value.id,
                     std::move( value.openinfo),
                     std::move( value.closeinfo)};
               }

               common::event::error::raise( code::casual::invalid_configuration, "missing configuration for linked named RM: ", resource, " - check domain configuration");
            };
         };
            
         // take care of named resources
         common::algorithm::transform( named, m_resources.all, transform_resource( []( auto& l, auto& r){ return l.name == r.name;}));
         
         // take care of unnamed resources
         common::algorithm::transform( unnamed, m_resources.all, transform_resource( []( auto& l, auto& r){ return l.key == r.key;}));


         // create the views
         std::tie( m_resources.dynamic, m_resources.fixed) =
            common::algorithm::partition( m_resources.all, []( auto& r){ return r.dynamic();});

         log::line( "static resources: ", m_resources.fixed);
         log::line( "dynamic resources: ", m_resources.dynamic);

         // Open the resources...
         // TODO semantics: Not sure if we can do this, or if users has to call tx_open by them self...
         if( auto code = open(); code != code::tx::ok)
            code::raise::error( code, "failed to open during configure");

      }


      bool Context::pending() const
      {
         const auto process = common::process::handle();

         return ! common::algorithm::find_if( m_transactions, [&]( const Transaction& transaction){
            return ! transaction.trid.null() && transaction.trid.owner() == process;
         }).empty();
      }

      namespace local
      {
         namespace
         {

            namespace accumulate
            {
               template< typename F>
               auto code( F functor)
               {
                  return [ functor = std::move( functor)]( code::tx code, auto&& value)
                  {
                     return code + functor( value);
                  };
               }
            } // accumulate

            namespace start
            {
               Transaction transaction()
               {
                  Transaction transaction{ common::transaction::id::create( common::process::id())};
                  transaction.state = Transaction::State::active;
                  return transaction;
               }

               Transaction transaction( common::chronology::duration timeout)
               {
                  auto transaction = start::transaction();

                  if( timeout > common::chronology::duration{})
                     transaction.deadline = platform::time::clock::type::now() + timeout;

                  return transaction;
               }
            } // start
            

            namespace resources::transform
            {
               template< typename R>
               auto ids( R&& resources)
               {
                  return common::algorithm::transform( resources, []( auto& resource)
                  { 
                     using resource_type = std::remove_cvref_t< decltype( resource)>;
                     return std::invoke( &resource_type::id, resource);
                  });
               }

            } // resources::transform

            namespace raise
            {
               //! raise error if not tx::ok
               template< typename... Ts>
               void code( code::tx code, Ts&&... ts)
               {
                  if( code != code::tx::ok)
                     code::raise::error( code, std::forward< Ts>( ts)...);
               }
            } // raise

            namespace resources
            {
               //! invoke `xa_function` for all resources **involved** in `transaction` and return the accumulated code::tx result. 
               //! The `flags` is passed to all invocations of `xa_function`
               [[nodiscard]] common::code::tx invoke( const Transaction& transaction, std::span< Resource> resources, auto xa_function, common::flag::xa::Flag flags)
               {
                  return common::algorithm::accumulate( transaction.involved(), common::code::tx::ok, [ &transaction, resources, xa_function, flags]( auto code, auto id)
                  {
                     if( auto found = common::algorithm::find( resources, id))
                        return code + code::convert::to::tx( std::invoke( xa_function, *found, transaction.trid, flags));

                     return code;
                  });
               }
            } // resources

            namespace resources::start
            {
               namespace involved
               {
                  auto synchronize( const transaction::ID& trid, std::vector< common::strong::resource::id> resources)
                  {
                     Trace trace{ "transaction::local::resources::involved::synchronize"};

                     common::message::transaction::resource::involved::Request message;
                     message.process = common::process::handle();
                     message.trid = trid;
                     message.involved = std::move( resources);

                     log::line( "involved message: ", message);

                     return common::communication::ipc::call( common::communication::instance::outbound::transaction::manager::device(), message).involved;
                  }

                  void send( const transaction::ID& trid, std::vector< common::strong::resource::id> resources)
                  {
                     Trace trace{ "transaction::local::resources::involved::send"};

                     common::message::transaction::resource::involved::Request message;
                     message.process = common::process::handle();
                     message.trid = trid;
                     message.involved = std::move( resources);
                     message.reply = false;

                     log::line( "involved send-and-forget message: ", message);

                     common::communication::device::blocking::send( common::communication::instance::outbound::transaction::manager::device(), message);
                  }

               } // involved


               [[nodiscard]] common::code::tx invoke( Transaction& transaction, std::span< Resource> resources, common::flag::xa::Flag flags)
               {
                  Trace trace{ "transaction::local::resources::start::invoke"};
                  common::log::debug( "transaction: ", transaction, ", resources: ", resources, ", flags: ", flags);

                  transaction.involve( transform::ids( resources));

                  return resources::invoke( transaction, resources, &Resource::start, flags);
               }

               //! When a service invocation joins a transaction
               [[nodiscard]] common::code::tx join( Transaction& transaction, std::span< Resource> resources)
               {
                  Trace trace{ "transaction::local::resources::start::join"};
                  common::log::debug( "transaction: ", transaction, ", resources: ", resources);

                  if( std::empty( resources))
                     return common::code::tx::ok; // nothing to do...

                  // We absolutely know that this is a distributed transaction, since we've been invoked
                  // with a transaction that we shall join...
                  // We need to correlate with TM if we've going to use join our not.
                  auto involved = involved::synchronize( transaction.trid, transform::ids( resources));

                  // involve all resources
                  transaction.involve( transform::ids( resources));

                  return common::algorithm::accumulate( resources, common::code::tx::ok, [ &transaction, &involved]( auto code, auto& resource)
                  {
                     auto flag = std::ranges::contains( involved, resource.id()) ? common::flag::xa::Flag::join : common::flag::xa::Flag::no_flags;

                     return code + code::convert::to::tx( resource.start( transaction.trid, flag));
                  });
               }

               [[nodiscard]] common::code::tx branch( Transaction& transaction, std::span< Resource> resources)
               {
                  Trace trace{ "transaction::local::resources::start::branch"};
                  common::log::debug( "transaction: ", transaction, ", resources: ", resources);

                  if( std::empty( resources))
                     return common::code::tx::ok; // nothing to do...

                  // We absolutely know that this is a distributed transaction (non-local gtrid)
                  // we let the TM know about our resources
                  involved::send( transaction.trid, transform::ids( resources));

                  return invoke( transaction, resources, common::flag::xa::Flag::no_flags);
               }

               [[nodiscard]] common::code::tx resume( Transaction& transaction, std::span< Resource> resources)
               {
                  Trace trace{ "transaction::local::resources::start::resume"};
                  common::log::debug( "transaction: ", transaction, ", resources: ", resources);

                  return invoke( transaction, resources, common::flag::xa::Flag::resume);
               }

            } // resources::start

            namespace resources::end
            {

               [[nodiscard]] common::code::tx invoke( const Transaction& transaction, std::span< Resource> resources, common::flag::xa::Flag flags)
               {
                  Trace trace{ "transaction::local::resources::end::invoke"};
                  common::log::debug( "transaction: ", transaction, ", resources: ", resources, ", flags: ", flags);

                  return resources::invoke( transaction, resources, &Resource::end, flags);
               }
               
            } // resources::end

         } // <unnamed>
      } // local

      std::vector< common::strong::resource::id> Context::resources() const noexcept
      {
         return common::algorithm::transform( m_resources.all, []( auto& resource){ return resource.id();});
      }

      Transaction& Context::join( const transaction::ID& trid)
      {
         Trace trace{ "transaction::Context::join"};

         auto& transaction = m_transactions.emplace_back( trid);
         update_execution_context();

         if( ! trid)
            return transaction;

         local::raise::code( local::resources::start::join( transaction, m_resources.fixed),
            "failed to join one or more fixed resources");

         log::event( "join", trid);

         return transaction;
      }

      Transaction& Context::start()
      {
         Trace trace{ "transaction::Context::start"};

         auto transaction = local::start::transaction();

          local::raise::code( local::resources::start::invoke( transaction, m_resources.fixed, common::flag::xa::Flag::no_flags),
            "failed to start one or more fixed resources");

         log::event( "start", transaction.trid);

         m_transactions.push_back( std::move( transaction));
         update_execution_context();

         return m_transactions.back();
      }

      Transaction& Context::branch( const transaction::ID& trid)
      {
         Trace trace{ "transaction::Context::branch"};

         auto& transaction = m_transactions.emplace_back( id::branch( trid));
         update_execution_context();

         if( transaction)
            local::raise::code( local::resources::start::branch( transaction, m_resources.fixed),
               "failed to branch one or more fixed resources");


         log::event( "branch", transaction.trid);

         return transaction;
      }


      void Context::update( common::message::service::call::Reply& reply)
      {
         Trace trace{ "transaction::Context::update"};

         if( auto found = common::algorithm::find( m_transactions, reply.correlation))
         {
            auto state = Transaction::State( reply.transaction_state);

            if( found->state < state)
               found->state = state;

            // this descriptor is done, and we can remove the association to the transaction
            found->replied( reply.correlation);
         }
         // TODO:
         // We seem to propagate transaction state even if the call was not in a transaction
         // (callee started a transaction). This does not really matter, but we should fix it.
         //
         //else if( reply.transaction_state != decltype( reply.transaction_state)::ok)
         //{
         //   code::raise::error( code::tx::protocol,
         //      "reply with transaction state: ", reply.transaction_state, " and no associated transaction");
         //}
      }

      common::message::service::transaction::State Context::finalize( bool commit)
      {
         Trace trace{ "transaction::Context::finalize"};

         // Regardless, we will consume every transaction.
         auto transactions = std::exchange( m_transactions, {});
         update_execution_context();

         log::line( "transactions: ", transactions);

         auto result = common::message::service::transaction::State::ok;

         auto pending_check = [&]( Transaction& transaction)
         {
            if( transaction.pending())
            {
               if( transaction.trid)
               {
                  common::log::line( common::log::category::error, "pending replies associated with transaction - action: discard pending and set transaction state to rollback only");
                  log::line( transaction);

                  result = common::message::service::transaction::State::error;
               }

               // Discard pending
               common::algorithm::for_each( transaction.correlations(), []( const auto& correlation){ 
                  common::communication::ipc::inbound::device().discard( correlation);
               });
            }
         };

         auto invoke_rollback = [ this]( const Transaction& transaction)
         {
            return log::code( Context::rollback( transaction), "rollback trid: ", transaction.trid);
         };

         auto invoke_commit_rollback = [ this, invoke_rollback, commit]( const Transaction& transaction)
         {
            if( commit && transaction.state == Transaction::State::active)
               return log::code( Context::commit( transaction), "commit trid: ", transaction.trid);
            else
               return invoke_rollback( transaction);
         };

         auto transform_state = casual::overloaded{
            []( Transaction::State state)
            {
               using State = common::message::service::transaction::State;
               switch( state)
               {
                  case Transaction::State::active: return State::ok;
                  case Transaction::State::rollback: return State::rollback;
                  case Transaction::State::timeout: return State::timeout;
               }
               return State::error;
            },
            []( code::tx code)
            {
               using State = common::message::service::transaction::State;
               switch( code)
               {
                  case code::tx::ok: return State::ok;
                  case code::tx::rollback: return State::rollback;
                  default: return State::error;
               }
            }};


         // Check pending calls
         common::algorithm::for_each( transactions, pending_check);

         // Ignore 'null trid':s
         auto filter_active = []( auto& transactions)
         {
            return common::algorithm::stable::filter( transactions, []( auto& transaction){ return common::predicate::boolean( transaction);});
         };

         auto [ not_owner, owner] = common::algorithm::stable::partition( filter_active( transactions), []( auto& transaction)
         {
            return transaction.trid.owner() != common::process::handle();
         });

         // take care of owned transactions
         auto code = common::algorithm::accumulate( owner, code::tx::ok, local::accumulate::code( invoke_commit_rollback));

         // Take care of not-owned transaction(s) ( even if owned failed in some way, we need to "consume" not-owned)
         {
            // should be 0..1
            assert( not_owner.size() <= 1);

            if( not_owner)
            {
               log::line( "not_owner: ", *not_owner);

               result += transform_state( not_owner->state);

               if( ! commit && result == decltype( result)::ok)
                  result += decltype( result)::rollback;

               // end resource
               code += local::resources::end::invoke( *not_owner, m_resources.all, common::flag::xa::Flag::success);
            }
         }

         result += transform_state( code);

         log::line( "result: ", result);
         log::event( "finalize", result);

         return result;
      }

      code::ax Context::resource_registration( common::strong::resource::id rmid, XID* xid)
      {
         Trace trace{ "transaction::Context::resource_registration"};

         // Verify that rmid is known and is dynamic
         if( ! common::algorithm::find( m_resources.dynamic, rmid))
            code::raise::error( code::ax::argument, "resource id: ", rmid);
         
         auto& transaction = current();

         // XA-spec - RM can't reg when it's already regged... Why?
         // We'll interpret this as the transaction has been suspended, and
         // then resumed.
         if( ! transaction.associate_dynamic( rmid))
            code::raise::error( code::ax::resume, "resource id: ", rmid);

         
         // Let the resource know the xid (if any)
         *xid = transaction.trid.to_xid();

         if( transaction)
         {
            // if the rm is already involved vi use join directly
            if( common::algorithm::find( transaction.involved(), rmid))
               return code::ax::join;

            transaction.involve( rmid);

            // if local, we know that this rm has never been associated with this transaction.
            if( transaction.local())
               return code::ax::ok;

            // we need to correlate with TM
            auto involved = local::resources::start::involved::synchronize( transaction.trid, { rmid});
            return common::algorithm::find( involved, rmid).empty() ? code::ax::ok : code::ax::join;
         }

         return code::ax::ok;
      }

      code::ax Context::resource_unregistration( common::strong::resource::id rmid)
      {
         Trace trace{ "transaction::Context::resource_unregistration"};

         auto& transaction = current();

         // RM:s can only unregister if we're outside global
         // transactions, and the rm is registered before
         if( transaction.trid || ! transaction.disassociate_dynamic( rmid))
            return log::code( code::ax::protocol, "resource id: ", rmid);

         return code::ax::ok;
      }


      code::tx Context::begin()
      {
         Trace trace{ "transaction::Context::begin"};

         // check "precondition" based on current 
         {
            auto& transaction = current();

            if( transaction.trid)
            {
               if( m_control != Control::stacked)
                  return log::code( code::tx::protocol, "begin - already in transaction mode - ", transaction);

               // Tell the RM:s to suspend
               if( auto code = local::resources::end::invoke( transaction, m_resources.all, common::flag::xa::Flag::suspend); code != code::tx::ok)
                  return log::code( code, "failed to suspend one or more resources - ", transaction);
            }
            else if( ! transaction.dynamic().empty())
               return code::tx::outside;
         }

         auto transaction = local::start::transaction( std::exchange( m_timeout, {}));

         // We know we've got a local transaction.
         {
            auto resource_start = [ &transaction]( auto& resource)
            {
               // involve the resource in the transaction
               transaction.involve( resource.id());
               return common::algorithm::compare::any( resource.start( transaction.trid, common::flag::xa::Flag::no_flags), code::xa::ok, code::xa::read_only);
            };
            auto [ successful, failed] = common::algorithm::partition( m_resources.fixed, resource_start);

            if( failed)
            {
               // some of the resources failed, make sure we xa_end the successful ones...
               common::algorithm::for_each( successful, [ &transaction]( auto& resource)
               {
                  // TODO semantics: is it ok to end with success? Seams better than 
                  // to mark this extremely short lived transaction with 'error'. Don't know
                  // how resources "wants it"...
                  resource.end( transaction.trid, common::flag::xa::Flag::success);
               });

               return log::code( code::tx::error, "some resources failed to start: ", common::algorithm::transform( failed, []( auto& rm){ return rm.id();}));
            }
         }            

         m_transactions.push_back( std::move( transaction));
         update_execution_context();

         log::event( "begin", m_transactions.back().trid);

         return code::tx::ok;
      }


      code::tx Context::open()
      {
         Trace trace{ "transaction::Context::open"};

         // XA spec: if one, or more of resources opens ok, then it's not an error...
         //   seams really strange not to notify user that some of the resources has
         //   failed to open...

         return log::code( common::algorithm::accumulate( m_resources.all, code::tx::ok, local::accumulate::code( []( auto& resource)
         {
            return code::convert::to::tx( resource.open());
         })), "failed to open one or more resource");

      }

      code::tx Context::close()
      {
         Trace trace{ "transaction::Context::close"};

         return log::code( common::algorithm::accumulate( m_resources.all, code::tx::ok, local::accumulate::code( []( auto& resource)
         {
            return code::convert::to::tx( resource.close());
         })), "failed to close one or more resource");
      }

      namespace local
      {
         namespace
         {
            namespace precondition
            {
               code::tx commit( const Transaction& transaction)
               {
                  Trace trace{ "transaction::local::precondition::commit"};

                  if( ! transaction.trid)
                     return log::code( code::tx::protocol, "commit - no ongoing transaction");

                  if( transaction.trid.owner() != common::process::handle())
                     return log::code( code::tx::protocol, "commit - not owner of transaction: ", transaction.trid);

                  if( transaction.state != Transaction::State::active)
                     return log::code( code::tx::protocol, "commit - transaction is in rollback only mode - ", transaction.trid);

                  if( transaction.pending())
                     return log::code( code::tx::protocol, "commit - pending replies associated with transaction: ", transaction.trid);

                  return code::tx::ok;
               }

               code::tx rollback( const Transaction& transaction)
               {
                  Trace trace{ "transaction::local::precondition::rollback"};

                  if( ! transaction)
                     return log::code( code::tx::protocol, "rollback - no ongoing transaction");

                  if( transaction.trid.owner() != common::process::handle())
                     return log::code( code::tx::protocol, "rollback - not owner of transaction: ", transaction.trid);

                  // TODO can we do a rollback with pending replies? I think so...
                  //if( transaction.pending())
                  //  return log::code( code::tx::protocol, "rollback - pending replies associated with transaction: ", transaction.trid);

                  return code::tx::ok;
               }
            } // precondition
         } // <unnamed>
      } // local


      code::tx Context::commit( const Transaction& transaction)
      {
         Trace trace{ "transaction::Context::commit - transaction"};
         casual::assertion( transaction, "not a valid transaction: ", transaction);

         // end resources with success.
         auto code = local::resources::end::invoke( transaction, m_resources.all, common::flag::xa::Flag::success);
         log::line( "code: ", code);

         // if we succeeded to end resources, and the transaction is local, and at most one resource is involved,
         // we can do the commit directly against the resource (if any).
         if( common::code::success( code) && transaction.local() && transaction.involved().size() <= 1)
         {
            Trace trace{ "transaction::Context::commit - local"};

            // transaction is local, and at most one resource is involved.
            // We do the commit directly against the resource (if any).
            // TODO: we could do a two-phase-commit local if the transaction is 'local'

            log::event( "commit", transaction.trid, transaction.involved());

            // we know that we have [0..1] resources, so we can use the _invoke function_ to do the commit, even if it's a bit of an overkill. 
            // If we have no resource, this will just return ok.
            return log::code( local::resources::invoke( transaction, m_resources.all, &Resource::commit, common::flag::xa::Flag::one_phase),
               "commit, trid: ", transaction.trid);
         }
         // otherwise, we let the TM take care of the commit.
         else
         {
            Trace trace{ "transaction::Context::commit - distributed"};

            common::message::transaction::commit::Request request{ common::process::handle()};
            request.trid = transaction.trid;
            request.involved = transaction.involved();

            auto reply = common::communication::ipc::call( common::communication::instance::outbound::transaction::manager::device(), request);
            log::line( "message: ", reply);

            switch( reply.stage)
            {
               using Stage = decltype( reply.stage);

               case Stage::prepare:
               {
                  log::line( "commit - stage prepare - state: ", reply.state);

                  switch( m_commit_return)
                  {
                     using Enum = decltype( m_commit_return);
                     case Enum::logged:
                     {
                        log::line( "decision logged directive");

                        // Discard the coming commit-message
                        common::communication::ipc::inbound::device().discard( reply.correlation);
                        break;
                     }
                     case Enum::completed:
                     {
                        // Wait for the commit
                        common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply, reply.correlation);

                        log::line( "commit reply: ", reply.state);
                        break;
                     }
                  }

                  break;
               }
               case Stage::commit:
                  log::event( "commit", "distributed", transaction.trid, reply.state);
                  return log::code( reply.state, "commit - stage commit");
               case Stage::rollback:
                  log::event( "commit", "distributed", transaction.trid, reply.state);
                  return log::code( code::tx::rollback, "commit - stage rollback - state: ", reply.state);
            }

            log::event( "commit", "distributed", transaction.trid, reply.state);
            return log::code( reply.state, "commit, trid: ", transaction.trid);
         }
      }

      code::tx Context::commit()
      {
         Trace trace{ "transaction::Context::commit"};

         if( auto code = local::precondition::commit( current()); code != code::tx::ok)
            return code;

         // we know that we got an _active_ transaction that passed the precondition.
         // we consume the transaction, regardless...
         auto transaction = common::algorithm::container::extract( m_transactions, std::prev( std::end( m_transactions)));
         update_execution_context();

         return control_continuation( commit( transaction));
      }

      code::tx Context::rollback( const Transaction& transaction)
      {
         Trace trace{ "transaction::Context::rollback"};
         casual::assertion( transaction, "not a valid transaction: ", transaction); 

         // end resources
         auto code = local::resources::end::invoke( transaction, m_resources.all, common::flag::xa::Flag::success);

         // if one or more resource failed to end, we let TM take care of the rollback.
         // For example if rm returns XA_RB* error, it means that the resource has marked 
         // the transaction as rollback only, and has dissociated the transaction from this thread
         // of control. 
         // In this case, we can't do a rollback against the resource in this thread of control.
         // See XA state tables (Chapter 6), documentation/xa/state-tables.md.

         if( common::code::success( code) && transaction.local())
         {
            auto result = local::resources::invoke( transaction, m_resources.all, &Resource::rollback, common::flag::xa::Flag::no_flags);

            log::event( "rollback", "local", transaction.trid, result);

            return log::code( result, "rollback, trid: ", transaction.trid);
         }
         else 
         {
            common::message::transaction::rollback::Request request{ common::process::handle()};
            request.trid = transaction.trid;
            request.involved = transaction.involved();

            auto reply = common::communication::ipc::call( common::communication::instance::outbound::transaction::manager::device(), request);

            log::event( "rollback", "distributed", transaction.trid, reply.state, transaction.involved());

            return log::code( reply.state, "rollback, trid: ", transaction.trid);
         }
      }

      code::tx Context::rollback()
      {
         if( auto code = local::precondition::rollback( current()); code != code::tx::ok)
            return code;

         // we know that we got an _active_ transaction that passed the precondition.
         // we consume the transaction, regardless...
         auto transaction = common::algorithm::container::extract( m_transactions, std::prev( std::end( m_transactions)));
         update_execution_context();

         return control_continuation( rollback( transaction));
      }

      code::tx Context::set_commit_return( commit::Return value) noexcept
      {
         common::log::debug( "set_commit_return: ", value);
         m_commit_return = value;

         return code::tx::ok;
      }

      commit::Return Context::get_commit_return() const noexcept
      {
         return m_commit_return;
      }

      code::tx Context::set_transaction_control( transaction::Control control)
      {
         common::log::debug( "set_transaction_control: ", control);
         m_control = control;
         return code::tx::ok;
      }

      code::tx Context::set_transaction_timeout( common::chronology::duration timeout)
      {
         if( timeout < common::chronology::duration{})
            return log::code( code::tx::argument, "set_transaction_timeout - timeout value has to be 0 or greater");

         m_timeout = timeout;
         return code::tx::ok;
      }

      bool Context::info( TXINFO* info)
      {
         auto&& transaction = current();

         if( info)
         {
            info->xid = transaction.trid.to_xid();
            info->transaction_state = static_cast< decltype( info->transaction_state)>( transaction.state);
            info->transaction_timeout = std::chrono::duration_cast< std::chrono::seconds>( m_timeout).count();
            info->transaction_control = std::to_underlying( m_control);
         }
         return common::predicate::boolean( transaction);
      }

      namespace local
      {
         namespace
         {
            namespace precondition
            {
               auto suspend( const XID* xid, const Transaction& current)
               {
                  if( xid == nullptr)
                     return log::code( code::tx::argument, "suspend: argument xid is null");

                  if( current.trid.null())
                     return log::code( code::tx::protocol, "suspend: attempt to suspend a null xid");

                  return code::tx::ok;
               }

               auto resume( const XID* xid, const Transaction& current)
               {
                  if( xid == nullptr)
                     return log::code( code::tx::argument, "resume: argument xid is null");

                  if( common::transaction::xid::null( *xid))
                     return log::code( code::tx::argument, "resume: attempt to resume a 'null xid'");

                  if( current.trid && ! current.suspended())
                     return log::code( code::tx::protocol, "resume: ongoing transaction is active");

                  if( ! current.dynamic().empty())
                     return log::code( code::tx::outside, "resume: ongoing work outside global transaction: ", current);

                  return code::tx::ok;
               }
            } // precondition
         } // <unnamed>
      } // local

      code::tx Context::suspend( XID* xid)
      {
         Trace trace{ "transaction::Context::suspend"};

         if( auto code = local::precondition::suspend( xid, current()); code != code::tx::ok)
            return code;

         auto& ongoing = current();

         // We don't check if current transaction is aborted. This differs from Tuxedo's semantics

         // mark the transaction as suspended
         ongoing.suspend();
         update_execution_context();

         // tell the RM:s to suspend
         auto code = local::resources::end::invoke( ongoing, m_resources.all, common::flag::xa::Flag::suspend);
         log::line( "suspend code: ", code);

         if( ! common::code::success( code))
            return code;

         *xid = ongoing.trid.to_xid();

         log::event( "suspend", ongoing.trid);

         return code::tx::ok;
      }

      code::tx Context::resume( const XID* xid)
      {
         Trace trace{ "transaction::Context::resume"};

         if( auto code = local::precondition::resume( xid, current()); code != code::tx::ok)
            return code;

         if( auto found = common::algorithm::find( m_transactions, *xid))
         {
            if( ! found->suspended())
               return log::code( code::tx::protocol, "resume: wanted transaction is not suspended");

            found->resume();

            // Tell the RM:s to resume
            if( auto code = local::resources::start::resume( *found, m_resources.fixed); code != code::tx::ok)
               return log::code( code, "failed to resume one or more fixed resources");

            // We rotate the wanted to end;
            common::algorithm::rotate( m_transactions, ++found);
            update_execution_context();

            log::event( "resume", current().trid);

            return code::tx::ok;
         }
         else
            return log::code( code::tx::argument, "resume: transaction not known - xid: ", *xid);

      }

      void Context::resources_resume( Transaction& transaction)
      {
         Trace trace{ "transaction::Context::resources_start"};

         if( ! transaction)
            return; // nothing to do

         local::raise::code( local::resources::start::resume( transaction, m_resources.fixed),
            "failed to resume one or more fixed resources");
      }

      void Context::resources_suspend( Transaction& transaction)
      {
         Trace trace{ "transaction::Context::resources_suspend"};

         if( ! transaction)
            return;

         // Tell the RM:s to suspend
         local::raise::code( local::resources::end::invoke( transaction, m_resources.all, common::flag::xa::Flag::suspend),
            "failed to suspend one or more resources");
      }


      code::tx Context::control_continuation( code::tx code)
      {
         Trace trace{ "transaction::Context::control_continuation"};
         common::log::debug( "code: ", code);

         // Dependent on control we do different stuff
         switch( m_control)
         {
            case Control::unchained:
               // no op
               return code; 
               
            case Control::chained:
               // We start a new one
               return code + begin();

            case Control::stacked:
               // Tell the RM:s to resume, if we've got a transaction
               if( auto& current = Context::current())
                  return code + local::resources::start::resume( current, m_resources.fixed);

               return code;
         }

         casual::terminate( code::casual::internal_unexpected_value, "unknown control directive: ", std::to_underlying( m_control), " - this can not happen");
      }

      void Context::update_execution_context()
      {
         common::execution::context::trid::set( current().trid);

      }

   } // transaction
} //casual


