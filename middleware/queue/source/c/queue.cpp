//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/queue/c/queue.h"

#include "queue/api/queue.h"
#include "queue/common/log.h"

#include "casual/platform.h"
#include "common/exception/capture.h"
#include "common/strong/type.h"
#include "common/algorithm.h"
#include "common/string.h"
#include "common/log/category.h"
#include "common/code/queue.h"
#include "common/code/casual.h"
#include "common/code/category.h"
#include "common/buffer/type.h"
#include "common/buffer/pool.h"
#include "common/execute.h"
#include "common/code/queue.h"
#include "common/code/signal.h"

namespace casual
{
   namespace queue
   {
      namespace local
      {
         namespace
         {
            namespace global
            {
               auto code = common::code::queue::ok;
            } // global
            
            template< typename F, typename T, typename... Ts>
            auto wrap( F&& functor, T error, Ts&&... ts) -> decltype( functor( std::forward< Ts>( ts)...))
            {
               try 
               {
                  global::code = common::code::queue::ok;
                  return functor( std::forward< Ts>( ts)...);
               }
               catch( ...)
               {
                  auto error = common::exception::capture();

                  if( common::code::is::category< common::code::queue>( error.code()))
                  {
                     global::code = static_cast< common::code::queue>( error.code().value());
                  }
                  else if( common::code::is::category< common::code::signal>( error.code()))
                  {
                     global::code = common::code::queue::signaled;
                  }
                  else
                  {
                     common::log::line( common::log::category::error, common::code::casual::internal_unexpected_value, " unexpected error: ", error);
                     global::code = common::code::queue::system;
                  }
               }

               return error;
            }

            template< typename I, typename V>
            struct basic_descriptor
            {
               using id_type = I;
               using value_type = V;

               struct Holder 
               {
                  Holder() = default;
                  Holder( id_type id, value_type value)
                     : id{ id}, value{ std::move( value)} {}

                  id_type id;
                  value_type value;
               };

               Holder& get( id_type descriptor)
               {
                  auto is_descriptor = [descriptor]( auto& holder ){ return holder.id == descriptor;};

                  if( auto found = common::algorithm::find_if( m_cache, is_descriptor))
                     return *found;

                  common::code::raise::error( common::code::queue::argument);
               }

               Holder& add( value_type value)
               {
                  auto found = common::algorithm::find_if( m_cache, []( auto& m){ return ! m.id;});

                  if( found)
                  {
                     found->id = id_type( std::distance( std::begin( m_cache), std::begin( found)));
                     found->value = std::move( value);
                     return *found;
                  }
                  else 
                  {
                     m_cache.emplace_back( id_type( m_cache.size()), std::move( value));
                     return m_cache.back();
                  }
               }

               void erase( id_type descriptor)
               {
                  get( descriptor) = Holder{};
               }

               private:
                  std::vector< Holder> m_cache;
            };

            namespace message
            {
               namespace descriptor
               {
                  struct policy
                  {
                     constexpr static long initialize() { return -1;}
                     constexpr static bool valid( long value) { return value != initialize();}
                  };
         
                  using id = common::strong::Type< long, policy>;
               
               } // descriptor

               namespace global
               {
                  basic_descriptor< descriptor::id, casual::queue::xatmi::Message> cache;
               } // global

               auto create( casual_buffer_t buffer)
               {
                  return global::cache.add( casual::queue::xatmi::Message{ casual::queue::xatmi::Payload{ buffer.data, buffer.size}}).id.value();
               }
               
               auto clean( descriptor::id descriptor)
               {
                  global::cache.erase( descriptor);
                  return 0;
               }

               namespace attribute
               {
                  namespace properties
                  {
                     auto set( descriptor::id descriptor, const char* properties)
                     {
                        global::cache.get( descriptor).value.attributes.properties = properties;
                        return 0;
                     }

                     auto get( descriptor::id descriptor)
                     {
                        return global::cache.get( descriptor).value.attributes.properties.c_str();
                     }
                  } // properties

                  namespace reply
                  {
                     auto set( descriptor::id descriptor, const char* queue)
                     {
                        global::cache.get( descriptor).value.attributes.reply = queue;
                        return 0;
                     }

                     auto get( descriptor::id descriptor)
                     {
                        return global::cache.get( descriptor).value.attributes.reply.c_str();
                     }
                     
                  } // reply

                  namespace available
                  {
                     auto set( descriptor::id descriptor, std::chrono::milliseconds time)
                     {
                        global::cache.get( descriptor).value.attributes.available = platform::time::point::type{ time};
                        return 0;
                     }

                     auto get( descriptor::id descriptor, long* ms_since_epoch)
                     {
                        if( ! ms_since_epoch)
                           common::code::raise::error( common::code::queue::argument);

                        *ms_since_epoch = std::chrono::duration_cast< std::chrono::milliseconds>( global::cache.get( descriptor).value.attributes.available.time_since_epoch()).count();
                        return 0;
                     }
                  } // available
               } // attribute

               namespace buffer
               {
                  auto set( descriptor::id descriptor, casual_buffer_t buffer)
                  {
                     global::cache.get( descriptor).value.payload = casual::queue::xatmi::Payload{ buffer.data, buffer.size};
                     return 0;
                  }

                  auto get( descriptor::id descriptor, casual_buffer_t* buffer)
                  {
                     auto& payload = global::cache.get( descriptor).value.payload;
                     buffer->data = payload.buffer;
                     buffer->size = payload.size;
                     return 0;
                  }
                  
               } // buffer

               namespace id
               {
                  auto set( descriptor::id descriptor, const uuid_t* id)
                  {
                     if( ! id)
                        common::code::raise::error( common::code::queue::argument);

                     global::cache.get( descriptor).value.id = common::Uuid{ *id};
                     return 0;
                  }
                  
                  auto get( descriptor::id descriptor, uuid_t* id)
                  {
                     if( ! id)
                        common::code::raise::error( common::code::queue::argument);
                        
                     global::cache.get( descriptor).value.id.copy( *id);
                     return 0;
                  }
               } // id
            } // message

            namespace selector
            {
               namespace descriptor
               {
                  struct policy
                  {
                     constexpr static long initialize() { return -1;}
                     constexpr static bool valid( long value) { return value != initialize();}
                  };
         
                  using id = common::strong::Type< long, policy>;
               
               } // descriptor

               namespace global
               {
                  basic_descriptor< descriptor::id, casual::queue::Selector> cache;
               } // global

               auto create()
               {
                  return global::cache.add( {}).id.value();
               }

               auto clear( descriptor::id descriptor)
               {
                  global::cache.erase( descriptor);
                  return 0;
               }

               namespace properties
               {
                  auto set( descriptor::id descriptor, const char* properties)
                  {
                     global::cache.get( descriptor).value.properties = properties;
                     return 0;
                  }

               } // properties

               namespace id
               {
                  auto set( descriptor::id descriptor, const uuid_t* id)
                  {
                     global::cache.get( descriptor).value.id = common::Uuid{ *id};
                     return 0;
                  }
               } // id
               
            } // selector

            auto enqueue( const char* queue, message::descriptor::id descriptor)
            {
               Trace trace{ "queue::local::enqueue"};

               auto& message = message::global::cache.get( descriptor);
               message.value.id = queue::xatmi::enqueue( queue, message.value);
               return 0;
            }

            auto dequeue( const char* queue, selector::descriptor::id selector)
            {
               Trace trace{ "queue::local::dequeue"};

               auto message = []( auto queue, auto selector)
               {
                  if( selector)
                     return queue::xatmi::dequeue( queue, selector::global::cache.get( selector).value);
                  else
                     return queue::xatmi::dequeue( queue);
               }( queue, selector);

               if( message.empty())
                  common::code::raise::error( common::code::queue::no_message);

               return local::message::global::cache.add( std::move( message.front())).id.value();
            }

            auto peek( const char* queue, selector::descriptor::id selector)
            {
               Trace trace{ "queue::local::peek"};

               auto information = []( auto queue, auto selector)
               {
                  if( selector)
                     return queue::peek::information( queue, selector::global::cache.get( selector).value);
                  else
                     return queue::peek::information( queue);
               }( queue, selector);

               if( information.empty())
                  common::code::raise::error( common::code::queue::no_message);

               auto message = peek::messages( queue, { information.front().id});
               if( message.empty())
                  common::code::raise::error( common::code::queue::no_message);

               xatmi::Message result;
               {
                  result.id = std::move( message.front().id);
                  result.attributes = std::move( message.front().attributes);

                  common::buffer::Payload payload;
                  payload.type = std::move( message.front().payload.type);
                  payload.data = std::move( message.front().payload.data);

                  auto buffer = common::buffer::pool::holder().insert( std::move( payload));
                  result.payload.buffer = std::get< 0>( buffer).raw();
                  result.payload.size = std::get< 1>( buffer);
               }
               return local::message::global::cache.add( std::move( result)).id.value();
            }

            namespace browse
            {
               auto peek( const char* queue,common::unique_function< int( casual_selector_descriptor_t, void*) const> callback, void* state)
               {
                  Trace trace{ "queue::local::browse::peek"};                  

                  casual::queue::browse::peek( queue, [ callback = std::move( callback), state]( auto&& message)
                  {
                     xatmi::Message result;
                     {
                        result.id = std::move( message.id);
                        result.attributes = std::move( message.attributes);

                        common::buffer::Payload payload;
                        payload.type = std::move( message.payload.type);
                        payload.data = std::move( message.payload.data);

                        auto buffer = common::buffer::pool::holder().insert( std::move( payload));
                        result.payload.buffer = std::get< 0>( buffer).raw();
                        result.payload.size = std::get< 1>( buffer);
                     }

                     auto id = local::message::global::cache.add( std::move( result)).id;

                     auto erase_scope = common::execute::scope( [ id](){ local::message::global::cache.erase( id);});

                     return callback( id.value(), state) != 0;
                  });


                  return 0;
               }
            } // browse

         } // <unnamed>
      } // local         

      extern "C" 
      {
         const char* casual_queue_error_string( int code)
         {
            switch( code)
            {
               case CASUAL_QE_OK: return "CASUAL_QE_OK";
               case CASUAL_QE_NO_MESSAGE: return "CASUAL_QE_NO_MESSAGE";
               case CASUAL_QE_NO_QUEUE : return "CASUAL_QE_NO_QUEUE";
               case CASUAL_QE_INVALID_ARGUMENTS: return "CASUAL_QE_INVALID_ARGUMENTS"; 
               case CASUAL_QE_SYSTEM: return "CASUAL_QE_SYSTEM";
               case CASUAL_QE_SIGNALED: return "CASUAL_QE_SIGNALED";
            }

            return "<unknown>";
         }

         int casual_queue_get_errno()
         {
            return std::to_underlying( local::global::code);
         }

         casual_message_descriptor_t casual_queue_message_create( casual_buffer_t buffer)
         {
            return local::wrap( local::message::create, -1l, buffer);
         }

         int casual_queue_message_attribute_set_properties( casual_message_descriptor_t message, const char* properties)
         {
            return local::wrap( local::message::attribute::properties::set, -1, local::message::descriptor::id{ message}, properties);
         }

         const char* casual_queue_message_attribute_get_properties( casual_message_descriptor_t message)
         {
            return local::wrap( local::message::attribute::properties::get, nullptr, local::message::descriptor::id{ message});
         }

         int casual_queue_message_attribute_set_reply( casual_message_descriptor_t message, const char* queue)
         {
            return local::wrap( local::message::attribute::reply::set, -1, local::message::descriptor::id{ message}, queue);
         }

         const char* casual_queue_message_attribute_get_reply( casual_message_descriptor_t message)
         {
            return local::wrap( local::message::attribute::reply::get, nullptr, local::message::descriptor::id{ message});
         }

         int casual_queue_message_attribute_set_available( casual_message_descriptor_t message, long ms_since_epoch)
         {
            return local::wrap( local::message::attribute::available::set, -1, local::message::descriptor::id{ message}, std::chrono::milliseconds{ ms_since_epoch});
         }

         int casual_queue_message_attribute_get_available( casual_message_descriptor_t message, long* ms_since_epoch)
         {
            return local::wrap( local::message::attribute::available::get, -1, local::message::descriptor::id{ message}, ms_since_epoch);
         }

         int casual_queue_message_set_buffer( casual_message_descriptor_t message, casual_buffer_t buffer)
         {
            return local::wrap( local::message::buffer::set, -1, local::message::descriptor::id{ message}, buffer);
         }

         int casual_queue_message_get_buffer( casual_message_descriptor_t message, casual_buffer_t* buffer)
         {
            return local::wrap( local::message::buffer::get, -1, local::message::descriptor::id{ message}, buffer);
         }

         int casual_queue_message_set_id( casual_message_descriptor_t message, const uuid_t* id)
         {
            return local::wrap( local::message::id::set, -1, local::message::descriptor::id{ message}, id);
         }

         int casual_queue_message_get_id( casual_message_descriptor_t message, uuid_t* id)
         {
            return local::wrap( local::message::id::get, -1, local::message::descriptor::id{ message}, id);
         }

         int casual_queue_message_delete( casual_message_descriptor_t message)
         {
            return local::wrap( local::message::clean, -1, local::message::descriptor::id{ message});
         }

         casual_selector_descriptor_t casual_queue_selector_create()
         {
            return local::wrap( local::selector::create, -1l);
         }
         
         int casual_queue_selector_delete( casual_selector_descriptor_t selector)
         {
            return local::wrap( local::selector::clear, -1, local::selector::descriptor::id{ selector});
         }
         
         int casual_queue_selector_set_properties( casual_selector_descriptor_t selector, const char* properties)
         {
            return local::wrap( local::selector::properties::set, -1, local::selector::descriptor::id{ selector}, properties);
         }

         int casual_queue_selector_set_id( casual_selector_descriptor_t selector, const uuid_t* id)
         {
            return local::wrap( local::selector::id::set, -1, local::selector::descriptor::id{ selector}, id);
         }

         int casual_queue_enqueue( const char* queue, const casual_message_descriptor_t message)
         {
            return local::wrap( local::enqueue, -1, queue, local::message::descriptor::id{ message});
         }

         casual_message_descriptor_t casual_queue_dequeue( const char* queue, casual_selector_descriptor_t selector)
         {
            return local::wrap( local::dequeue, -1l, queue, local::selector::descriptor::id{ selector});
         }

         casual_message_descriptor_t casual_queue_peek( const char* queue, casual_selector_descriptor_t selector)
         {
            return local::wrap( local::peek, -1l, queue, local::selector::descriptor::id{ selector});
         }

         //int casual_queue_browse_peek( const char* queue, int (*callback)( casual_selector_descriptor_t))
         int casual_queue_browse_peek( const char* queue, ::casual_browse_callback_t callback, void* state)
         {
            return local::wrap( local::browse::peek, -1, queue, callback, state);
         }

      } // extern C

   } // queue
} // casual
