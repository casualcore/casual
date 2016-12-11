//!
//! casual 
//!

#include "gateway/outbound/gateway.h"


namespace casual
{
   namespace gateway
   {
      namespace outbound
      {
         namespace error
         {

            namespace local
            {
               namespace
               {
                  namespace reply
                  {

                     template< typename M>
                     struct basic_transaction
                     {
                        void operator () ( const Routing::Point& point) const
                        {
                           M message;
                           message.correlation = point.correlation;
                           message.state = XAER_RMFAIL;

                           ipc::optional::send( point.destination.queue, message);
                        }
                     };

                     void send( const Routing::Point& point)
                     {
                        Trace trace{ "gateway::outbound::error::reply::send"};

                        static const std::map< common::message::Type, std::function<void( const Routing::Point&)>> dispatch{
                           // transactions
                           {
                              common::message::transaction::resource::prepare::Request::type(),
                              basic_transaction< common::message::transaction::resource::prepare::Reply>{}
                           },
                           {
                              common::message::transaction::resource::commit::Request::type(),
                              basic_transaction< common::message::transaction::resource::commit::Reply>{}
                           },
                           {
                              common::message::transaction::resource::rollback::Request::type(),
                              basic_transaction< common::message::transaction::resource::rollback::Reply>{}
                           },
                           // call
                           {
                              common::message::service::call::callee::Request::type(),
                              []( const Routing::Point& point){
                                 common::message::service::call::Reply reply;

                                 reply.correlation = point.correlation;
                                 reply.error = TPESYSTEM;

                                 ipc::optional::send( point.destination.queue, reply);
                              }
                           },
                           // domain discover
                           {
                              common::message::gateway::domain::discover::Request::type(),
                              []( const Routing::Point& point){
                                 common::message::gateway::domain::discover::Reply reply;

                                 reply.correlation = point.correlation;

                                 ipc::optional::send( point.destination.queue, reply);
                              }
                           },
                           // queue
                           {
                              common::message::queue::enqueue::Request::type(),
                              []( const Routing::Point& point){
                                 common::message::queue::enqueue::Reply reply;

                                 reply.correlation = point.correlation;

                                 ipc::optional::send( point.destination.queue, reply);
                              }
                           },
                           {
                              common::message::queue::dequeue::Request::type(),
                              []( const Routing::Point& point){
                                 common::message::queue::dequeue::Reply reply;

                                 reply.correlation = point.correlation;

                                 ipc::optional::send( point.destination.queue, reply);
                              }
                           },
                        };

                        dispatch.at( point.type)( point);

                     }

                     struct Send
                     {
                        void operator () ( const Routing::Point& point) const
                        {
                           try
                           {
                              send( point);
                           }
                           catch( ...)
                           {
                              common::error::handler();
                           }
                        }
                     };

                  } // reply

               } // <unnamed>
            } // local

            Reply::Reply( const Routing& routing) : m_routing( routing) {}

            Reply::~Reply()
            {
               try
               {
                  auto pending = m_routing.extract();

                  log << "pending requests: " << common::range::make( pending) << '\n';

                  common::range::for_each( pending, local::reply::Send{});
               }
               catch( ...)
               {
                  common::error::handler();
               }
            }

         } // error

      } // outbound

   } // gateway



} // casual
