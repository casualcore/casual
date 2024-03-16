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

#include "casual/overloaded.h"
#include "casual/assert.h"

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

                     auto result = common::environment::normalize( 
                        communication::ipc::call( communication::instance::outbound::transaction::manager::device(), request)).resources;

                     common::log::line( verbose::log, "result: ", result);

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

            auto configuration = local::resource::configuration( algorithm::transform( named, transform_name));

            auto transform_resource = [ &configuration]( auto predicate)
            {
               return [ &configuration, predicate = std::move( predicate)]( auto& resource)
               {  
                  auto is_configuration = [&]( auto& configuration){ return predicate( resource, configuration);};

                  if( auto found = algorithm::find_if( configuration, is_configuration))
                  {
                     // consume the resource configuration (make it impossible to reuse it...)
                     auto value = algorithm::container::extract( configuration, std::begin( found));

                     return Resource{
                        resource,
                        value.id,
                        std::move( value.openinfo),
                        std::move( value.closeinfo)};
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
            if( auto code = open(); code != code::tx::ok)
               code::raise::error( code, "failed to open during configure");

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
               namespace log
               {
                  template< typename Code, typename... Ts>
                  [[nodiscard]] auto code( Code code, Ts&&... ts)
                  {
                     if( code == Code::ok)
                        common::log::line( verbose::log, code, ' ', std::forward< Ts>( ts)...);
                     else
                        common::log::line( common::log::category::error, code, ' ', std::forward< Ts>( ts)...);

                     return code;
                  }

                  void event( std::string_view context, const transaction::ID& trid)
                  {
                     common::log::line( common::log::category::event::transaction, context, '|', trid);
                  }
               } // log

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
                  Transaction transaction( const platform::time::point::type& start, platform::time::unit timeout)
                  {
                     Transaction transaction{ common::transaction::id::create( process::handle())};
                     transaction.state = Transaction::State::active;
                     if( timeout > platform::time::unit{})
                        transaction.deadline = platform::time::clock::type::now() + timeout;

                     return transaction;
                  }
               } // start


               namespace resources::start
               {
                  /*
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
                  */

                  namespace involved
                  {
                     auto synchronize( const transaction::ID& trid, std::vector< strong::resource::id> resources)
                     {
                        Trace trace{ "transaction::local::resources::involved::synchronize"};

                        message::transaction::resource::involved::Request message;
                        message.process = process::handle();
                        message.trid = trid;
                        message.involved = std::move( resources);

                        common::log::line( common::log::category::transaction, "involved message: ", message);

                        return communication::ipc::call( communication::instance::outbound::transaction::manager::device(), message).involved;
                     }

                     void send( const transaction::ID& trid, std::vector< strong::resource::id> resources)
                     {
                        Trace trace{ "transaction::local::resources::involved::send"};

                        message::transaction::resource::involved::Request message;
                        message.process = process::handle();
                        message.trid = trid;
                        message.involved = std::move( resources);
                        message.reply = false;

                        common::log::line( common::log::category::transaction, "involved send-and-forget message: ", message);

                        communication::device::blocking::send( communication::instance::outbound::transaction::manager::device(), message);
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
                  [[nodiscard]] code::tx invoke( P&& policy, Transaction& transaction, R&& resources)
                  {
                     Trace trace{ "transaction::local::resources::start"};

                     if( resources.empty())
                        return code::tx::ok; // nothing to do...

                     return policy( transaction, resources);
                  }

                  namespace policy
                  {
                     //! When a service invocation joins a transaction
                     auto join()
                     {
                        return []( Transaction& transaction, auto&& resources)
                        {
                           // We absolutely know that this is a distributed transaction, since we've been invoked
                           // with a transaction that we shall join...
                           // We need to correlate with TM if we've going to use join our not.
                           auto involved = involved::synchronize( transaction.trid, transform::ids( resources));

                           // involve all resources
                           transaction.involve( transform::ids( resources));

                           return local::log::code( algorithm::accumulate( resources, code::tx::ok, local::accumulate::code( [ &transaction, &involved]( auto& resource)
                           {
                              if( algorithm::find( involved, resource.id()))
                                 return code::convert::to::tx( resource.start( transaction.trid, flag::xa::Flag::join));
                              else
                                 return code::convert::to::tx( resource.start( transaction.trid, flag::xa::Flag::no_flags));
                           })), "join - failed to start one ore more resources");
                        };
                     }

                     //! "helper" for start, branch and resume below
                     template< typename R>
                     auto start( Transaction& transaction, R& resources, flag::xa::Flag flags)
                     {
                        // involve the resources
                        transaction.involve( transform::ids( resources));

                        return algorithm::accumulate( transaction.involved(), code::tx::ok, local::accumulate::code( [ &transaction, &resources, flags]( auto& id)
                        {
                           if( auto found = algorithm::find( resources, id))
                              return code::convert::to::tx( found->start( transaction.trid, flags));

                           return code::tx::ok;
                        })); 
                     }

                     //! When a service invocation starts a transaction
                     auto start()
                     {
                        return []( Transaction& transaction, auto&& resources)
                        {
                           return policy::start( transaction, resources, flag::xa::Flag::no_flags);
                        };
                     }

                     auto branch()
                     {
                        return []( Transaction& transaction, auto&& resources)
                        {
                           // We absolutely know that this is a distributed transaction (non-local gtrid)
                           // we let the TM know about our resources
                           involved::send( transaction.trid, transform::ids( resources));

                           return policy::start( transaction, resources, flag::xa::Flag::no_flags);
                        };
                     }

                     auto resume()
                     {
                        return []( Transaction& transaction, auto&& resources)
                        {
                           return policy::start( transaction, resources, flag::xa::Flag::resume);
                        };
                     }
                     
                  } // policy
               } // resources::start

               namespace resources::end
               {
                  template< typename P, typename R>
                  [[nodiscard]] code::tx invoke( P&& policy, const Transaction& transaction, R& resources)
                  {
                     Trace trace{ "transaction::local::resources::end::invoke"};
                     common::log::line( verbose::log, "transaction: ", transaction, ", resources: ", resources);

                     if( resources.empty())
                        return code::tx::ok; // nothing to do...

                     return policy( transaction, resources);
                  }

                  namespace policy
                  {
                     template< typename R>
                     auto end( const Transaction& transaction, R& resources, flag::xa::Flag flags)
                     {
                        return algorithm::accumulate( transaction.involved(), code::tx::ok, local::accumulate::code( [ &transaction, &resources, flags]( auto& id)
                        {
                           if( auto found = algorithm::find( resources, id))
                              return code::convert::to::tx( found->end( transaction.trid, flags));

                           return code::tx::ok;
                        })); 
                     }

                     auto suspend()
                     {
                        return []( const Transaction& transaction, auto& resources)
                        {
                           return policy::end( transaction, resources, flag::xa::Flag::suspend);
                        };
                     }

                     auto success()
                     {
                        return []( const Transaction& transaction, auto& resources)
                        {
                           return policy::end( transaction, resources, flag::xa::Flag::success);
                        };
                     }
                  } // policy
                  
               } // resources::end

            } // <unnamed>
         } // local

         std::vector< strong::resource::id> Context::resources() const noexcept
         {
            return algorithm::transform( m_resources.all, []( auto& resource){ return resource.id();});
         }

         Transaction& Context::join( const transaction::ID& trid)
         {
            Trace trace{ "transaction::Context::join"};

            auto& transaction = m_transactions.emplace_back( trid);

            if( trid)
               local::raise::code( local::resources::start::invoke( local::resources::start::policy::join(), transaction, m_resources.fixed), 
                  "failed to join one or more fixed resources");

            local::log::event( "join", trid);

            return transaction;
         }

         Transaction& Context::start( const platform::time::point::type& start)
         {
            Trace trace{ "transaction::Context::start"};

            auto transaction = local::start::transaction( start, m_timeout);

            local::raise::code( local::resources::start::invoke( local::resources::start::policy::start(), transaction, m_resources.fixed),
               "failed to start on ore more fixed resources");

            local::log::event( "start", transaction.trid);

            m_transactions.push_back( std::move( transaction));
            return m_transactions.back();
         }

         Transaction& Context::branch( const transaction::ID& trid)
         {
            Trace trace{ "transaction::Context::branch"};

            auto& transaction = m_transactions.emplace_back( id::branch( trid));

            if( transaction)
               local::raise::code( local::resources::start::invoke( local::resources::start::policy::branch(), transaction, m_resources.fixed),
                  "failed to branch one or more fixed resources");

            local::log::event( "branch", transaction.trid);

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

            message::service::Transaction result;
            result.trid = std::move( caller);
            result.state = message::service::transaction::State::active;

            auto pending_check = [&]( Transaction& transaction)
            {
               if( transaction.pending())
               {
                  if( transaction.trid)
                  {
                     log::line( log::category::error, "pending replies associated with transaction - action: discard pending and set transaction state to rollback only");
                     log::line( log::category::transaction, transaction);

                     transaction.state = Transaction::State::rollback;
                     result.state = message::service::transaction::State::error;
                  }

                  // Discard pending
                  algorithm::for_each( transaction.correlations(), []( const auto& correlation){ 
                     communication::ipc::inbound::device().discard( correlation);
                  });
               }
            };

            auto invoke_rollback = [ this]( const Transaction& transaction)
            {
               return local::log::code( Context::rollback( transaction), "failed to rollback transaction: ", transaction.trid);
            };

            auto invoke_commit_rollback = [ this, invoke_rollback, commit]( const Transaction& transaction)
            {
               if( commit && transaction.state == Transaction::State::active)
                  return Context::commit( transaction);
               else
                  return invoke_rollback( transaction);
            };

            auto transform_state = casual::overloaded{
               []( Transaction::State state)
               {
                  using State = message::service::transaction::State;
                  switch( state)
                  {
                     case Transaction::State::active: return State::active;
                     case Transaction::State::rollback: return State::rollback;
                     case Transaction::State::timeout: return State::timeout;
                  }
                  return State::error;
               },
               []( code::tx code)
               {
                  using State = message::service::transaction::State;
                  switch( code)
                  {
                     case code::tx::ok: return State::active;
                     case code::tx::rollback: return State::rollback;
                     default: return State::error;
                  }
               }};


            // Check pending calls
            algorithm::for_each( transactions, pending_check);

            // Ignore 'null trid':s
            auto filter_active = []( auto& transactions)
            {
               return algorithm::stable::filter( transactions, []( auto& transaction){ return predicate::boolean( transaction);});
            };

            auto [ not_owner, owner] = algorithm::stable::partition( filter_active( transactions), []( auto& transaction)
            {
               return transaction.trid.owner() != process::handle();
            });

            // take care of owned transactions
            auto code = algorithm::accumulate( owner, code::tx::ok, local::accumulate::code( invoke_commit_rollback));

            // Take care of not-owned transaction(s) ( even if owned failed in some way, we need to "consume" not-owned)
            {
               // should be 0..1
               assert( not_owner.size() <= 1);

               if( not_owner)
               {
                  log::line( log::category::transaction, "not_owner: ", *not_owner);

                  result.state += transform_state( not_owner->state);

                  if( ! commit && result.state == decltype( result.state)::active)
                     result.state += decltype( result.state)::rollback;

                  // end resource
                  code += local::resources::end::invoke( local::resources::end::policy::success(), *not_owner, m_resources.all);                  
               }
            }

            result.state += transform_state( code);

            log::line( log::category::transaction, "result: ", result);
            log::line( log::category::event::transaction , "finalize");

            return result;
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
               auto involved = local::resources::start::involved::synchronize( transaction.trid, { rmid});
               return algorithm::find( involved, rmid).empty() ? code::ax::ok : code::ax::join;
            }

            return code::ax::ok;
         }

         code::ax Context::resource_unregistration( strong::resource::id rmid)
         {
            Trace trace{ "transaction::Context::resource_unregistration"};

            auto& transaction = current();

            // RM:s can only unregister if we're outside global
            // transactions, and the rm is registered before
            if( transaction.trid || ! transaction.disassociate_dynamic( rmid))
               return local::log::code( code::ax::protocol, "resource id: ", rmid);

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
                     return local::log::code( code::tx::protocol, "begin - already in transaction mode - ", transaction);

                  // Tell the RM:s to suspend
                  if( auto code = local::resources::end::invoke( local::resources::end::policy::suspend(), transaction, m_resources.all); code != code::tx::ok)
                     return local::log::code( code, "failed to suspend resources for transaction: ", transaction);
               }
               else if( ! transaction.dynamic().empty())
                  return code::tx::outside;
            }

            auto transaction = local::start::transaction( platform::time::clock::type::now(), m_timeout);

            // We know we've got a local transaction.
            {
               auto resource_start = [ &transaction]( auto& resource)
               {
                  // involve the resource in the transaction
                  transaction.involve( resource.id());
                  return algorithm::compare::any( resource.start( transaction.trid, flag::xa::Flag::no_flags), code::xa::ok, code::xa::read_only);
               };
               auto [ successful, failed] = algorithm::partition( m_resources.fixed, resource_start);

               if( failed)
               {
                  // some of the resources failed, make sure we xa_end the successful ones...
                  algorithm::for_each( successful, [ &transaction]( auto& resource)
                  {
                     // TODO semantics: is it ok to end with success? Seams better than 
                     // to mark this extremely short lived transaction with 'error'. Don't know
                     // how resources "wants it"...
                     resource.end( transaction.trid, flag::xa::Flag::success);
                  });

                  return local::log::code( code::tx::error, "some resources failed to start: ", algorithm::transform( failed, []( auto& rm){ return rm.id();}));
               }
            }            

            m_transactions.push_back( std::move( transaction));
            local::log::event( "begin", m_transactions.back().trid);

            return code::tx::ok;
         }


         code::tx Context::open()
         {
            Trace trace{ "transaction::Context::open"};

            // XA spec: if one, or more of resources opens ok, then it's not an error...
            //   seams really strange not to notify user that some of the resources has
            //   failed to open...

            return local::log::code( algorithm::accumulate( m_resources.all, code::tx::ok, local::accumulate::code( []( auto& resource)
            {
               return code::convert::to::tx( resource.open());
            })), "failed to open one or more resource");

         }

         code::tx Context::close()
         {
            Trace trace{ "transaction::Context::close"};

            return local::log::code( algorithm::accumulate( m_resources.all, code::tx::ok, local::accumulate::code( []( auto& resource)
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
                       return local::log::code( code::tx::protocol, "commit - no ongoing transaction");

                     if( transaction.trid.owner() != process::handle())
                       return local::log::code( code::tx::protocol, "commit - not owner of transaction: ", transaction.trid);

                     if( transaction.state != Transaction::State::active)
                       return local::log::code( code::tx::protocol, "commit - transaction is in rollback only mode - ", transaction.trid);

                     if( transaction.pending())
                       return local::log::code( code::tx::protocol, "commit - pending replies associated with transaction: ", transaction.trid);

                     return code::tx::ok;
                  }

                  code::tx rollback( const Transaction& transaction)
                  {
                     Trace trace{ "transaction::local::precondition::rollback"};

                     if( ! transaction)
                       return local::log::code( code::tx::protocol, "rollback - no ongoing transaction");

                     if( transaction.trid.owner() != process::handle())
                       return local::log::code( code::tx::protocol, "rollback - not owner of transaction: ", transaction.trid);

                     // TODO can we do a rollback with pending replies? I think so...
                     //if( transaction.pending())
                     //  return local::log::code( code::tx::protocol, "rollback - pending replies associated with transaction: ", transaction.trid);

                     return code::tx::ok;
                  }
               } // precondition
            } // <unnamed>
         } // local


         code::tx Context::commit( const Transaction& transaction)
         {
            Trace trace{ "transaction::Context::commit - transaction"};
            casual::assertion( transaction, "not a valid transaction: ", transaction);

            // end resources
            if( auto code = local::resources::end::invoke( local::resources::end::policy::success(), transaction, m_resources.all); code != code::tx::ok)
               return local::log::code( code, "commit - failed to end one or more resources");

            if( transaction.local() && transaction.involved().size() <= 1)
            {
               Trace trace{ "transaction::Context::commit - local"};

               // transaction is local, and at most one resource is involved.
               // We do the commit directly against the resource (if any).
               // TODO: we could do a two-phase-commit local if the transaction is 'local'

               if( ! transaction.involved().empty())
                  return resource_commit( transaction.involved().front(), transaction, flag::xa::Flag::one_phase);

               local::log::event( "commit", transaction.trid); 

               // No resources associated to this transaction, hence the commit is successful.
               return code::tx::ok;
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
                  using Stage = decltype( reply.stage);

                  case Stage::prepare:
                  {
                     log::line( log::category::transaction, "commit - stage prepare - state: ", reply.state);

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
                  case Stage::commit:
                     local::log::event( "commit", transaction.trid);
                     return local::log::code( reply.state, "commit - stage commit");
                  case Stage::rollback:
                     local::log::event( "commit", transaction.trid);
                     return local::log::code( code::tx::rollback, "commit - stage rollback - state: ", reply.state);
               }

               local::log::event( "commit", transaction.trid);
               return local::log::code( reply.state, "during commit");
            }
         }

         code::tx Context::commit()
         {
            Trace trace{ "transaction::Context::commit"};

            if( auto code = local::precondition::commit( current()); code != code::tx::ok)
               return code;

            // we know that we got an _active_ transaction that passed the precondition.
            // we consume the transaction, regardless...
            auto transaction = algorithm::container::extract( m_transactions, std::prev( std::end( m_transactions)));

            return control_continuation( commit( transaction));
         }

         code::tx Context::rollback( const Transaction& transaction)
         {
            Trace trace{ "transaction::Context::rollback"};
            casual::assertion( transaction, "not a valid transaction: ", transaction);

            // end resources
            if( auto code = local::resources::end::invoke( local::resources::end::policy::success(), transaction, m_resources.all); code != code::tx::ok)
               return local::log::code( code, "rollback - failed to end one or more resources for transaction: ", transaction.trid);

            if( transaction.local())
            {
               log::line( log::category::transaction, "rollback is local");

               auto involved = transaction.involved();
               algorithm::transform( m_resources.fixed, involved, []( auto& r){ return r.id();});

               auto result = algorithm::accumulate( algorithm::unique( algorithm::sort( involved)), code::tx::ok, local::accumulate::code( [&]( auto id)
               {
                  return resource_rollback( id, transaction);
               }));

               local::log::event( "rollback", transaction.trid);

               return result;
            }
            else 
            {
               log::line( log::category::transaction, "rollback is distributed");

               message::transaction::rollback::Request request{ process::handle()};
               request.trid = transaction.trid;
               algorithm::container::append( transaction.involved(), request.involved);

               auto reply = communication::ipc::call( communication::instance::outbound::transaction::manager::device(), request);

               local::log::event( "rollback", transaction.trid);

               return local::log::code( reply.state, "during rollback");
            }
         }

         code::tx Context::rollback()
         {
            if( auto code = local::precondition::rollback( current()); code != code::tx::ok)
               return code;

            // we know that we got an _active_ transaction that passed the precondition.
            // we consume the transaction, regardless...
            auto transaction = algorithm::container::extract( m_transactions, std::prev( std::end( m_transactions)));

            return control_continuation( rollback( transaction));
         }

         code::tx Context::set_commit_return( commit::Return value) noexcept
         {
            log::line( verbose::log, "set_commit_return: ", value);
            m_commit_return = value;

            return code::tx::ok;
         }

         commit::Return Context::get_commit_return() const noexcept
         {
            return m_commit_return;
         }

         code::tx Context::set_transaction_control( transaction::Control control)
         {
            log::line( verbose::log, "set_transaction_control: ", control);
            m_control = control;
            return code::tx::ok;
         }

         code::tx Context::set_transaction_timeout( platform::time::unit timeout)
         {
            if( timeout < platform::time::unit{})
               return local::log::code( code::tx::argument, "set_transaction_timeout - timeout value has to be 0 or greater");

            m_timeout = timeout;
            return code::tx::ok;
         }

         bool Context::info( TXINFO* info)
         {
            auto&& transaction = current();

            if( info)
            {
               info->xid = transaction.trid.xid;
               info->transaction_state = static_cast< decltype( info->transaction_state)>( transaction.state);
               info->transaction_timeout = std::chrono::duration_cast< std::chrono::seconds>( m_timeout).count();
               info->transaction_control = std::to_underlying( m_control);
            }
            return static_cast< bool>( transaction);
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
                        return local::log::code( code::tx::argument, "suspend: argument xid is null");

                     if( id::null( current.trid))
                        return local::log::code( code::tx::protocol, "suspend: attempt to suspend a null xid");

                     return code::tx::ok;
                  }

                  auto resume( const XID* xid, const Transaction& current)
                  {
                     if( xid == nullptr)
                        return local::log::code( code::tx::argument, "resume: argument xid is null");

                     if( xid::null( *xid))
                        return local::log::code( code::tx::argument, "resume: attempt to resume a 'null xid'");

                     if( current.trid && ! current.suspended())
                        return local::log::code( code::tx::protocol, "resume: ongoing transaction is active");

                     if( ! current.dynamic().empty())
                        return local::log::code( code::tx::outside, "resume: ongoing work outside global transaction: ", current);

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

            // Tell the RM:s to suspend
            if( auto code = local::resources::end::invoke( local::resources::end::policy::suspend(), ongoing, m_resources.all); code != code::tx::ok)
               return local::log::code( code::tx::protocol, "suspend: failed to suspend one or more resources");

            *xid = ongoing.trid.xid;

            local::log::event( "suspend", ongoing.trid);

            return code::tx::ok;
         }

         code::tx Context::resume( const XID* xid)
         {
            Trace trace{ "transaction::Context::resume"};

            if( auto code = local::precondition::resume( xid, current()); code != code::tx::ok)
               return code;

            if( auto found = algorithm::find( m_transactions, *xid))
            {
               if( ! found->suspended())
                  return local::log::code( code::tx::protocol, "resume: wanted transaction is not suspended");

               found->resume();

               // Tell the RM:s to resume
               if( auto code = local::resources::start::invoke( local::resources::start::policy::resume(), *found, m_resources.fixed); code != code::tx::ok)
                  return local::log::code( code::tx::argument, "resume: failed to resume one or more fixed resources");

               // We rotate the wanted to end;
               algorithm::rotate( m_transactions, ++found);

               local::log::event( "resume", current().trid);

               return code::tx::ok;
            }
            else
               return local::log::code( code::tx::argument, "resume: transaction not known");

         }

         void Context::resources_resume( Transaction& transaction)
         {
            Trace trace{ "transaction::Context::resources_start"};

            if( ! transaction)
               return; // nothing to do

            local::raise::code( local::resources::start::invoke( local::resources::start::policy::resume(), transaction, m_resources.fixed),
               "failed to resume one ore more fixed resource");
         }

         void Context::resources_suspend( Transaction& transaction)
         {
            Trace trace{ "transaction::Context::resources_suspend"};

            if( ! transaction)
               return;

            // Tell the RM:s to suspend
            local::raise::code( local::resources::end::invoke( local::resources::end::policy::suspend(), transaction, m_resources.all),
               "failed to suspend one ore more resource");
         }

         code::tx Context::resource_commit( strong::resource::id rm, const Transaction& transaction, flag::xa::Flag flags)
         {
            Trace trace{ "transaction::Context::resources_commit"};
            log::line( log::category::transaction, "transaction: ", transaction, " - rm: ", rm, " - flags: ", flags);

            if( auto found = algorithm::find( m_resources.all, rm))
               return common::code::convert::to::tx( found->commit( transaction.trid, flags));
            else
               return local::log::code( code::tx::error, "resource id not known - rm: ", rm, " transaction: ", transaction);
         }

         code::tx Context::resource_rollback( strong::resource::id rm, const Transaction& transaction)
         {
            Trace trace{ "transaction::Context::resource_rollback"};
            log::line( log::category::transaction, "transaction: ", transaction, " - rm: ", rm);

            if( auto found = algorithm::find( m_resources.all, rm))
               return common::code::convert::to::tx( found->rollback( transaction.trid, flag::xa::Flag::no_flags));
            else
               return local::log::code( code::tx::error, "resource id not known - rm: ", rm, " transaction: ", transaction);
         }

         code::tx Context::control_continuation( code::tx code)
         {
            Trace trace{ "transaction::Context::control_continuation"};
            log::line( verbose::log, "code: ", code);

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
                     return code + local::resources::start::invoke( local::resources::start::policy::resume(), current, m_resources.all);
                  return code;
            }

            casual::terminate( code::casual::internal_unexpected_value, "unknown control directive: ", std::to_underlying( m_control), " - this can not happen");
         }

      } // transaction
   } // common
} //casual


