//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/transaction/context.h"

#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/environment.h"
#include "common/environment/normalize.h"
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

// std
#include <map>
#include <algorithm>

namespace casual
{
   namespace common
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

         struct Trace : common::log::Trace
         {
            template< typename T>
            Trace( T&& value) : common::log::Trace( std::forward< T>( value), log::category::transaction) {}
         };

         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }

         Context::Context()
         {
            // initialize the views
            std::tie( m_resources.dynamic, m_resources.fixed) =
               algorithm::partition( m_resources.all, []( auto& r){ return r.dynamic();});
         }

         Context::~Context()
         {
            if( ! m_transactions.empty())
               log::line( log::stream::get( "error"), code::casual::invalid_semantics, " transactions not consumed: ", m_transactions.size());
         }

         Context& Context::clear()
         {
            instance() = Context{};
            return instance();
         }

         bool Context::empty() const
         {
            return m_transactions.empty();
         }

         Transaction& Context::current()
         {
            if( m_transactions.empty() || m_transactions.back().suspended())
               return m_empty;

            return m_transactions.back();
         }

         bool Context::associated( const strong::correlation::id& correlation)
         {
            return algorithm::any_of( m_transactions, [&correlation]( auto& transaction)
            {
               return transaction.state == Transaction::State::active && transaction.associated( correlation);
            });
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

                     message::transaction::configuration::alias::Request request{ process::handle()};
                     request.alias = instance::alias();
                     request.resources = std::move( names);

                     return common::environment::normalize( 
                        communication::ipc::call( communication::instance::outbound::transaction::manager::device(), request)).resources;
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

            auto names = algorithm::transform( named, 
               []( auto& resource){ return resource.name;});

            auto configuration = local::resource::configuration( std::move( names));

            auto transform_resource = [&configuration]( auto&& predicate)
            {
               return [&configuration, predicate = std::move( predicate)]( auto& resource)
               {  
                  auto is_configuration = [&]( auto& configuration){ return predicate( resource, configuration);};

                  if( auto found = algorithm::find_if( configuration, is_configuration))
                  {
                     return Resource{
                        resource,
                        found->id,
                        found->openinfo,
                        found->closeinfo};
                  }

                  event::error::raise( code::casual::invalid_configuration, "missing configuration for linked named RM: ", resource, " - check domain configuration");
               };
            };
               
            // take care of named resources
            algorithm::transform( named, m_resources.all, transform_resource( []( auto& l, auto& r){ return l.name == r.name;}));
            
            // take care of unnamed resources
            algorithm::transform( unnamed, m_resources.all, transform_resource( []( auto& l, auto& r){ return l.key == r.key;}));


            // create the views
            std::tie( m_resources.dynamic, m_resources.fixed) =
               algorithm::partition( m_resources.all, []( auto& r){ return r.dynamic();});

            log::line( log::category::transaction, "static resources: ", m_resources.fixed);
            log::line( log::category::transaction, "dynamic resources: ", m_resources.dynamic);

            // Open the resources...
            // TODO semantics: Not sure if we can do this, or if users has to call tx_open by them self...
            open();

         }


         bool Context::pending() const
         {
            const auto process = process::handle();

            return ! algorithm::find_if( m_transactions, [&]( const Transaction& transaction){
               return ! transaction.trid.null() && transaction.trid.owner() == process;
            }).empty();
         }

         namespace local
         {
            namespace
            {
               namespace pop
               {
                  struct Guard
                  {
                     Guard( std::vector< Transaction>& transactions) : m_transactions( transactions) {}
                     ~Guard() { m_transactions.erase( std::end( m_transactions));}

                     std::vector< Transaction>& m_transactions;
                  };
               } // pop

               namespace start
               {
                  Transaction transaction( const platform::time::point::type& start, platform::time::unit timeout)
                  {
                     Transaction transaction{ common::transaction::id::create( process::handle())};
                     transaction.state = Transaction::State::active;
                     if( timeout > platform::time::unit{})
                        transaction.deadline = platform::time::clock::type::now() + timeout;

                     return transaction;
                  }
               } // start

               auto raise_if_not_ok = []( auto code, auto&& context)
               {
                  if( code != decltype( code)::ok)
                     code::raise::error( code, context);
               };

               namespace resources::start
               {
                  namespace check
                  {
                     struct Result
                     {
                        transaction::Resource* resource{};
                        code::xa code = code::xa::ok;
                     };

                     void results( const transaction::ID& trid, std::vector< Result> results)
                     {
                        auto [ failed, succeeded] = algorithm::partition( results, []( auto& result){ return result.code != code::xa::ok;});

                        if( ! failed)
                           return;

                        auto xa_end = [&trid]( auto& result)
                        {
                           result.resource->end( trid, flag::xa::Flag::success);
                        };

                        // xa_end on the succeeded.
                        algorithm::for_each( succeeded, xa_end);

                        code::raise::error( range::front( failed).code, "resource start failed: ", algorithm::transform( failed, []( auto& fail)
                        { 
                           return std::make_tuple( fail.resource->id(), fail.code);
                        }));
                     };
                     
                  } // check

                  namespace involved
                  {
                     auto future( const transaction::ID& trid, std::vector< strong::resource::id> resources)
                     {
                        Trace trace{ "transaction::local::resources::involved::future"};

                        message::transaction::resource::involved::Request message;
                        message.process = process::handle();
                        message.trid = trid;
                        message.involved = std::move( resources);

                        log::line( log::category::transaction, "involved message: ", message);

                        return communication::device::async::call( communication::instance::outbound::transaction::manager::device(), message);
                     } 
                  } // involved

                  namespace transform
                  {
                     template< typename R>
                     auto ids( R&& resources)
                     {
                        return algorithm::transform( resources, []( auto& resource){ return resource.id();});
                     }
                  } // transform

                  template< typename P, typename R>
                  void invoke( P&& policy, Transaction& transaction, R&& resources)
                  {
                     Trace trace{ "transaction::local::resources::start"};

                     if( resources.empty())
                        return; // nothing to do...

                     policy( transaction, resources);
                  }

                  namespace policy
                  {
                     struct Join
                     {
                        //! When a service invocation joins a transaction
                        template< typename R>
                        auto operator() ( Transaction& transaction, R&& resources)
                        {
                           // We absolutely know that this is a distributed transaction, since we've been invoked
                           // with a transaction that we shall join...
                           // We need to correlate with TM if we've going to use join our not.
                           auto future = involved::future( transaction.trid, transform::ids( resources));

                           auto start_functor = []( auto& trid, auto&& future)
                           {
                              return [ &trid, involved = std::move( future.get( communication::ipc::inbound::device()).involved)]( auto& resource)
                              {
                                 if( algorithm::find( involved, resource.id()))
                                    return check::Result{ &resource, resource.start( trid, flag::xa::Flag::join)};
                                 else
                                    return check::Result{ &resource, resource.start( trid, flag::xa::Flag::no_flags)};
                              };
                           };

                           check::results( transaction.trid, algorithm::transform( resources, start_functor( transaction.trid, future)));

                           // involve all static resources
                           transaction.involve( transform::ids( resources));
                        }
                     };

                     //! When a service invocation starts a transaction
                     struct Start
                     {
                        template< typename R>
                        auto operator() ( Transaction& transaction, R&& resources)
                        {
                           auto start_functor = []( auto& trid)
                           {
                              return [ &trid]( auto& resource)
                              {
                                 return check::Result{ &resource, resource.start( trid, flag::xa::Flag::no_flags)};
                              };
                           };

                           check::results( transaction.trid, algorithm::transform( resources, start_functor( transaction.trid)));

                           // involve all static resources
                           transaction.involve( transform::ids( resources));
                        }
                     };

                     struct Resume
                     {
                        template< typename R>
                        auto operator() ( Transaction& transaction, R&& resources)
                        {
                           auto resume = []( auto& trid)
                           {
                              return [ &trid]( auto& resource)
                              {
                                 return check::Result{ &resource, resource.start( trid, flag::xa::Flag::resume)};
                              };
                           };

                           check::results( transaction.trid, algorithm::transform( resources, resume( transaction.trid)));

                           // involve all static resources. This is probably not needed...
                           transaction.involve( transform::ids( resources));
                        }

                     };
                     
                  } // policy
               } // resources::start

               namespace resources::end
               {
                  template< typename P, typename R>
                  void invoke( P&& policy, const Transaction& transaction, R& resources)
                  {
                     Trace trace{ "transaction::local::resources::start"};
                     log::line( verbose::log, "transaction: ", transaction, ", resources: ", resources);

                     if( resources.empty())
                        return; // nothing to do...

                     policy( transaction, resources);
                  }

                  namespace policy
                  {
                     template< typename R>
                     auto end( const Transaction& transaction, R& resources, flag::xa::Flags flags)
                     {  
                        for( auto id : transaction.involved())
                           if( auto found = algorithm::find( resources, id))
                              found->end( transaction.trid, flags);

                     }

                     struct Suspend
                     {
                        template< typename R>
                        auto operator() ( const Transaction& transaction, R& resources)
                        {
                           policy::end( transaction, resources, flag::xa::Flag::suspend);
                        }
                     };

                     struct Success
                     {
                        template< typename R>
                        auto operator() ( const Transaction& transaction, R& resources)
                        {
                           policy::end( transaction, resources, flag::xa::Flag::success);
                        }
                     };

                  } // policy
                  
               } // resources::end

            } // <unnamed>
         } // local

         std::vector< strong::resource::id> Context::resources() const
         {
            return algorithm::transform( m_resources.all, []( auto& resource){ return resource.id();});
         }

         Transaction& Context::join( const transaction::ID& trid)
         {
            Trace trace{ "transaction::Context::join"};

            auto& transaction = m_transactions.emplace_back( trid);

            if( trid)
               local::resources::start::invoke( local::resources::start::policy::Join{}, transaction, m_resources.fixed);

            return transaction;
         }

         Transaction& Context::start( const platform::time::point::type& start)
         {
            Trace trace{ "transaction::Context::start"};

            auto transaction = local::start::transaction( start, m_timeout);

            local::resources::start::invoke( local::resources::start::policy::Start{}, transaction, m_resources.fixed);

            m_transactions.push_back( std::move( transaction));
            return m_transactions.back();
         }

         Transaction& Context::branch( const transaction::ID& trid)
         {
            Trace trace{ "transaction::Context::branch"};

            auto& transaction = m_transactions.emplace_back( id::branch( trid));

            if( transaction)
               local::resources::start::invoke( local::resources::start::policy::Start{}, transaction, m_resources.fixed);

            return transaction;
         }


         void Context::update( message::service::call::Reply& reply)
         {
            Trace trace{ "transaction::Context::update"};

            if( reply.transaction.trid)
            {
               auto found = algorithm::find( m_transactions, reply.transaction.trid);

               if( ! found)
                  code::raise::error( code::xatmi::system, "failed to find transaction: ", reply.transaction);

               auto& transaction = *found;

               auto state = Transaction::State( reply.transaction.state);

               if( transaction.state < state)
                  transaction.state = state;

               // this descriptor is done, and we can remove the association to the transaction
               transaction.replied( reply.correlation);

               log::line( log::category::transaction, "updated state: ", transaction);
            }
            else
            {
               // TODO: if call was made in transaction but the service has 'none', the
               // trid is not replied, but the descriptor is still associated with the transaction.
               // Don't know what is the best way to solve this, but for now we just go through all
               // transaction and discard the descriptor, just in case.
               // TODO: Look this over when we redesign 'call/transaction-context'
               for( auto& transaction : m_transactions)
                  transaction.replied( reply.correlation);
            }
         }

         message::service::Transaction Context::finalize( bool commit)
         {
            Trace trace{ "transaction::Context::finalize"};

            // Regardless, we will consume every transaction.
            auto transactions = std::exchange( m_transactions, {});

            log::line( log::category::transaction, "transactions: ", transactions);

            const auto process = process::handle();

            message::service::Transaction result;
            result.trid = std::move( caller);
            result.state = message::service::Transaction::State::active;

            auto pending_check = [&]( Transaction& transaction)
            {
               if( transaction.pending())
               {
                  if( transaction.trid)
                  {
                     log::line( log::category::error, "pending replies associated with transaction - action: discard pending and set transaction state to rollback only");
                     log::line( log::category::transaction, transaction);

                     transaction.state = Transaction::State::rollback;
                     result.state = message::service::Transaction::State::error;
                  }

                  // Discard pending
                  algorithm::for_each( transaction.correlations(), []( const auto& correlation){ 
                     communication::ipc::inbound::device().discard( correlation);
                  });
               }
            };


            auto trans_rollback = [&]( const Transaction& transaction)
            {
               try
               {
                  Context::rollback( transaction);
               }
               catch( ...)
               {
                  result.state = message::service::Transaction::State::error;
                  log::line( log::category::error, exception::capture(), " failed to rollback transaction: ", transaction.trid);
               }
            };

            auto trans_commit_rollback = [&]( const Transaction& transaction)
            {
               if( commit && transaction.state == Transaction::State::active)
               {
                  try
                  {
                    Context::commit( transaction); 
                  }
                  catch( ...)
                  {
                     result.state = message::service::Transaction::State::error;
                     log::line( log::category::error, exception::capture(), " failed to commit transaction: ", transaction.trid);
                  } 
               }
               else
               {
                  trans_rollback( transaction);
               }
            };


            // Check pending calls
            algorithm::for_each( transactions, pending_check);

            // Ignore 'null trid':s
            auto actual_transactions = std::get< 0>( algorithm::stable::partition( transactions, [&]( const Transaction& transaction){
               return ! transaction.trid.null();
            }));


            auto owner_split = algorithm::stable::partition( actual_transactions, [&]( const Transaction& transaction){
               return transaction.trid.owner() != process;
            });

            // take care of owned transactions
            {
               auto owner = std::get< 1>( owner_split);
               algorithm::for_each( owner, trans_commit_rollback);
            }

            // Take care of not-owned transaction(s)
            {
               auto not_owner = std::get< 0>( owner_split);

               // should be 0..1
               assert( not_owner.size() <= 1);

               auto transform_state = []( Transaction::State state){
                  switch( state)
                  {
                     case Transaction::State::active: return message::service::Transaction::State::active;
                     case Transaction::State::rollback: return message::service::Transaction::State::rollback;
                     case Transaction::State::timeout: return message::service::Transaction::State::timeout;
                     default: return message::service::Transaction::State::error;
                  }
               };

               if( not_owner)
               {
                  log::line( log::category::transaction, "not_owner: ", *not_owner);

                  result.state = transform_state( not_owner->state);

                  if( ! commit && result.state == message::service::Transaction::State::active)
                     result.state = message::service::Transaction::State::rollback;

                  // end resource
                  local::resources::end::invoke( local::resources::end::policy::Success{}, *not_owner, m_resources.all);                  
               }


               return result;
            }
         }

         code::ax Context::resource_registration( strong::resource::id rmid, XID* xid)
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
            *xid = transaction.trid.xid;

            if( transaction)
            {
               // if the rm is already involved vi use join directly
               if( algorithm::find( transaction.involved(), rmid))
                  return code::ax::join;

               transaction.involve( rmid);

               // if local, we know that this rm has never been associated with this transaction.
               if( transaction.local())
                  return code::ax::ok;

               // we need to correlate with TM
               auto reply = local::resources::start::involved::future( transaction.trid, { rmid}).get( communication::ipc::inbound::device());
               return algorithm::find( reply.involved, rmid).empty() ? code::ax::ok : code::ax::join;
            }

            return code::ax::ok;
         }

         void Context::resource_unregistration( strong::resource::id rmid)
         {
            Trace trace{ "transaction::Context::resource_unregistration"};

            auto&& transaction = current();

            // RM:s can only unregister if we're outside global
            // transactions, and the rm is registered before
            if( transaction.trid || ! transaction.disassociate_dynamic( rmid))
               code::raise::error( code::ax::protocol, "resource id: ", rmid);
         }


         void Context::begin()
         {
            Trace trace{ "transaction::Context::begin"};

            auto&& transaction = current();

            if( transaction.trid)
            {
               if( m_control != Control::stacked)
                  code::raise::error( code::tx::protocol, "begin - already in transaction mode - ", transaction);

               // Tell the RM:s to suspend
               local::resources::end::invoke( local::resources::end::policy::Suspend{}, transaction, m_resources.all);

            }
            else if( ! transaction.dynamic().empty())
               code::raise::error( code::tx::outside,  "begin - dynamic resources not done with work outside global transaction");

            auto trans = local::start::transaction( platform::time::clock::type::now(), m_timeout);

            // We know we've got a local transaction.
            {
               auto resource_start = [&trans]( auto& resource)
               {
                  // involve the resource in the transaction
                  trans.involve( resource.id());
                  return algorithm::compare::any( resource.start( trans.trid, flag::xa::Flag::no_flags), code::xa::ok, code::xa::read_only);
               };
               auto [ successfull, failed] = algorithm::partition( m_resources.fixed, resource_start);

               if( failed)
               {
                  // some of the resources failed, make sure we xa_end the successfull ones...
                  algorithm::for_each( successfull, [ &trans]( auto& resource)
                  {
                     // TODO semantics: is it ok to end with success? Seams better than 
                     // to mark this extremely short lived transaction with 'error'. Don't know
                     // how resources "wants it"...
                     resource.end( trans.trid, flag::xa::Flag::success);
                  });
                  
                  code::raise::error( code::tx::error, "some resources failed to start: ", algorithm::transform( failed, []( auto& rm){ return rm.id();}));
               }
            }            

            m_transactions.push_back( std::move( trans));

            log::line( log::category::transaction, "transaction: ", m_transactions.back().trid, " started");
         }


         void Context::open()
         {
            Trace trace{ "transaction::Context::open"};

            // XA spec: if one, or more of resources opens ok, then it's not an error...
            //   seams really strange not to notify user that some of the resources has
            //   failed to open...

            auto results = algorithm::transform( m_resources.all, []( auto& resource)
            {
               return resource.open();
            });

            if( ! algorithm::all_of( results, []( auto r){ return r == code::xa::ok;}))
               log::line( log::category::error, code::xa::resource_error, "failed to open one or more resource");
         }

         void Context::close()
         {
            Trace trace{ "transaction::Context::close"};

            auto results = algorithm::transform( m_resources.all, []( auto& resource)
            {
               return resource.close();
            });

            if( ! algorithm::all_of( results, []( auto r){ return r == code::xa::ok;}))
               log::line( log::category::error, code::xa::resource_error, "failed to close one or more resource");
         }


         void Context::commit( const Transaction& transaction)
         {
            Trace trace{ "transaction::Context::commit - transaction"};

            const auto process = process::handle();

            if( ! transaction.trid)
               code::raise::error( code::tx::no_begin, "commit - no ongoing transaction");

            if( transaction.trid.owner() != process)
               code::raise::error( code::tx::protocol, "commit - not owner of transaction: ", transaction.trid);

            if( transaction.state != Transaction::State::active)
               code::raise::error( code::tx::protocol, "commit - transaction is in rollback only mode - ", transaction.trid);

            if( transaction.pending())
               code::raise::error( code::tx::protocol, "commit - pending replies associated with transaction: ", transaction.trid);

            // end resources
            local::resources::end::invoke( local::resources::end::policy::Success{}, transaction, m_resources.all);


            if( transaction.local() && transaction.involved().size() <= 1)
            {
               Trace trace{ "transaction::Context::commit - local"};

               // transaction is local, and at most one resource is involved.
               // We do the commit directly against the resource (if any).
               // TODO: we could do a two-phase-commit local if the transaction is 'local'

               if( ! transaction.involved().empty())
                  return resource_commit( transaction.involved().front(), transaction, flag::xa::Flag::one_phase);

               // No resources associated to this transaction, hence the commit is successful.
               return;
            }
            else
            {
               Trace trace{ "transaction::Context::commit - distributed"};

               message::transaction::commit::Request request{ process::handle()};
               request.trid = transaction.trid;
               request.involved = transaction.involved();

               auto reply = communication::ipc::call( communication::instance::outbound::transaction::manager::device(), request);
               log::line( log::category::transaction, "message: ", reply);

               switch( reply.stage)
               {
                  using State = decltype( reply.stage);

                  case State::prepare:
                  {
                     log::line( log::category::transaction, "prepare reply: ", reply.state);

                     switch( m_commit_return)
                     {
                        using Enum = decltype( m_commit_return);
                        case Enum::logged:
                        {
                           log::line( log::category::transaction, "decision logged directive");

                           // Discard the coming commit-message
                           communication::ipc::inbound::device().discard( reply.correlation);
                           break;
                        }
                        case Enum::completed:
                        {
                           // Wait for the commit
                           communication::device::blocking::receive( communication::ipc::inbound::device(), reply, reply.correlation);

                           log::line( log::category::transaction, "commit reply: ", reply.state);
                           break;
                        }
                     }

                     break;
                  }
                  case State::commit:
                  {
                     log::line( log::category::transaction, "commit reply: ", reply.state);
                     break;
                  }
                  case State::rollback:
                  {
                     pop_transaction();
                     code::raise::error( code::tx::rollback, "commit failed - transaction rollback");
                     break;
                  }
               }

               local::raise_if_not_ok( reply.state, "during commit");
            }
         }

         void Context::commit()
         {
            Trace trace{ "transaction::Context::commit"};

            commit( current());

            // We only remove/consume transaction if commit succeed
            // TODO: any other situation we should remove?
            pop_transaction();
         }

         void Context::rollback( const Transaction& transaction)
         {
            Trace trace{ "transaction::Context::rollback"};

            log::line( log::category::transaction, "transaction: ", transaction);

            if( ! transaction)
               code::raise::error( code::tx::protocol, "no ongoing transaction");

            const auto process = process::handle();

            if( transaction.trid.owner() != process)
               code::raise::error( code::tx::protocol, "current process not owner of transaction: ", transaction.trid);

            // end resources
            local::resources::end::invoke( local::resources::end::policy::Success{}, transaction, m_resources.all);

            if( transaction.local())
            {
               log::line( log::category::transaction, "rollback is local");

               auto involved = transaction.involved();
               algorithm::transform( m_resources.fixed, involved, []( auto& r){ return r.id();});

               algorithm::for_each( algorithm::unique( algorithm::sort( involved)), [&]( auto id)
               {
                  resource_rollback( id, transaction);
               });
            }
            else 
            {
               log::line( log::category::transaction, "rollback is distributed");

               message::transaction::rollback::Request request;
               request.trid = transaction.trid;
               request.process = process;
               algorithm::append( transaction.involved(), request.involved);

               auto reply = communication::ipc::call( communication::instance::outbound::transaction::manager::device(), request);

               log::line( log::category::transaction, "rollback reply tx: ", reply.state);

               local::raise_if_not_ok( reply.state, "during rollback");
            }
         }

         void Context::rollback()
         {
            rollback( current());
            pop_transaction();
         }



         void Context::set_commit_return( commit::Return value) noexcept
         {
            log::line( verbose::log, "set_commit_return: ", value);
            m_commit_return = value;
         }

         commit::Return Context::get_commit_return() const noexcept
         {
            return m_commit_return;
         }

         void Context::set_transaction_control( transaction::Control control)
         {
            log::line( verbose::log, "set_transaction_control: ", control);
            m_control = control;
         }

         void Context::set_transaction_timeout( platform::time::unit timeout)
         {
            if( timeout < platform::time::unit{})
               code::raise::error( code::tx::argument, "timeout value has to be 0 or greater");

            m_timeout = timeout;
         }

         bool Context::info( TXINFO* info)
         {
            auto&& transaction = current();

            if( info)
            {
               info->xid = transaction.trid.xid;
               info->transaction_state = static_cast< decltype( info->transaction_state)>( transaction.state);
               info->transaction_timeout = std::chrono::duration_cast< std::chrono::seconds>( m_timeout).count();
               info->transaction_control = cast::underlying( m_control);
            }
            return static_cast< bool>( transaction);
         }


         void Context::suspend( XID* xid)
         {
            Trace trace{ "transaction::Context::suspend"};

            if( xid == nullptr)
               code::raise::error( code::tx::argument, "suspend: argument xid is null");

            auto& ongoing = current();

            if( id::null( ongoing.trid))
               code::raise::error( code::tx::protocol, "suspend: attempt to suspend a null xid");

            // We don't check if current transaction is aborted. This differs from Tuxedo's semantics

            // mark the transaction as suspended
            ongoing.suspend();

            // Tell the RM:s to suspend
            local::resources::end::invoke( local::resources::end::policy::Suspend{}, ongoing, m_resources.all);

            *xid = ongoing.trid.xid;
         }



         void Context::resume( const XID* xid)
         {
            Trace trace{ "transaction::Context::resume"};

            if( xid == nullptr)
               code::raise::error( code::tx::argument, "resume: argument xid is null");

            if( xid::null( *xid))
               code::raise::error( code::tx::argument, "resume: attempt to resume a 'null xid'");

            auto& ongoing = current();

            if( ongoing.trid && ! ongoing.suspended())
               code::raise::error( code::tx::protocol, "resume: ongoing transaction is active");

            if( ! ongoing.dynamic().empty())
               code::raise::error( code::tx::outside, "resume: ongoing work outside global transaction: ", ongoing);

            auto found = algorithm::find( m_transactions, *xid);

            if( ! found)
               code::raise::error( code::tx::argument, "resume: transaction not known");

            if( ! found->suspended())
               code::raise::error( code::tx::protocol, "resume: wanted transaction is not suspended");


            // All precondition are met, let's set wanted transaction as current
            {
               found->resume();

               // Tell the RM:s to resume
               local::resources::start::invoke( local::resources::start::policy::Resume{}, *found, m_resources.fixed);

               // We rotate the wanted to end;
               algorithm::rotate( m_transactions, ++found);
            }
         }

         void Context::resources_resume( Transaction& transaction)
         {
            Trace trace{ "transaction::Context::resources_start"};

            if( ! transaction)
               return; // nothing to do

            local::resources::start::invoke( local::resources::start::policy::Resume{}, transaction, m_resources.fixed);
         }

         void Context::resources_suspend( Transaction& transaction)
         {
            Trace trace{ "transaction::Context::resources_suspend"};

            if( ! transaction)
               return;

            // Tell the RM:s to suspend
            local::resources::end::invoke( local::resources::end::policy::Suspend{}, transaction, m_resources.all);      
         }

         void Context::resource_commit( strong::resource::id rm, const Transaction& transaction, flag::xa::Flags flags)
         {
            Trace trace{ "transaction::Context::resources_commit"};
            log::line( log::category::transaction, "transaction: ", transaction, " - rm: ", rm, " - flags: ", flags);

            if( auto found = algorithm::find( m_resources.all, rm))
            {
               auto code = common::code::convert::to::tx( found->commit( transaction.trid, flags));
               local::raise_if_not_ok( code, "resource commit");
            }
            else
               code::raise::error( code::tx::error, "resource id not known - rm: ", rm, " transaction: ", transaction);
         }

         void Context::resource_rollback( strong::resource::id rm, const Transaction& transaction)
         {
            Trace trace{ "transaction::Context::resource_rollback"};
            log::line( log::category::transaction, "transaction: ", transaction, " - rm: ", rm);

            if( auto found = algorithm::find( m_resources.all, rm))
            {
               auto code = common::code::convert::to::tx( found->rollback( transaction.trid, flag::xa::Flag::no_flags));
               local::raise_if_not_ok( code, "resource rollback");
            }
            else
               code::raise::error( code::tx::error, "resource id not known - rm: ", rm, " transaction: ", transaction);
         }

         void Context::pop_transaction()
         {
            Trace trace{ "transaction::Context::pop_transaction"};

            // pop the current transaction
            m_transactions.pop_back();

            // Dependent on control we do different stuff
            switch( m_control)
            {
               case Control::unchained:
               {
                  // no op
                  break;
               }
               case Control::chained:
               {
                  // We start a new one
                  begin();
                  break;
               }
               case Control::stacked:
               {
                  if( auto& current = Context::current())
                  {
                     // Tell the RM:s to resume
                     local::resources::start::invoke( local::resources::start::policy::Resume{}, current, m_resources.all);
                  }

                  break;
               }
               default:
               {
                  code::raise::error( code::tx::fail, "unknown control directive - this can not happen");
               }
            }
         }

      } // transaction
   } // common
} //casual


