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

// TODO: temp
#include <sstream>

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

                  virtual platform::raw_buffer_type allocate( const Type& type, std::size_t size) = 0;
                  virtual platform::raw_buffer_type reallocate( platform::const_raw_buffer_type handle, std::size_t size) = 0;


                  virtual bool manage( platform::const_raw_buffer_type handle) = 0;

                  virtual bool manage( const Type& type) = 0;

                  virtual void deallocate( platform::const_raw_buffer_type handle) = 0;


                  virtual platform::raw_buffer_type insert( Payload payload) = 0;

                  virtual Payload& get( platform::const_raw_buffer_type handle) = 0;

                  virtual Payload extract( platform::const_raw_buffer_type handle) = 0;

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

                  platform::raw_buffer_type allocate( const Type& type, std::size_t size) override
                  {
                     return m_pool.allocate( type, size);
                  }
                  platform::raw_buffer_type reallocate( platform::const_raw_buffer_type handle, std::size_t size) override
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

                  Payload& get( platform::const_raw_buffer_type handle) override
                  {
                     return m_pool.get( handle).payload;
                  }

                  Payload extract( platform::const_raw_buffer_type handle) override
                  {
                     return m_pool.extract( handle);
                  }

                  void clear() override
                  {
                     m_pool.clear();
                  }

                  pool_type m_pool;
               };


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


            public:

               static Holder& instance()
               {
                  static Holder singleton;
                  return singleton;
               }

               platform::raw_buffer_type allocate( const Type& type, std::size_t size);

               platform::raw_buffer_type reallocate( platform::const_raw_buffer_type handle, std::size_t size);

               const Type& type( platform::const_raw_buffer_type handle);

               void deallocate( platform::const_raw_buffer_type handle);

               platform::raw_buffer_type insert( Payload&& payload);

               Payload& get( platform::const_raw_buffer_type handle);

               Payload extract( platform::const_raw_buffer_type handle);

               void clear();

            };


            template< typename B>
            struct basic_pool
            {
               using buffer_type = B;
               using pool_type = std::vector< buffer_type>;

               using types_type = std::vector< Type>;

               platform::raw_buffer_type allocate( const Type& type, std::size_t size)
               {
                  m_pool.emplace_back( type, size);
                  return m_pool.back().payload.memory.data();
               }

               platform::raw_buffer_type reallocate( platform::const_raw_buffer_type handle, std::size_t size)
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

               Payload extract( platform::const_raw_buffer_type handle)
               {
                  auto buffer = find( handle);

                  if( buffer != std::end( m_pool))
                  {
                     Payload result{ std::move( buffer->payload)};
                     m_pool.erase( buffer);
                     return result;
                  }
                  throw exception::xatmi::InvalidArguments{ "failed to find buffer"};
               }

               buffer_type& get( platform::const_raw_buffer_type handle)
               {
                  auto buffer = find( handle);

                  if( buffer != std::end( m_pool))
                  {
                     return *buffer;
                  }
                  throw exception::xatmi::InvalidArguments{ "failed to find buffer"};
               }

               void clear()
               {

                  if( ! m_pool.empty())
                  {
                     static bool logged = false;

                     if( ! logged)
                     {
                        logged = true;
                        log::warning << "buffer pool should be empty - casual takes care of missed deallocations - to be xatmi conformant use tpfree - will not be logged again\n";
                     }
                     log::internal::debug << "pool size: " << m_pool.size() << std::endl;
                  }
                  // pool_type empty;
                  // m_pool.swap( empty);
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

            template< typename P>
            P& Registration< P>::pool = Holder::instance().registration( P{});

         } // pool
      } // buffer
   } // common



} // casual

#endif // POOL_H_