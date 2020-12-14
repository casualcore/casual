//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/outbound/error/reply.h"
#include "gateway/common.h"

#include "common/code/xa.h"
#include "common/communication/ipc.h"

#include "common/message/transaction.h"
#include "common/message/queue.h"
#include "common/message/gateway.h"

namespace casual
{
   using namespace common;
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
                  namespace blocking
                  {
                     template< typename D, typename M>
                     void send( D&& device, M&& message)
                     {
                        communication::device::blocking::send( std::forward< D>( device), std::forward< M>( message));
                     }

                     namespace optional
                     {
                        template< typename D, typename M>
                        bool send( D&& device, M&& message)
                        {
                           return ! communication::device::blocking::optional::send( std::forward< D>( device), std::forward< M>( message)).empty();
                        }
                     } // optional
                  }

                  template< typename M>
                  void send_empty( const route::Point& point)
                  {
                     M message;
                     message.correlation = point.correlation;

                     blocking::optional::send( point.destination.ipc, message);
                  }
                  
                  template< typename M>
                  void send_transaction( const route::Point& point)
                  {
                     M message;
                     message.correlation = point.correlation;
                     message.state = common::code::xa::resource_fail;

                     blocking::optional::send( point.destination.ipc, message);
                  }

                  template< typename M>
                  void send_queue( const route::Point& point)
                  {
                     send_empty< M>( point);
                  }



                  
                  void point( const route::Point& point)
                  {
                     switch( point.type)
                     {
                        case common::message::transaction::resource::prepare::Request::type(): 
                        {
                           send_transaction< common::message::transaction::resource::prepare::Reply>( point);
                           break;
                        }
                        case common::message::transaction::resource::commit::Request::type(): 
                        {
                           send_transaction< common::message::transaction::resource::commit::Reply>( point);
                           break;
                        }
                        case common::message::transaction::resource::rollback::Request::type(): 
                        {
                           send_transaction< common::message::transaction::resource::rollback::Reply>( point);
                           break;
                        }
                        case common::message::queue::enqueue::Request::type(): 
                        {
                           send_queue< common::message::queue::enqueue::Reply>( point);
                           break;
                        }
                        case common::message::queue::dequeue::Request::type(): 
                        {
                           send_queue< common::message::queue::dequeue::Reply>( point);
                           break;
                        }
                        case common::message::gateway::domain::discover::Request::type():
                        {
                           send_empty< common::message::gateway::domain::discover::Reply>( point);
                           break;
                        }
                        default: 
                        {
                           log::line( log::category::error, "unexpected route point: ", point);
                           break;
                        }
                     }
                  }

                  void point( const route::service::Point& point)
                  {
                     common::message::service::call::Reply message;

                     message.correlation = point.correlation;
                     message.code.result = common::code::xatmi::system;

                     blocking::optional::send( point.destination.ipc, message);
                  }

                  template< typename R> 
                  void route( const R& route)
                  {
                     algorithm::for_each( route.points(), []( const auto& point){ local::point( point);});
                  }
               } // <unnamed>
            } // local

            void reply( const route::Route& route)
            {
               Trace trace{ "gateway::outbound::error::reply route::Route"};

               local::route( route);

            }
            void reply( const route::service::Route& route)
            {
               Trace trace{ "gateway::outbound::error::reply route::service::Route"};
               
               local::route( route);
            }

         } // error
      } // outbound
   } // gateway
} // casual