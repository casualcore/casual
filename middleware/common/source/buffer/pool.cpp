//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/buffer/pool.h"
#include "common/algorithm.h"
#include "common/exception/xatmi.h"
#include "common/log.h"

#include <functional>


namespace casual
{

   namespace common
   {
      namespace buffer
      {
         namespace pool
         {

            Holder::Base& Holder::find( const std::string& type)
            {
               auto pool = algorithm::find_if( m_pools, [&]( std::unique_ptr< Base>& b){
                  return b->manage( type);
               });

               if( ! pool)
               {
                  throw exception::xatmi::buffer::type::Input{};
               }
               return **pool;
            }

            Holder::Base& Holder::find( platform::buffer::raw::immutable::type handle)
            {
               auto pool = algorithm::find_if( m_pools, [&]( std::unique_ptr< Base>& b){
                  return b->manage( handle);
               });

               if( ! pool)
               {
                  throw exception::xatmi::invalid::Argument{ "buffer not valid"};
               }
               return **pool;
            }

            const Payload& Holder::null_payload() const
            {
               static const Payload singleton{ nullptr};
               return singleton;
            }


            platform::buffer::raw::type Holder::allocate( const std::string& type, platform::binary::size::type size)
            {
               auto buffer = find( type).allocate( type, size);

               if( log::category::buffer)
               {
                  log::category::buffer << "allocate type: " << type << " size: " << size << " @" << static_cast< const void*>( buffer) << '\n';
               }
               return buffer;
            }

            platform::buffer::raw::type Holder::reallocate( platform::buffer::raw::immutable::type handle, platform::binary::size::type size)
            {
               auto buffer = find( handle).reallocate( handle, size);

               if( log::category::buffer)
               {
                  log::category::buffer << "reallocate size: " << size
                        << " from @" << static_cast< const void*>( handle) << " to " << static_cast< const void*>( buffer) << '\n';
               }

               if( handle == m_inbound)
               {
                  m_inbound = buffer;
               }

               return buffer;
            }

            const std::string& Holder::type( platform::buffer::raw::immutable::type handle)
            {
               return get( handle).payload().type;
            }

            void Holder::deallocate( platform::buffer::raw::immutable::type handle)
            {
               Trace trace{ "buffer::pool::deallocate"};

               log::category::buffer << "deallocate @" << static_cast< const void*>( handle) << '\n';

               //
               // according to the XATMI-spec it's a no-op for tpfree for the inbound-buffer...
               //
               if( handle != m_inbound && handle != nullptr)
               {
                  find( handle).deallocate( handle);
               }
            }

            platform::buffer::raw::type Holder::adopt( Payload&& payload)
            {
               Trace trace{ "buffer::pool::adopt"};

               //
               // This is the only place where a buffer is consumed by the pool, hence can only happen
               // during service-invocation.
               //
               // Keep track of the inbound buffer given to the user. This is a
               // 'special' buffer according to the XATMI-spec.
               //
               m_inbound = std::get< 0>( insert( std::move( payload)));

               return m_inbound;
            }

            std::tuple< platform::buffer::raw::type, platform::buffer::raw::size::type> Holder::insert( Payload&& payload)
            {
               Trace trace{ "buffer::pool::insert"};

               using return_type = std::tuple< platform::buffer::raw::type, platform::buffer::raw::size::type>;

               log::category::buffer << "insert type: " << payload.type << " size: " << payload.memory.size()
                     << " @" << static_cast< const void*>( payload.memory.data()) << '\n';

               if( payload.null())
               {
                  // This should work - doesn't on g++ 5.4
                  //return { nullptr, 0};
                  return return_type{ nullptr, 0}; 
               }
               auto size = payload.memory.size();

               // This should work - doesn't on g++ 5.4
               //return { find( payload.type).insert( std::move( payload)), size};
               return return_type{ find( payload.type).insert( std::move( payload)), size};
            }

            payload::Send Holder::get( platform::buffer::raw::immutable::type handle, platform::binary::size::type user_size)
            {
               if( handle == nullptr)
               {
                  return null_payload();
               }
               return find( handle).get( handle, user_size);
            }

            payload::Send Holder::get( platform::buffer::raw::immutable::type handle)
            {
               if( handle == nullptr)
               {
                  return null_payload();
               }

               return find( handle).get( handle);
            }


            Payload Holder::release( platform::buffer::raw::immutable::type handle)
            {
               if( handle == nullptr)
               {
                  return null_payload();
               }

               auto result = find( handle).release( handle);

               if( m_inbound == handle) m_inbound = nullptr;

               log::category::buffer << "release type: " << result.type << " size: " << result.memory.size() << " @" << static_cast< const void*>( result.memory.data()) << '\n';

               return result;
            }

            Payload Holder::release( platform::buffer::raw::immutable::type handle, platform::binary::size::type size)
            {
               if( handle == nullptr)
               {
                  return null_payload();
               }

               auto result = find( handle).release( handle, size);

               if( m_inbound == handle) m_inbound = nullptr;

               log::category::buffer << "release type: " << result.type << " size: " << result.memory.size() << " @" << static_cast< const void*>( result.memory.data()) << '\n';

               return result;
            }


            void Holder::clear()
            {
               if( m_inbound)
               {
                  try
                  {
                     find( m_inbound).deallocate( m_inbound);
                     m_inbound = nullptr;
                  }
                  catch( const exception::base& exception)
                  {
                     log::category::error << "failed to deallocate inbound buffer - " << exception << '\n';
                  }
               }
               algorithm::for_each( m_pools, std::mem_fn( &Base::clear));
            }



         } // pool
      } // buffer
   } // common

} // casual
