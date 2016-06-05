//!
//! pool.h
//!
//! Created on: Sep 17, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_BUFFER_POOL_H_
#define CASUAL_COMMON_BUFFER_POOL_H_

#include "common/buffer/type.h"
#include "common/platform.h"
#include "common/algorithm.h"
#include "common/exception.h"

#include "common/log.h"
#include "common/internal/log.h"


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


               struct Base
               {
                  virtual ~Base() = default;

                  virtual platform::raw_buffer_type allocate( const Type& type, platform::binary_size_type size) = 0;
                  virtual platform::raw_buffer_type reallocate( platform::const_raw_buffer_type handle, platform::binary_size_type size) = 0;


                  virtual bool manage( platform::const_raw_buffer_type handle) = 0;

                  virtual bool manage( const Type& type) = 0;

                  virtual void deallocate( platform::const_raw_buffer_type handle) = 0;


                  virtual platform::raw_buffer_type insert( Payload payload) = 0;

                  virtual payload::Send get( platform::const_raw_buffer_type handle) = 0;
                  virtual payload::Send get( platform::const_raw_buffer_type handle, platform::binary_size_type user_size) = 0;

                  virtual Payload release( platform::const_raw_buffer_type handle, platform::binary_size_type user_size) = 0;
                  virtual Payload release( platform::const_raw_buffer_type handle) = 0;

                  virtual void clear() = 0;

               };

               template< typename P>
               struct Concrete : Base
               {
                  using pool_type = P;

                  Concrete( pool_type pool) : m_pool( std::move( pool)) {}
                  ~Concrete() = default;

                  Concrete( const Concrete&) = delete;
                  Concrete& operator = ( const Concrete&) = delete;

                  pool_type& pool()
                  {
                     return m_pool;
                  }

               private:

                  platform::raw_buffer_type allocate( const Type& type, platform::binary_size_type size) override
                  {
                     return m_pool.allocate( type, size);
                  }
                  platform::raw_buffer_type reallocate( platform::const_raw_buffer_type handle, platform::binary_size_type size) override
                  {
                     return m_pool.reallocate( handle, size);
                  }


                  bool manage( platform::const_raw_buffer_type handle) override
                  {
                     return m_pool.manage( handle);
                  }

                  bool manage( const Type& type) override
                  {
                     for( auto& check : pool_type::types())
                     {
                        if( check == type)
                        {
                           return true;
                        }
                     }
                     return false;
                  }

                  void deallocate( platform::const_raw_buffer_type handle) override
                  {
                     m_pool.deallocate( handle);
                  }


                  platform::raw_buffer_type insert( Payload payload) override
                  {
                     return m_pool.insert( std::move( payload));
                  }

                  payload::Send get( platform::const_raw_buffer_type handle) override
                  {
                     auto& buffer = m_pool.get( handle);

                     return { buffer.payload, buffer.transport( buffer.reserved()), buffer.reserved()};
                  }

                  payload::Send get( platform::const_raw_buffer_type handle, platform::binary_size_type user_size) override
                  {
                     auto& buffer = m_pool.get( handle);

                     payload::Send result{ buffer.payload, buffer.transport( user_size), buffer.reserved()};

                     log::internal::buffer << "pool::get - buffer: " << result << std::endl;

                     return result;
                  }

                  Payload release( platform::const_raw_buffer_type handle) override
                  {
                     return m_pool.release( handle).payload;
                  }

                  Payload release( platform::const_raw_buffer_type handle, platform::binary_size_type user_size) override
                  {
                     auto buffer = m_pool.release( handle);

                     log::internal::buffer << "pool::release - payload: " << buffer.payload << " - transport: " << buffer.transport( user_size) << std::endl;

                     //
                     // Adjust the buffer size, with regards to the user size
                     //
                     buffer.payload.memory.erase( std::begin( buffer.payload.memory) + buffer.transport( user_size), std::end( buffer.payload.memory));
                     return std::move( buffer.payload);
                  }

                  void clear() override
                  {
                     m_pool.clear();
                  }

                  pool_type m_pool;
               };


               platform::raw_buffer_type m_inbound = nullptr;
               std::vector< std::unique_ptr< Base>> m_pools;

               template< typename P>
               friend class Registration;

               template< typename P>
               P& registration( P&& pool)
               {

                  for( auto& type : P::types())
                  {
                     try
                     {
                        find( type);
                     }
                     catch( ...)
                     {
                        continue;
                     }

                     {
                        std::ostringstream message{ "buffer type already registered: "};
                        message << type;
                        throw exception::invalid::Argument{ message.str()};
                     }

                  }

                  auto subtype = std::unique_ptr< Concrete< P>>( new Concrete< P>( std::forward< P>( pool)));

                  auto& result = subtype->pool();

                  m_pools.emplace_back( std::move( subtype));

                  return result;
               }


               Base& find( const Type& type);
               Base& find( platform::const_raw_buffer_type handle);

               const Payload& null_payload() const;

            public:

               static Holder& instance()
               {
                  static Holder singleton;
                  return singleton;
               }


               platform::raw_buffer_type allocate( const Type& type, platform::binary_size_type size);

               platform::raw_buffer_type reallocate( platform::const_raw_buffer_type handle, platform::binary_size_type size);

               const Type& type( platform::const_raw_buffer_type handle);

               void deallocate( platform::const_raw_buffer_type handle);

               //!
               //! Adopts the payload in 'service-invoke' context. So we keep track of
               //! inbound buffer (which is 'special' in XATMI). Otherwise it's the same semantics
               //! as insert
               //!
               platform::raw_buffer_type adopt( Payload&& payload);

               platform::raw_buffer_type insert( Payload&& payload);

               payload::Send get( platform::const_raw_buffer_type handle);

               payload::Send get( platform::const_raw_buffer_type handle, platform::binary_size_type user_size);

               Payload release( platform::const_raw_buffer_type handle, platform::binary_size_type user_size);
               Payload release( platform::const_raw_buffer_type handle);

               void clear();

            };


            template< typename B>
            struct basic_pool
            {
               using buffer_type = B;
               using pool_type = std::vector< buffer_type>;

               using types_type = std::vector< Type>;

               platform::raw_buffer_type allocate( const Type& type, platform::binary_size_type size)
               {
                  m_pool.emplace_back( type, size);
                  return m_pool.back().payload.memory.data();
               }

               platform::raw_buffer_type reallocate( platform::const_raw_buffer_type handle, platform::binary_size_type size)
               {
                  auto buffer = find( handle);

                  if( buffer != std::end( m_pool))
                  {
                     buffer->payload.memory.resize( size);
                     return buffer->payload.memory.data();
                  }

                  return nullptr;

               }


               bool manage( platform::const_raw_buffer_type handle)
               {
                  return find( handle) != std::end( m_pool);
               }

               void deallocate( platform::const_raw_buffer_type handle)
               {
                  auto buffer = find( handle);

                  if( buffer != std::end( m_pool))
                  {
                     m_pool.erase( buffer);
                  }
               }


               platform::raw_buffer_type insert( Payload payload)
               {
                  m_pool.emplace_back( std::move( payload));
                  return m_pool.back().payload.memory.data();
               }

               buffer_type& get( platform::const_raw_buffer_type handle)
               {
                  auto buffer = find( handle);

                  if( buffer != std::end( m_pool))
                  {
                     return *buffer;
                  }
                  throw exception::xatmi::invalid::Argument{ "failed to find buffer"};
               }


               buffer_type release( platform::const_raw_buffer_type handle)
               {
                  auto buffer = find( handle);

                  if( buffer != std::end( m_pool))
                  {
                     buffer_type result{ std::move( *buffer)};
                     m_pool.erase( buffer);

                     return result;
                  }
                  throw exception::xatmi::invalid::Argument{ "failed to find buffer"};
               }


               void clear()
               {
                  //
                  // We don't do any automatic cleanup, since the user could have a
                  // global handle to some buffer.
                  //
                  // To look at the previous implementation see commit: 2655a5b61813e628153535ec05082c3582eb86f1
                  //
               }

            protected:

               typename pool_type::iterator find( platform::const_raw_buffer_type handle)
               {
                  return std::find_if( std::begin( m_pool), std::end( m_pool),
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


#ifdef __GNUC__
            template< typename P>
            __attribute__((__unused__)) P& Registration< P>::pool = Holder::instance().registration( P{});
#else
            template< typename P>
            //[[maybe_unused]] P& Registration< P>::pool = Holder::instance().registration( P{});
            P& Registration< P>::pool = Holder::instance().registration( P{});
#endif

         } // pool
      } // buffer
   } // common



} // casual

#endif // POOL_H_
