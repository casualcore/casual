//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/buffer/type.h"
#include "casual/platform.h"
#include "common/algorithm.h"
#include "common/string.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/code/xatmi.h"

#include "common/log.h"


#include <memory>
#include <map>

namespace casual
{
   namespace common
   {
      namespace buffer
      {
         namespace pool
         {

            struct Holder
            {
            private:

               Holder() = default;


               struct concept
               {
                  virtual ~concept() = default;

                  virtual platform::buffer::raw::type allocate( const std::string& type, platform::binary::size::type size) = 0;
                  virtual platform::buffer::raw::type reallocate( platform::buffer::raw::immutable::type handle, platform::binary::size::type size) = 0;


                  virtual bool manage( platform::buffer::raw::immutable::type handle) = 0;

                  virtual bool manage( const std::string& type) = 0;

                  virtual void deallocate( platform::buffer::raw::immutable::type handle) = 0;


                  virtual platform::buffer::raw::type insert( Payload payload) = 0;

                  virtual payload::Send get( platform::buffer::raw::immutable::type handle) = 0;
                  virtual payload::Send get( platform::buffer::raw::immutable::type handle, platform::binary::size::type user_size) = 0;

                  virtual Payload release( platform::buffer::raw::immutable::type handle, platform::binary::size::type user_size) = 0;
                  virtual Payload release( platform::buffer::raw::immutable::type handle) = 0;

                  virtual void clear() = 0;
               };

               template< typename P>
               struct model : concept
               {
                  using pool_type = P;

                  model( pool_type pool) : m_pool( std::move( pool)) {}
                  ~model() = default;

                  model( const model&) = delete;
                  model& operator = ( const model&) = delete;

                  pool_type& pool()
                  {
                     return m_pool;
                  }

               private:

                  platform::buffer::raw::type allocate( const std::string& type, platform::binary::size::type size) override
                  {
                     return m_pool.allocate( type, size);
                  }
                  platform::buffer::raw::type reallocate( platform::buffer::raw::immutable::type handle, platform::binary::size::type size) override
                  {
                     return m_pool.reallocate( handle, size);
                  }


                  bool manage( platform::buffer::raw::immutable::type handle) override
                  {
                     return m_pool.manage( handle);
                  }

                  bool manage( const std::string& type) override
                  {
                     for( auto& check : pool_type::types())
                     {
                        if( check == type)
                           return true;
                     }
                     return false;
                  }

                  void deallocate( platform::buffer::raw::immutable::type handle) override
                  {
                     m_pool.deallocate( handle);
                  }


                  platform::buffer::raw::type insert( Payload payload) override
                  {
                     return m_pool.insert( std::move( payload));
                  }

                  payload::Send get( platform::buffer::raw::immutable::type handle) override
                  {
                     auto& buffer = m_pool.get( handle);

                     return { buffer.payload, buffer.transport( buffer.reserved()), buffer.reserved()};
                  }

                  payload::Send get( platform::buffer::raw::immutable::type handle, platform::binary::size::type user_size) override
                  {
                     log::line( log::category::buffer, "handle: @", static_cast< const void*>( handle), ", user_size: ", user_size);

                     auto& buffer = m_pool.get( handle);

                     payload::Send result{ buffer.payload, buffer.transport( user_size), buffer.reserved()};

                     log::line( log::category::buffer, "pool::get - buffer: ", result);

                     return result;
                  }

                  Payload release( platform::buffer::raw::immutable::type handle) override
                  {
                     return m_pool.release( handle).payload;
                  }

                  Payload release( platform::buffer::raw::immutable::type handle, platform::binary::size::type user_size) override
                  {
                     log::line( log::category::buffer, "handle: @", static_cast< const void*>( handle), ", user_size: ", user_size);
                     
                     auto buffer = m_pool.release( handle);

                     log::line( log::category::buffer, "pool::release - payload: ", buffer.payload, " - transport: ", buffer.transport( user_size));

                     // Adjust the buffer size, with regards to the user size
                     buffer.payload.memory.erase( std::begin( buffer.payload.memory) + buffer.transport( user_size), std::end( buffer.payload.memory));
                     return std::move( buffer.payload);
                  }

                  void clear() override
                  {
                     m_pool.clear();
                  }

                  pool_type m_pool;
               };


               platform::buffer::raw::type m_inbound = nullptr;
               std::vector< std::unique_ptr< concept>> m_pools;

               template< typename P>
               friend class Registration;

               template< typename P>
               P& registration( P&& pool)
               {
                  for( auto& type : P::types())
                  {
                     if( Holder::find_concept( type))
                        code::raise::generic( code::casual::buffer_type_duplicate, log::stream::get( "error"), "buffer type already registered: ", type);
                  }

                  auto subtype = std::make_unique< model< P>>( std::forward< P>( pool));

                  auto& result = subtype->pool();

                  m_pools.emplace_back( std::move( subtype));

                  return result;
               }


               concept* find_concept( const std::string& type);
               concept& get_concept( const std::string& type);
               concept& get_concept( platform::buffer::raw::immutable::type handle);

               const Payload& null_payload() const;

            public:

               static Holder& instance()
               {
                  static Holder singleton;
                  return singleton;
               }


               inline platform::buffer::raw::type allocate( const char* type, const char* subtype, platform::binary::size::type size)
               {
                  return allocate( type::combine( type, subtype), size);
               }
               platform::buffer::raw::type allocate( const std::string& type, platform::binary::size::type size);

               platform::buffer::raw::type reallocate( platform::buffer::raw::immutable::type handle, platform::binary::size::type size);

               const std::string& type( platform::buffer::raw::immutable::type handle);

               void deallocate( platform::buffer::raw::immutable::type handle);

               //! Adopts the payload in 'service-invoke' context. So we keep track of
               //! inbound buffer (which is 'special' in XATMI). Otherwise it's the same semantics
               //! as insert
               platform::buffer::raw::type adopt( Payload&& payload);

               std::tuple< platform::buffer::raw::type, platform::buffer::raw::size::type> insert( Payload&& payload);

               payload::Send get( platform::buffer::raw::immutable::type handle);

               payload::Send get( platform::buffer::raw::immutable::type handle, platform::binary::size::type user_size);

               Payload release( platform::buffer::raw::immutable::type handle, platform::binary::size::type user_size);
               Payload release( platform::buffer::raw::immutable::type handle);

               void clear();

            };


            inline Holder& holder() { return Holder::instance();}

            template< typename B>
            struct basic_pool
            {
               using buffer_type = B;
               using pool_type = std::vector< buffer_type>;
               using range_type = range::type_t< pool_type>;


               using types_type = std::vector< std::string>;

               platform::buffer::raw::type allocate( const std::string& type, platform::binary::size::type size)
               {
                  auto& payload = m_pool.emplace_back( type, size).payload;

                  // Make sure we've got a handle
                  if( ! payload.memory.data())
                     payload.memory.reserve( 1);

                  return payload.memory.data();
               }

               platform::buffer::raw::type reallocate( platform::buffer::raw::immutable::type handle, platform::binary::size::type size)
               {
                  auto found = find( handle);

                  if( found)
                  {
                     auto& payload = found->payload;

                     payload.memory.resize( size);

                     // Make sure we've got a handle
                     if( ! payload.memory.data())
                        payload.memory.reserve( 1);

                     return payload.memory.data();
                  }
                  return nullptr;
               }


               bool manage( platform::buffer::raw::immutable::type handle)
               {
                  return ! find( handle).empty();
               }

               void deallocate( platform::buffer::raw::immutable::type handle)
               {
                  auto found = find( handle);

                  if( found)
                     m_pool.erase( std::begin( found));
               }


               platform::buffer::raw::type insert( Payload payload)
               {
                  m_pool.emplace_back( std::move( payload));

                  if( ! m_pool.back().payload.memory.data())
                     m_pool.back().payload.memory.reserve( 1);

                  return m_pool.back().payload.memory.data();
               }

               buffer_type& get( platform::buffer::raw::immutable::type handle)
               {
                  if( auto found = find( handle))
                     return *found;

                  code::raise::generic( code::xatmi::argument, log::category::buffer, "failed to find buffer");
               }


               buffer_type release( platform::buffer::raw::immutable::type handle)
               {
                  if( auto found = find( handle))
                  {
                     buffer_type result{ std::move( *found)};
                     m_pool.erase( std::begin( found));

                     return result;
                  }

                  code::raise::generic( code::xatmi::argument, log::category::buffer, "failed to find buffer");
               }


               void clear()
               {
                  // We don't do any automatic cleanup, since the user could have a
                  // global handle to some buffer.
                  //
                  // To look at the previous implementation see commit: 2655a5b61813e628153535ec05082c3582eb86f1
               }

            protected:

               range_type find( platform::buffer::raw::immutable::type handle)
               {
                  return algorithm::find_if( m_pool,
                        [&]( const buffer_type& b){ return b.payload.memory.data() == handle;});
               }
               pool_type m_pool;
            };

            using default_pool = basic_pool< buffer::Buffer>;


            template< typename P>
            class Registration
            {
            public:
               static P& pool;
            };


            template< typename P>
            CASUAL_MAYBE_UNUSED P& Registration< P>::pool = Holder::instance().registration( P{});

         } // pool
      } // buffer
   } // common



} // casual


