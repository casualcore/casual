//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/event/dispatch.h"
#include "casual/event/common.h"

#include "common/event/listen.h"
#include "common/communication/ipc.h"

#include "common/code/casual.h"


namespace casual
{
   using namespace common;
   namespace event
   {
      inline namespace v1 {
      
      namespace detail
      {
         namespace local
         {
            namespace
            {
               namespace transform
               {

                  template< typename R> 
                  auto metric(common::message::event::service::Metric&& metric)
                  {
                     R result;
                     metric.execution.underlying().copy( result.execution);
                     result.service.name = std::move( metric.service);
                     result.service.parent = std::move( metric.parent.service);
                     result.process.pid = metric.process.pid.value();
                     metric.process.ipc.value().copy( result.process.ipc);

                     result.start = std::move( metric.start);
                     result.end = std::move( metric.end);

                     result.pending = std::move( metric.pending);

                     result.code = std::to_underlying( metric.code.result);

                     return result;
                  }
               } // transform
            } // <unnamed>
         } // local

         struct Dispatch::Implementation 
         {
            using handler_type = decltype( message::dispatch::handler( communication::ipc::inbound::device()));

            void start() 
            {
               struct
               {
                  bool done = false;
               } state;


               auto handle_shutdown = [&state]( const message::shutdown::Request&){ state.done = true;};
               auto done = [&state](){ return state.done;};

               if( empty)
                  common::event::listen( 
                     common::event::condition::compose( 
                        common::event::condition::idle( std::move( empty)),
                        common::event::condition::done( std::move( done))),
                     std::move( handler) + handler_type{ std::move( handle_shutdown)});
               else 
                  common::event::listen( 
                     common::event::condition::compose( common::event::condition::done( std::move( done))),
                     std::move( handler) + handler_type{ std::move( handle_shutdown)});
            }

            template< typename T> 
            void add_callback( T&& callback)
            {
               Trace trace{ "event::detail::Dispatch::Implementation::add_callback"};

               auto handle = [ callback = std::forward< T>( callback)]( common::message::event::service::Calls& message)
               {
                  Trace trace{ "event::detail::Dispatch::Implementation callback message::event::service::Calls"};
                  log::line( verbose::log, "message: ", message);
                  
                  auto transform_metric = []( common::message::event::service::Metric& metric)
                  {
                     using Metric = model::service::Call::Metric;
                     
                     auto type = metric.type == decltype( metric.type)::concurrent ? 
                        decltype( Metric::service.type)::concurrent : decltype( Metric::service.type)::sequential;

                     auto result = local::transform::metric< model::service::Call::Metric>( std::move( metric));
                     result.service.type = type;

                     return result;
                  };

                  model::service::Call event;
                  algorithm::transform( message.metrics, event.metrics, transform_metric);
                  
                  callback( std::move( event));
               };

               handler.insert( std::move( handle));
            }

            template< typename T> 
            void add_deprecated( T&& callback)
            {
               auto handle = [ callback = std::forward< T>( callback)]( common::message::event::service::Calls& message)
               {
                  Trace trace{ "event::detail::Implementation::handle message::event::service::Calls"};
                  log::line( verbose::log, "message: ", message);
                  
                  auto transform_metric = []( common::message::event::service::Metric& metric)
                  {
                     return local::transform::metric< model::v1::service::Call::Metric>( std::move( metric));
                  };
                  model::v1::service::Call event;
                  algorithm::transform( message.metrics, event.metrics, transform_metric);
                  
                  callback( std::move( event));
               };

               handler.insert( std::move( handle));
            }

            handler_type handler;
            std::function< void()> empty;
         };

         Dispatch::Dispatch() 
            : m_implementation{ std::make_unique< Implementation>()} {}  

         Dispatch::Dispatch( Dispatch&&) noexcept = default;
         Dispatch& Dispatch::operator = ( Dispatch&&) noexcept = default;
         Dispatch::~Dispatch() = default;

         void Dispatch::start()
         {
            Trace trace{ "event::detail::Dispatch::start"};

            m_implementation->start();           
         }
         
         void Dispatch::add( std::function< void( const model::service::Call&)> callback)
         {
            m_implementation->add_callback( std::move( callback));
         }

         void Dispatch::add( std::function< void( model::service::Call&&)> callback)
         {
            m_implementation->add_callback( std::move( callback));
         }

         void Dispatch::add( std::function< void()> empty)
         {
            m_implementation->empty = std::move( empty);
         }
         
         // deprecated
         void Dispatch::add( std::function< void( const model::v1::service::Call&)> callback)
         {
            m_implementation->add_deprecated( std::move( callback));
         }

         // deprecated
         void Dispatch::add( std::function< void( model::v1::service::Call&&)> callback)
         {
            m_implementation->add_deprecated( std::move( callback));
         }         

      } // detail

      } // v1
   } // event
} // casual