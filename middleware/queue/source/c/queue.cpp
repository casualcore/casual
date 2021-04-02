//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/queue/c/queue.h"

#include "queue/api/queue.h"
#include "queue/common/log.h"
#include "queue/code.h"

#include "casual/platform.h"
#include "common/exception/handle.h"
#include "common/value/optional.h"
#include "common/algorithm.h"
#include "common/string.h"
#include "common/log/category.h"
#include "common/code/casual.h"
#include "common/code/category.h"

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
               queue::code code = queue::code::ok;
            } // global
            
            template< typename F, typename T, typename... Ts>
            auto wrap( F&& functor, T error, Ts&&... ts) -> decltype( functor( std::forward< Ts>( ts)...))
            {
               try 
               {
                  global::code = queue::code::ok;
                  return functor( std::forward< Ts>( ts)...);
               }
               catch( ...)
               {
                  auto error = common::exception::capture();

                  if( common::code::is::category< queue::code>( error.code()))
                  {
                     global::code = static_cast< queue::code>( error.code().value());
                  }
                  else
                  {
                     common::log::line( common::log::category::error, common::code::casual::internal_unexpected_value, " unexpected error: ", error);
                     global::code = queue::code::system;
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

                  queue::raise( queue::code::argument);
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
                  namespace tag
                  {
                     struct type{};
                  } // tag
         
                  using id = common::value::Optional< long, -1l, tag::type>;
               
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
                     
                  } // reply

                  namespace available
                  {
                     auto set( descriptor::id descriptor, std::chrono::milliseconds time)
                     {
                        global::cache.get( descriptor).value.attributes.available = platform::time::point::type{ time};
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
                     global::cache.get( descriptor).value.id = common::Uuid{ *id};
                     return 0;
                  }
                  
                  auto get( descriptor::id descriptor, uuid_t* id)
                  {
                     global::cache.get( descriptor).value.id.copy( *id);
                     return 0;
                  }
               } // id
            } // message

            namespace selector
            {
               namespace descriptor
               {
                  namespace tag
                  {
                     struct type{};
                  } // tag
         
                  using id = common::value::Optional< long, -1l, tag::type>;
               
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
               auto& message = message::global::cache.get( descriptor);
               message.value.id = queue::xatmi::enqueue( queue, message.value);
               return 0;
            }

            auto dequeue( const char* queue, selector::descriptor::id selector)
            {
               auto message = []( auto queue, auto selector)
               {
                  if( selector)
                     return queue::xatmi::dequeue( queue, selector::global::cache.get( selector).value);
                  else
                     return queue::xatmi::dequeue( queue);
               }( queue, selector);

               if( message.empty())
                  queue::raise( queue::code::no_message);

               return local::message::global::cache.add( std::move( message.front())).id.value();
            }

         } // <unnamed>
      } // local         
   } // queue
} // casual

extern "C" 
{
   using namespace casual::queue;

   int casual_queue_get_errno()
   {
      return casual::common::cast::underlying( local::global::code);
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

   int casual_queue_message_attribute_set_available( casual_message_descriptor_t message, long ms_since_epoc)
   {
      return local::wrap( local::message::attribute::available::set, -1, local::message::descriptor::id{ message}, std::chrono::milliseconds{ ms_since_epoc});
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

} // extern C
