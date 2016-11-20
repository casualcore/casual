//!
//! casual
//!

#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"
#include "transaction/common.h"


namespace casual
{

   namespace transaction
   {
      namespace ipc
      {

         const common::communication::ipc::Helper& device()
         {
            static common::communication::ipc::Helper ipc{
               common::communication::error::handler::callback::on::Terminate
               {
                  []( const common::process::lifetime::Exit& exit){
                     //
                     // We put a dead process event on our own ipc device, that
                     // will be handled later on.
                     //
                     common::message::domain::process::termination::Event event{ exit};
                     common::communication::ipc::inbound::device().push( std::move( event));
                  }
               }
            };
            return ipc;

         }

      } // ipc

      namespace handle
      {
         namespace local
         {
            namespace
            {

               namespace transform
               {

                  template< typename R, typename M>
                  R message( M&& message)
                  {
                     R result;

                     result.process = message.process;
                     result.trid = message.trid;

                     return result;
                  }


                  template< typename M>
                  auto reply( M&& message) -> decltype( common::message::reverse::type( message))
                  {
                     auto result = common::message::reverse::type( message);

                     result.process = common::process::handle();
                     result.trid = message.trid;

                     return result;
                  }

               } // transform

               namespace send
               {
                  namespace persistent
                  {

                     template< typename R, typename M>
                     void reply( State& state, const M& request, int code, const common::process::Handle& target)
                     {
                        R message;
                        message.correlation = request.correlation;
                        message.process = common::process::handle();
                        message.trid = request.trid;
                        message.state = code;

                        state.persistent.replies.emplace_back( target.queue, std::move( message));
                     }

                     template< typename R, typename M>
                     void reply( State& state, const M& request, int code)
                     {
                        reply< R>( state, request, code, request.process);
                     }

                     template< typename M>
                     void reply( State& state, M&& message, const common::process::Handle& target)
                     {
                        state.persistent.replies.emplace_back( target.queue, std::move( message));
                     }

                  } // persistent

                  template< typename M>
                  struct Reply : state::Base
                  {
                     using state::Base::Base;

                     template< typename T>
                     void operator () ( const T& request, int code, const common::process::Handle& target)
                     {
                        M message;
                        message.correlation = request.correlation;
                        message.process = common::process::handle();
                        message.trid = request.trid;
                        message.state = code;

                        state::pending::Reply reply{ target.queue, message};

                        action::persistent::Send send{ m_state};

                        if( ! send( reply))
                        {
                           log << "failed to send reply directly to : " << target << " type: " << common::message::type( message) << " transaction: " << message.trid << " - action: pend reply\n";
                           m_state.persistent.replies.push_back( std::move( reply));
                        }

                     }

                     template< typename T>
                     void operator () ( const T& request, int code)
                     {
                        operator()( request, code, request.process);
                     }

                  };


                  template< typename R>
                  void reply( State& state, R&& message, const common::process::Handle& target)
                  {
                     state::pending::Reply reply{ target.queue, std::forward< R>( message)};

                     action::persistent::Send send{ state};

                     if( ! send( reply))
                     {
                        log << "failed to send reply directly to : " << target << " type: " << common::message::type( message) << " transaction: " << message.trid << " - action: pend reply\n";
                        state.persistent.replies.push_back( std::move( reply));
                     }
                  }

                  template< typename R>
                  void read_only( State& state, R&& message)
                  {
                     auto reply = local::transform::reply( message);
                     reply.state = XA_RDONLY;
                     reply.resource = message.resource;

                     send::reply( state, std::move( reply), message.process);
                  }




                  namespace resource
                  {

                     namespace persistent
                     {
                        template< typename M, typename F>
                        void request( State& state, Transaction& transaction, F&& filter, long flags = TMNOFLAGS)
                        {
                           auto resources = common::range::partition(
                              transaction.resources,
                              filter);

                           common::range::for_each( resources, [&]( const Transaction::Resource& r){

                              M message;
                              message.process = common::process::handle();
                              message.trid = transaction.trid;
                              message.flags = flags;
                              message.resource = r.id;

                              state::pending::Request request{ r.id, message};

                              state.persistent.requests.push_back( std::move( request));
                           });
                        }

                     } // persistent


                     template< typename M, typename F>
                     void request( State& state, Transaction& transaction, F&& filter, Transaction::Resource::Stage new_stage, long flags = TMNOFLAGS)
                     {
                        Trace trace{ "transaction::handle::send::resource::request"};


                        auto resources = std::get< 0>( common::range::partition(
                           transaction.resources,
                           filter));

                        //
                        // Update state on transaction-resources
                        //
                        common::range::for_each(
                           resources,
                           Transaction::Resource::update::Stage{ new_stage});


                        common::range::for_each( resources, [&]( const Transaction::Resource& r){

                           M message;
                           message.process = common::process::handle();
                           message.trid = transaction.trid;
                           message.flags = flags;
                           message.resource = r.id;

                           state::pending::Request request{ r.id, message};

                           if( ! action::resource::request( state, request))
                           {
                              //
                              // Could not send to resource. We put the request
                              // in pending.
                              //
                              log << "could not send to resource: " << r.id << " - action: try later\n";

                              state.pending.requests.push_back( std::move( request));
                           }
                        });
                     }

                  } // resource
               } // send

               namespace instance
               {

                  void done( State& state, state::resource::Proxy::Instance& instance)
                  {
                     auto request = common::range::find_if(
                           state.pending.requests,
                           state::pending::filter::Request{ instance.id});

                     instance.state( state::resource::Proxy::Instance::State::idle);


                     if( request)
                     {
                        //
                        // We got a pending request for this resource, let's oblige
                        //
                        if( ipc::device().non_blocking_push( instance.process.queue, request->message))
                        {
                           instance.state( state::resource::Proxy::Instance::State::busy);

                           state.pending.requests.erase( std::begin( request));
                        }
                        else
                        {
                           common::log::error << "failed to send pending request to resource, although the instance (" << instance <<  ") reported idle\n";
                        }
                     }
                  }

                  template< typename M>
                  void statistics( state::resource::Proxy::Instance& instance, M&& message, const common::platform::time_point& now)
                  {
                     instance.statistics.resource.time( message.statistics.start, message.statistics.end);
                     instance.statistics.roundtrip.end( now);
                  }

               } // instance

               namespace resource
               {

                  template< typename M>
                  void involved( State& state, Transaction& transaction, M&& message)
                  {
                     for( auto& resource : message.resources)
                     {
                        if( common::range::find( state.resources, resource))
                        {
                           transaction.resources.emplace_back( resource);
                        }
                        else
                        {
                           common::log::error << "invalid resource id: " << resource << " - involved with " << message.trid << " - action: discard\n";
                        }
                     }

                     common::range::trim( transaction.resources, common::range::unique( common::range::sort( transaction.resources)));

                     log << "involved: " << transaction << '\n';

                  }

               } // resource

               namespace transaction
               {
                  template< typename M>
                  auto find_or_add( State& state, M&& message) -> decltype( common::range::make( state.transactions))
                  {
                     //
                     // Find the transaction
                     //
                     auto found = common::range::find_if( state.transactions, find::Transaction{ message.trid});

                     if( found)
                     {
                        return found;
                     }

                     state.transactions.emplace_back( message.trid);

                     return common::range::make( std::prev( std::end( state.transactions)), std::end( state.transactions));
                  }

               } // transaction

               namespace implementation
               {
                  struct Local : handle::implementation::Interface
                  {
                     static const Local* instance()
                     {
                        static Local singleton;
                        return &singleton;
                     }

                     bool prepare( State& state, common::message::transaction::resource::prepare::Reply& message, Transaction& transaction) const override
                     {
                        Trace trace{ "transaction::handle::resource::prepare local reply"};

                        using reply_type = common::message::transaction::commit::Reply;

                        //
                        // Normalize all the resources return-state
                        //
                        auto result = transaction.results();

                        switch( result)
                        {
                           case Transaction::Resource::Result::xa_RDONLY:
                           {
                              log << "prepare completed - " << transaction << " XA_RDONLY\n";

                              //
                              // Read-only optimization. We can send the reply directly and
                              // discard the transaction
                              //
                              state.log.remove( transaction.trid);

                              //
                              // Send reply
                              //
                              {
                                 auto reply = local::transform::message< reply_type>( message);
                                 reply.correlation = transaction.correlation;
                                 reply.stage = reply_type::Stage::commit;
                                 reply.state = XA_RDONLY;

                                 local::send::reply( state, std::move( reply), transaction.trid.owner());
                              }

                              //
                              // Indicate that wrapper should remove the transaction from state
                              //
                              return true;
                           }
                           case Transaction::Resource::Result::xa_OK:
                           {
                              log << "prepare completed - " << transaction << " XA_OK\n";

                              //
                              // Prepare has gone ok. Log state
                              //
                              state.log.prepare( transaction.trid);

                              //
                              // prepare send reply. Will be sent after persistent write to file
                              //
                              {
                                 auto reply = local::transform::message< reply_type>( message);
                                 reply.correlation = transaction.correlation;
                                 reply.stage = reply_type::Stage::prepare;
                                 reply.state = XA_OK;

                                 local::send::persistent::reply( state, std::move( reply), transaction.trid.owner());
                              }

                              //
                              // All XA_OK is to be committed, send commit to all
                              //

                              //
                              // We only want to send to resorces that has reported ok, and is in prepared state
                              // (could be that some has read-only)
                              //
                              auto filter = common::chain::And::link(
                                    Transaction::Resource::filter::Result{ Transaction::Resource::Result::xa_OK},
                                    Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::prepare_replied});

                              local::send::resource::request<
                                 common::message::transaction::resource::commit::Request>( state, transaction, filter, Transaction::Resource::Stage::prepare_requested);


                              break;
                           }
                           default:
                           {
                              //
                              // Something has gone wrong.
                              //
                              common::log::error << "prepare phase failed for transaction: " << transaction << " - action: rollback\n";

                              local::send::resource::request< common::message::transaction::resource::rollback::Request>(
                                 state,
                                 transaction,
                                 Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::prepare_replied},
                                 Transaction::Resource::Stage::rollback_requested);

                              break;
                           }
                        }


                        //
                        // Transaction is not done
                        //
                        return false;
                     }

                     bool commit( State& state, common::message::transaction::resource::commit::Reply& message, Transaction& transaction) const override
                     {
                        Trace trace{ "transaction::handle::resource::commit local reply"};

                        using reply_type = common::message::transaction::commit::Reply;

                        //
                        // Normalize all the resources return-state
                        //
                        auto result = transaction.results();

                        switch( result)
                        {
                           case Transaction::Resource::Result::xa_OK:
                           {
                              log << "commit completed - " << transaction << " XA_OK\n";

                              auto reply = local::transform::message< reply_type>( message);
                              reply.correlation = transaction.correlation;
                              reply.stage = reply_type::Stage::commit;
                              reply.state = XA_OK;

                              if( transaction.resources.size() <= 1)
                              {
                                 local::send::reply( state, reply, transaction.trid.owner());
                              }
                              else
                              {
                                 //
                                 // Send reply
                                 // TODO: We can't send reply directly without checking that the prepare-reply
                                 // has been sent (prepare-reply could be pending for persistent write).
                                 // For now, we do a delayed reply, so we're sure that the
                                 // commit-reply arrives after the prepare-reply
                                 //
                                 local::send::persistent::reply( state, std::move( reply), transaction.trid.owner());
                              }

                              //
                              // Remove transaction
                              //
                              state.log.remove( message.trid);
                              return true;
                           }
                           default:
                           {
                              //
                              // Something has gone wrong.
                              //
                              common::log::error << "TODO: something has gone wrong...\n";

                              //
                              // prepare send reply. Will be sent after persistent write to file.
                              //
                              // TOOD: we do have to save the state of the transaction?
                              //
                              {
                                 auto reply = local::transform::message< reply_type>( message);
                                 reply.correlation = transaction.correlation;
                                 reply.stage = reply_type::Stage::commit;
                                 reply.state = Transaction::Resource::convert( result);

                                 local::send::persistent::reply( state, std::move( reply), transaction.trid.owner());
                              }
                              break;
                           }
                        }

                        return false;
                     }

                     bool rollback( State& state, common::message::transaction::resource::rollback::Reply& message, Transaction& transaction) const override
                     {
                        Trace trace{ "transaction::handle::resource::rollback local reply"};

                        using reply_type = common::message::transaction::rollback::Reply;

                        //
                        // Normalize all the resources return-state
                        //
                        auto result = transaction.results();

                        switch( result)
                        {
                           case Transaction::Resource::Result::xa_OK:
                           case Transaction::Resource::Result::xaer_NOTA:
                           case Transaction::Resource::Result::xa_RDONLY:
                           {
                              log << "rollback completed - " << transaction << " XA_OK\n";

                              //
                              // Send reply
                              //
                              {
                                 auto reply = local::transform::message< reply_type>( message);
                                 reply.correlation = transaction.correlation;
                                 reply.state = XA_OK;

                                 local::send::reply( state, std::move( reply), transaction.trid.owner());
                              }

                              //
                              // Remove transaction
                              //
                              state.log.remove( message.trid);
                              return true;
                           }
                           default:
                           {
                              //
                              // Something has gone wrong.
                              //
                              common::log::error << "TODO: resource rollback - something has gone wrong...\n";

                              //
                              // prepare send reply. Will be sent after persistent write to file
                              //
                              //
                              // TOOD: we do have to save the state of the transaction?
                              //
                              {
                                 auto reply = local::transform::message< reply_type>( message);
                                 reply.correlation = transaction.correlation;
                                 reply.state = Transaction::Resource::convert( result);

                                 local::send::persistent::reply( state, std::move( reply), transaction.trid.owner());
                              }
                              break;
                           }
                        }
                        return false;

                     }


                  private:
                     Local() = default;
                  };

                  struct Remote : handle::implementation::Interface
                  {
                     static const Remote* instance()
                     {
                        static Remote singleton;
                        return &singleton;
                     }

                     bool prepare( State& state, common::message::transaction::resource::prepare::Reply& message, Transaction& transaction) const override
                     {
                        Trace trace{ "transaction::handle::implementation::Remote::prepare reply"};

                        //
                        // Transaction is owned by another domain, so we just act as a resource.
                        // This TM does not own the transaction, so we don't need to store
                        // state.
                        //

                        using reply_type = common::message::transaction::resource::prepare::Reply;

                        {
                           auto reply = local::transform::message< reply_type>( message);
                           reply.state = Transaction::Resource::convert( transaction.results());
                           reply.correlation = transaction.correlation;
                           reply.resource = transaction.resource;

                           local::send::reply( state, std::move( reply), transaction.trid.owner());
                        }

                        return false;
                     }

                     bool commit( State& state, common::message::transaction::resource::commit::Reply& message, Transaction& transaction) const override
                     {
                        Trace trace{ "transaction::handle::implementation::Remote::commit reply"};

                        //
                        // Transaction is owned by another domain, so we just act as a resource.
                        // This TM does not own the transaction, so we don't need to store
                        // state.
                        //

                        using reply_type = common::message::transaction::resource::commit::Reply;

                        {
                           auto reply = local::transform::message< reply_type>( message);
                           reply.state = Transaction::Resource::convert( transaction.results());
                           reply.correlation = transaction.correlation;
                           reply.resource = transaction.resource;

                           local::send::reply( state, std::move( reply), transaction.trid.owner());
                        }

                        return true;
                     }

                     bool rollback( State& state, common::message::transaction::resource::rollback::Reply& message, Transaction& transaction) const override
                     {
                        Trace trace{ "transaction::handle::implementation::Remote::rollback reply"};

                        //
                        // Transaction is owned by another domain, so we just act as a resource.
                        // This TM does not own the transaction, so we don't need to store
                        // state.
                        //

                        using reply_type = common::message::transaction::resource::rollback::Reply;

                        {
                           auto reply = local::transform::message< reply_type>( message);
                           reply.state = Transaction::Resource::convert( transaction.results());
                           reply.correlation = transaction.correlation;
                           reply.resource = transaction.resource;

                           local::send::reply( state, std::move( reply), transaction.trid.owner());
                        }

                        return true;
                     }

                  protected:
                     Remote() = default;
                  };


                  namespace one
                  {
                     namespace phase
                     {
                        namespace commit
                        {

                           struct Remote : implementation::Remote
                           {
                              static const Remote* instance()
                              {
                                 static Remote singleton;
                                 return &singleton;
                              }

                              bool prepare( State& state, common::message::transaction::resource::prepare::Reply& message, Transaction& transaction) const override
                              {
                                 Trace trace{ "transaction::handle::implementation::one::phase::commit::Remote::prepare reply"};
                                 //
                                 // We're done with the prepare phase, start with commit or rollback
                                 //

                                 using reply_type = common::message::transaction::resource::commit::Reply;

                                 //
                                 // Normalize all the resources return-state
                                 //
                                 auto result = transaction.results();

                                 switch( result)
                                 {
                                    case Transaction::Resource::Result::xa_RDONLY:
                                    {
                                       log << "prepare completed - " << transaction << " XA_RDONLY\n";

                                       //
                                       // Read-only optimization. We can send the reply directly and
                                       // discard the transaction
                                       //
                                       state.log.remove( transaction.trid);

                                       //
                                       // Send reply
                                       //
                                       {
                                          auto reply = local::transform::message< reply_type>( message);
                                          reply.correlation = transaction.correlation;
                                          reply.resource = transaction.resource;
                                          reply.state = XA_RDONLY;

                                          local::send::reply( state, std::move( reply), transaction.trid.owner());
                                       }

                                       //
                                       // Indicate that wrapper should remove the transaction from state
                                       //
                                       return true;
                                    }
                                    case Transaction::Resource::Result::xa_OK:
                                    {
                                       log << "prepare completed - " << transaction << " XA_OK\n";

                                       //
                                       // Prepare has gone ok. Log state
                                       //
                                       state.log.prepare( transaction.trid);


                                       //
                                       // All XA_OK is to be committed, send commit to all
                                       //

                                       //
                                       // We only want to send to resorces that has reported ok, and is in prepared state
                                       // (could be that some has read-only)
                                       //
                                       auto filter = common::chain::And::link(
                                             Transaction::Resource::filter::Result{ Transaction::Resource::Result::xa_OK},
                                             Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::prepare_replied});

                                       local::send::resource::request<
                                          common::message::transaction::resource::commit::Request>(
                                                state, transaction, filter, Transaction::Resource::Stage::prepare_requested);


                                       break;
                                    }
                                    default:
                                    {
                                       //
                                       // Something has gone wrong.
                                       //
                                       common::log::error << "prepare phase failed for transaction: " << transaction << " - action: rollback\n";

                                       local::send::resource::request< common::message::transaction::resource::rollback::Request>(
                                          state,
                                          transaction,
                                          Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::prepare_replied},
                                          Transaction::Resource::Stage::rollback_requested);

                                       break;
                                    }
                                 }
                                 return false;
                              }

                              bool rollback( State& state, common::message::transaction::resource::rollback::Reply& message, Transaction& transaction) const override
                              {
                                 Trace trace{ "transaction::handle::implementation::one::phase::commit::Remote::handle rollback reply"};

                                 using reply_type = common::message::transaction::resource::commit::Reply;

                                 //
                                 // Send reply
                                 //
                                 {
                                    auto reply = local::transform::message< reply_type>( message);
                                    reply.correlation = transaction.correlation;
                                    reply.resource = transaction.resource;
                                    reply.state = XA_RBOTHER;

                                    local::send::reply( state, std::move( reply), transaction.trid.owner());
                                 }
                                 return true;
                              }

                           private:
                              Remote() = default;
                           };

                        } // commit
                     } // phase
                  } // one
               } // implementation

            } // <unnamed>
         } // local

         namespace process
         {


            void Exit::operator () ( message_type& message)
            {
               apply( message.death);
            }

            void Exit::apply( const common::process::lifetime::Exit& exit)
            {
               Trace trace{ "transaction::handle::dead::Process"};

               // TODO: check if it's one of spawned resource proxies, if so, restart?

               //
               // Check if the now dead process is owner to any transactions, if so, roll'em back...
               //
               std::vector< common::transaction::ID> trids;

               for( auto& trans : m_state.transactions)
               {
                  if( trans.trid.owner().pid == exit.pid)
                  {
                     trids.push_back( trans.trid);
                  }
               }

               for( auto& trid : trids)
               {
                  common::message::transaction::rollback::Request request;
                  request.process = common::process::handle();
                  request.trid = trid;

                  //
                  // This could change the state, that's why we don't do it directly in the loop above.
                  //
                  handle::Rollback{ m_state}( request);
               }
            }
         }

         namespace resource
         {
            void Involved::operator () ( common::message::transaction::resource::Involved& message)
            {
               Trace trace{ "transaction::handle::resource::Involved"};

               log << "involved message: " << message << '\n';

               auto& transaction = *local::transaction::find_or_add( m_state, message);

               local::resource::involved( m_state, transaction, message);
            }

            namespace reply
            {

               template< typename H>
               void Wrapper< H>::operator () ( message_type& message)
               {

                  if( state::resource::id::local( message.resource))
                  {
                     //
                     // The resource is a local resource proxy, and it's done
                     // and ready for more work
                     //

                     auto& instance = m_state.get_instance( message.resource, message.process.pid);

                     {
                        local::instance::done( this->m_state, instance);
                        local::instance::statistics( instance, message, common::platform::clock_type::now());
                     }

                  }

                  //
                  // Find the transaction
                  //
                  auto found = common::range::find_if(
                        common::range::make( m_state.transactions), find::Transaction{ message.trid});

                  if( found)
                  {
                     auto& transaction = *found;

                     auto resource = common::range::find_if(
                           transaction.resources,
                           Transaction::Resource::filter::ID{ message.resource});

                     if( resource)
                     {
                        //
                        // We found all the stuff, let the real handler handle the message
                        //
                        if( m_handler( message, transaction, *resource))
                        {
                           //
                           // We remove the transaction from our state
                           //
                           m_state.transactions.erase( std::begin( found));
                        }
                     }
                     else
                     {
                        // TODO: what to do? We have previously sent a prepare request, why do we not find the resource?
                        common::log::error << "failed to locate resource: " <<  message.resource  << " for trid: " << message.trid << " - action: discard?\n";
                     }

                  }
                  else
                  {
                     // TODO: what to do? We have previously sent a prepare request, why do we not find the trid?
                     common::log::error << "failed to locate trid: " << message.trid << " - action: discard?\n";
                  }
               }



               void Connect::operator () ( message_type& message)
               {
                  Trace trace{ "transaction::handle::resource::connect reply"};

                  log << "resource connected: " << message << std::endl;

                  try
                  {
                     auto& instance = m_state.get_instance( message.resource, message.process.pid);

                     if( message.state == XA_OK)
                     {
                        instance.process = std::move( message.process);

                        local::instance::done( m_state, instance);

                     }
                     else
                     {
                        common::log::error << "resource proxy: " <<  message.process << " startup error" << std::endl;
                        instance.state( state::resource::Proxy::Instance::State::error);
                        //throw common::exception::signal::Terminate{};
                        // TODO: what to do?
                     }

                  }
                  catch( common::exception::invalid::Argument&)
                  {
                     common::log::error << "unexpected resource connecting: " << message << " - action: discard" << std::endl;

                     log << "resources: " << common::range::make( m_state.resources) << std::endl;
                  }


                  if( ! m_connected && common::range::all_of( common::range::make( m_state.resources), state::filter::Running{}))
                  {
                     //
                     // We now have enough resource proxies up and running to guarantee consistency
                     // notify broker
                     //
                     /*

                     log << "enough resources are connected - send connect to broker\n";

                     common::message::transaction::manager::Ready running;
                     running.process = common::process::handle();
                     ipc::device().blocking_send( common::communication::ipc::broker::device(), running);
                     */

                     m_connected = true;
                  }
               }

               bool basic_prepare::operator () ( message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  Trace trace{ "transaction::handle::resource::prepare reply"};

                  log << "message: " << message << '\n';


                  //
                  // If the resource only did a read only, we 'promote' it to 'not involved'
                  //
                  {
                     resource.result = Transaction::Resource::convert( message.state);

                     if( resource.result == Transaction::Resource::Result::xa_RDONLY)
                     {
                        resource.stage = Transaction::Resource::Stage::not_involved;
                     }
                     else
                     {
                        resource.stage = Transaction::Resource::Stage::prepare_replied;
                     }
                  }

                  //
                  // Are we in a prepared state?
                  //
                  if( transaction.stage() < Transaction::Resource::Stage::prepare_replied)
                  {
                     //
                     // If not, we wait for more replies...
                     //
                     return false;
                  }

                  return transaction.implementation->prepare( m_state, message, transaction);

               }



               bool basic_commit::operator () ( message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  Trace trace{ "transaction::handle::resource::commit reply"};

                  log << "message: " << message << '\n';

                  resource.stage = Transaction::Resource::Stage::commit_replied;


                  //
                  // Are we in a committed state?
                  //
                  if( transaction.stage() < Transaction::Resource::Stage::commit_replied)
                  {
                     //
                     // If not, we wait for more replies...
                     //
                     return false;
                  }

                  return transaction.implementation->commit( m_state, message, transaction);
               }



               bool basic_rollback::operator () ( message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  Trace trace{ "transaction::handle::resource::rollback reply"};

                  log << "message: " << message << '\n';

                  resource.stage = Transaction::Resource::Stage::rollback_replied;

                  //
                  // Are we in a rolled back state?
                  //
                  if( transaction.stage() < Transaction::Resource::Stage::rollback_replied)
                  {

                     return false;
                  }

                  return transaction.implementation->rollback( m_state, message, transaction);
               }
            } // reply
         } // resource


         template< typename Handler>
         void user_reply_wrapper< Handler>::operator () ( typename Handler::message_type& message)
         {
            try
            {
               Handler::operator() ( message);
            }
            catch( const user::error& exception)
            {
               common::error::handler();

               auto reply = local::transform::reply( message);
               reply.stage = decltype( reply)::Stage::error;
               reply.state = exception.code();

               local::send::reply( Handler::m_state, std::move( reply), message.process);
            }
            catch( const common::exception::signal::Terminate&)
            {
               throw;
            }
            catch( ...)
            {
               common::log::error << "unexpected error - action: send reply XAER_RMERR\n";

               common::error::handler();

               auto reply = local::transform::reply( message);
               reply.stage = decltype( reply)::Stage::error;
               reply.state = XAER_RMERR;

               local::send::reply( Handler::m_state, std::move( reply), message.process);
            }
         }


         void basic_commit::operator () ( message_type& message)
         {
            Trace trace{ "transaction::handle::Commit"};

            log << "message: " << message << '\n';

            auto location = local::transaction::find_or_add( m_state, message);
            auto& transaction = *location;

            //
            // Make sure we add the involved resources from the commit message (if any)
            //
            local::resource::involved( m_state, transaction, message);

            switch( transaction.stage())
            {
               case Transaction::Resource::Stage::involved:
               case Transaction::Resource::Stage::not_involved:
               {
                  break;
               }
               default:
               {
                  throw user::error{ XAER_PROTO, "Attempt to commit transaction, which is not in a state for commit", CASUAL_NIP( message.trid)};
               }
            }

            //
            // Local normal commit phase
            //
            transaction.implementation = local::implementation::Local::instance();


            //
            // Only the owner of the transaction can fiddle with the transaction ?
            //


            switch( transaction.resources.size())
            {
               case 0:
               {
                  log << "no resources involved - " << transaction << " XA_RDONLY\n";

                  //
                  // We can remove this transaction
                  //
                  m_state.transactions.erase( std::begin( location));

                  //
                  // Send reply
                  //
                  {
                     auto reply = local::transform::reply( message);
                     reply.state = XA_RDONLY;
                     reply.stage = reply_type::Stage::commit;

                     local::send::reply( m_state, std::move( reply), message.process);
                  }

                  break;
               }
               case 1:
               {
                  //
                  // Only one resource involved, we do a one-phase-commit optimization.
                  //
                  log << "only one resource involved - " << transaction << " TMONEPHASE\n";

                  //
                  // Keep the correlation so we can send correct reply
                  //
                  transaction.correlation = message.correlation;

                  local::send::resource::request< common::message::transaction::resource::commit::Request>(
                     m_state,
                     transaction,
                     Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::involved},
                     Transaction::Resource::Stage::commit_requested,
                     TMONEPHASE
                  );

                  break;
               }
               default:
               {
                  //
                  // More than one resource involved, we do the prepare stage
                  //
                  log << "prepare " << transaction << "\n";

                  //
                  // Keep the correlation so we can send correct reply
                  //
                  transaction.correlation = message.correlation;

                  local::send::resource::request< common::message::transaction::resource::prepare::Request>(
                     m_state,
                     transaction,
                     Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::involved},
                     Transaction::Resource::Stage::prepare_requested
                  );

                  break;
               }
            }
         }

         template struct user_reply_wrapper< basic_commit>;


         void basic_rollback::operator () ( message_type& message)
         {
            Trace trace{ "transaction::handle::Rollback"};

            log << "message: " << message << '\n';

            auto location = local::transaction::find_or_add( m_state, message);
            auto& transaction = *location;

            //
            // Local normal rollback phase
            //
            transaction.implementation = local::implementation::Local::instance();

            //
            // Make sure we add the involved resources from the rollback message (if any)
            //
            local::resource::involved( m_state, transaction, message);

            if( transaction.resources.empty())
            {
               log << "no resources involved - " << transaction << " XA_OK\n";

               //
               // We can remove this transaction.
               //
               m_state.transactions.erase( std::begin( location));

               //
               // Send reply
               //
               {
                  auto reply = local::transform::reply( message);
                  reply.state = XA_OK;

                  local::send::reply( m_state, std::move( reply), message.process);
               }
            }
            else
            {
               //
               // Keep the correlation so we can send correct reply
               //
               transaction.correlation = message.correlation;

               local::send::resource::request< common::message::transaction::resource::rollback::Request>(
                  m_state,
                  transaction,
                  Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::involved},
                  Transaction::Resource::Stage::rollback_requested
               );
            }
         }

         template struct user_reply_wrapper< basic_rollback>;


         namespace external
         {
            void Involved::operator () ( common::message::transaction::resource::external::Involved& message)
            {
               Trace trace{ "transaction::handle::external::Involved"};

               log << "involved message: " << message << '\n';

               auto& transaction = *local::transaction::find_or_add( m_state, message);

               auto id = state::resource::external::proxy::id( m_state, message.process);

               if( ! common::range::find( transaction.resources, id))
               {
                  transaction.resources.emplace_back( id);
               }

               log << "transaction: " << transaction << '\n';
            }
         } // external

         namespace domain
         {


            void Prepare::operator () ( message_type& message)
            {
               Trace trace{ "transaction::handle::domain::prepare request"};

               log << "message: " << message << '\n';

               //
               // Find the transaction
               //
               auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.trid});


               if( found)
               {
                  if( handle( message, *found))
                  {
                     //
                     // We remove the transaction
                     //
                     m_state.transactions.erase( std::begin( found));
                  }
               }
               else
               {
                  //
                  // We don't have the transaction. This could be for two reasons:
                  // 1) We had it, but it was already prepared from another domain, and XA
                  // optimization kicked in ("read only") and the transaction was done
                  // 2) casual have made some optimizations.
                  // Either way, we don't have it, so we just reply that it has been prepared with "read only"
                  //

                  log << "XA_RDONLY transaction (" << message.trid << ") either does not exists (longer) in this domain or there are no resources involved - action: send prepare-reply (read only)\n";

                  local::send::read_only( m_state, message);
               }
            }

            bool Prepare::handle( message_type& message, Transaction& transaction)
            {
               Trace trace{ "transaction::handle::domain::Prepare::handle"};

               log << "transaction: " << transaction << '\n';

               //
               // We can only get this message if a 'user commit' has
               // been invoked somewhere.
               //
               // The transaction can only be in one of two states.
               //
               // 1) The prepare phase has already started, either by this domain
               //    or another domain, either way the work has begun, and we don't
               //    need to do anything for this particular request.
               //
               // 2) Some other domain has received a 'user commit' and started the
               //    prepare phase. This domain should carry out the request on behalf
               //    of the other domain.
               //

               if( transaction.implementation)
               {
                  //
                  // state 1:
                  //
                  // The commit/rollback phase has already started, we send
                  // read only and let the phase take it's course
                  //
                  local::send::read_only( m_state, message);
               }
               else
               {
                  //
                  // state 2:
                  //

                  transaction.implementation = local::implementation::Remote::instance();

                  switch( transaction.stage())
                  {
                     case Transaction::Resource::Stage::involved:
                     {
                        //
                        // We're in the second state. We change the owner to the inbound gateway that
                        // sent the request, so we know where to send the accumulated reply.
                        //
                        transaction.trid.owner( message.process);
                        transaction.correlation = message.correlation;
                        transaction.resource = message.resource;

                        local::send::resource::request< common::message::transaction::resource::prepare::Request>(
                           m_state,
                           transaction,
                           Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::involved},
                           Transaction::Resource::Stage::prepare_requested,
                           message.flags
                        );

                        break;
                     }
                     case Transaction::Resource::Stage::not_involved:
                     {
                        local::send::read_only( m_state, message);

                        //
                        // We remove the transaction
                        //
                        return true;
                     }
                     default:
                     {
                        common::log::error << "unexpected transaction stage: " << transaction << '\n';
                        break;
                     }
                  }
               }
               return false;
            }

            void Commit::operator () ( message_type& message)
            {
               Trace trace{ "transaction::handle::domain::commit request"};

               log << "message: " << message << '\n';

               //
               // Find the transaction
               //
               auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.trid});

               if( found)
               {
                  handle( message, *found);
               }
               else
               {

                  log << "XAER_NOTA trid: " << message.trid << " is not known to this TM - action: send XAER_NOTA reply\n";

                  //
                  // Send reply
                  //
                  {
                     auto reply = local::transform::reply( message);
                     reply.state = XAER_NOTA;
                     reply.resource = message.resource;

                     local::send::reply( m_state, std::move( reply), message.process);
                  }
               }
            }

            void Commit::handle( message_type& message, Transaction& transaction)
            {
               Trace trace{ "transaction::handle::domain::Commit::handle"};

               transaction.correlation = message.correlation;

               log << "transaction: " << transaction << '\n';

               if( transaction.implementation)
               {
                  //
                  // We've completed the prepare stage, now it's time for the commit stage
                  //

                  local::send::resource::request< common::message::transaction::resource::commit::Request>(
                     m_state,
                     transaction,
                     Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::prepare_replied},
                     Transaction::Resource::Stage::commit_requested,
                     message.flags
                  );
               }
               else
               {
                  //
                  // It has to be a one phase commit optimization.
                  //
                  if( ! common::flag< TMONEPHASE>( message.flags))
                  {
                     auto reply = local::transform::reply( message);
                     reply.state = XAER_PROTO;
                     reply.resource = message.resource;

                     local::send::reply( m_state, std::move( reply), message.process);
                     return;
                  }

                  transaction.implementation = local::implementation::one::phase::commit::Remote::instance();

                  //
                  // Make sure we can send enough stuff so remote domain can correlate,
                  // when we actually send the accumulated reply
                  //
                  transaction.trid.owner( message.process);
                  transaction.correlation = message.correlation;
                  transaction.resource = message.resource;

                  if( transaction.resources.size() > 1)
                  {
                     local::send::resource::request< common::message::transaction::resource::prepare::Request>(
                        m_state,
                        transaction,
                        Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::involved},
                        Transaction::Resource::Stage::prepare_requested,
                        TMNOFLAGS
                     );
                  }
                  else
                  {
                     local::send::resource::request< common::message::transaction::resource::commit::Request>(
                        m_state,
                        transaction,
                        Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::involved},
                        Transaction::Resource::Stage::commit_requested,
                        message.flags
                     );
                  }

               }
            }

            void Rollback::operator () ( message_type& message)
            {
               Trace trace{ "transaction::handle::domain::rollback request"};

               log << "message: " << message << '\n';


               //
               // Find the transaction
               //
               auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.trid});

               if( found)
               {
                  auto& transaction = *found;
                  log << "transaction: " << transaction << '\n';

                  local::send::resource::request< common::message::transaction::resource::rollback::Request>(
                     m_state,
                     transaction,
                     Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::prepare_replied},
                     Transaction::Resource::Stage::rollback_requested,
                     message.flags
                  );
               }
               else
               {
                  log << "XAER_NOTA trid: " << message.trid << " is not known to this TM - action: send XAER_NOTA reply\n";

                  //
                  // Send reply
                  //
                  {
                     auto reply = local::transform::reply( message);
                     reply.state = XAER_NOTA;
                     reply.resource = message.resource;

                     local::send::reply( m_state, std::move( reply), message.process);
                  }
               }
            }
         } // domain



         namespace resource
         {
            namespace reply
            {
               template struct Wrapper< basic_prepare>;
               template struct Wrapper< basic_commit>;
               template struct Wrapper< basic_rollback>;

            } // reply

         }


      } // handle
   } // transaction

} // casual
