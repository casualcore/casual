//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/outbound/error/reply.h"
#include "gateway/common.h"
#include "gateway/message.h"


#include "common/code/xa.h"
#include "common/communication/ipc.h"

namespace casual
{
   using namespace common;

   namespace gateway::group::outbound::error::reply
   {
      namespace local
      {
         namespace
         {
            namespace send
            {
               template< typename M>
               void empty( const state::route::Point& point)
               {
                  M message;
                  message.correlation = point.correlation;

                  communication::device::blocking::optional::send( point.process.ipc, message);
               }
               
               template< typename M>
               void transaction( const state::route::Point& point)
               {
                  M message;
                  message.correlation = point.correlation;
                  message.state = common::code::xa::resource_fail;

                  communication::device::blocking::optional::send( point.process.ipc, message);
               }

               template< typename M>
               void queue( const state::route::Point& point)
               {
                  send::empty< M>( point);
               }  
            } // send

         } // <unnamed>
      } // local


      void point( const state::route::Point& point)
      {
         Trace trace{ "gateway::group::outbound::error::reply::point"};
         log::line( verbose::log, "point: ", point);

         switch( point.type)
         {
            case common::message::transaction::resource::prepare::Request::type(): 
            {
               local::send::transaction< common::message::transaction::resource::prepare::Reply>( point);
               break;
            }
            case common::message::transaction::resource::commit::Request::type(): 
            {
               local::send::transaction< common::message::transaction::resource::commit::Reply>( point);
               break;
            }
            case common::message::transaction::resource::rollback::Request::type(): 
            {
               local::send::transaction< common::message::transaction::resource::rollback::Reply>( point);
               break;
            }
            case casual::queue::ipc::message::group::enqueue::Request::type(): 
            {
               local::send::queue< casual::queue::ipc::message::group::enqueue::Reply>( point);
               break;
            }
            case casual::queue::ipc::message::group::dequeue::Request::type(): 
            {
               local::send::queue< casual::queue::ipc::message::group::dequeue::Reply>( point);
               break;
            }
            case message::domain::connect::Request::type():
            {
               break; // no op.
            }
            default: 
            {
               log::line( log::category::error, "unexpected route point: ", point);
               break;
            }
         }
      }

      void point( const state::route::service::Point& point)
      {
         Trace trace{ "gateway::group::outbound::error::reply::point service"};
         log::line( verbose::log, "point: ", point);

         common::message::service::call::Reply message;

         message.correlation = point.correlation;
         message.code.result = common::code::xatmi::system;

         communication::device::blocking::optional::send( point.process.ipc, message);
      }


   } // gateway::group::outbound::error::reply
} // casual