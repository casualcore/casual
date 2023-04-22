//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/buffer/type.h"
#include "casual/platform.h"
#include "common/algorithm.h"
#include "common/algorithm/container.h"
#include "common/string.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/code/xatmi.h"

#include "common/log.h"

#include <memory>
#include <map>

namespace casual
{
   namespace common::buffer::pool
   {
      //! if needed somewhere else, move it to common/traits.h (if it's possible)
      namespace traits::is
      {
         namespace detail
         {
            template< typename T> 
            using loggable = decltype( common::stream::write( std::declval< std::ostream&>(), std::declval< const T&>()));

            template< typename T> 
            using adoptable = decltype( std::declval< T&>().adopt( Payload{}, std::add_pointer_t< buffer::handle::type>{}));
         } // detail

         template< typename T>
         constexpr bool loggable_v = common::traits::detect::is_detected_v< detail::loggable, T>;

         template< typename T>
         constexpr bool adoptable_v = common::traits::detect::is_detected_v< detail::adoptable, T>;
         
      } // traits::is


      struct Holder
      {
         static Holder& instance();

         inline auto allocate( const char* type, const char* subtype, platform::binary::size::type size)
         {
            return allocate( type::combine( type, subtype), size);
         }
         buffer::handle::mutate::type allocate( std::string_view type, platform::binary::size::type size);
         buffer::handle::mutate::type reallocate( buffer::handle::type handle, platform::binary::size::type size);

         const std::string& type( buffer::handle::type handle);

         void deallocate( buffer::handle::type handle);

         //! Adopts the payload in 'service-invoke' context. So we keep track of
         //! inbound buffer (which is 'special' in XATMI). Otherwise it's the same semantics
         //! as insert
         buffer::handle::mutate::type adopt( Payload&& payload);

         std::tuple< buffer::handle::mutate::type, platform::buffer::raw::size::type> insert( Payload&& payload);

         payload::Send get( buffer::handle::type handle);
         payload::Send get( buffer::handle::type handle, platform::binary::size::type user_size);

         Payload release( buffer::handle::type handle, platform::binary::size::type user_size);
         Payload release( buffer::handle::type handle);

         void clear();

         buffer::handle::type inbound() const noexcept { return m_inbound;}

         CASUAL_FORWARD_SERIALIZE( m_pools);


      private:

         Holder() = default;

         struct concept
         {
            virtual ~concept() = default;

            virtual buffer::handle::mutate::type allocate( std::string_view type, platform::binary::size::type size) = 0;
            virtual buffer::handle::mutate::type reallocate( buffer::handle::type handle, platform::binary::size::type size) = 0;



            virtual bool manage( buffer::handle::type handle) const noexcept = 0;
            virtual bool manage( std::string_view type) const noexcept = 0;

            virtual void deallocate( buffer::handle::type handle) = 0;
            virtual buffer::handle::mutate::type insert( Payload payload) = 0;
            virtual buffer::handle::mutate::type adopt( Payload payload, buffer::handle::type* inbound) = 0;

            virtual payload::Send get( buffer::handle::type handle) = 0;
            virtual payload::Send get( buffer::handle::type handle, platform::binary::size::type user_size) = 0;

            virtual Payload release( buffer::handle::type handle, platform::binary::size::type user_size) = 0;
            virtual Payload release( buffer::handle::type handle) = 0;

            virtual void print( std::ostream& out) = 0;

            virtual void clear() = 0;
         };

         template< typename P>
         struct model : concept
         {
            using pool_type = P;

            template< typename... Args>
            model( Args&&... args) : m_pool( std::forward< Args>( args)...) {}

            ~model() = default;

            model( const model&) = delete;
            model& operator = ( const model&) = delete;

            pool_type* implementation() noexcept { return &m_pool;}

         private:
            
            buffer::handle::mutate::type allocate( std::string_view type, platform::binary::size::type size) override { return m_pool.allocate( type, size);}
            buffer::handle::mutate::type reallocate( buffer::handle::type handle, platform::binary::size::type size) override { return m_pool.reallocate( handle, size);}
            bool manage( buffer::handle::type handle) const noexcept override { return m_pool.manage( handle);}
            bool manage( std::string_view type) const noexcept override { return predicate::boolean( algorithm::find( pool_type::types(), type));}
            void deallocate( buffer::handle::type handle) override { m_pool.deallocate( handle);}
            buffer::handle::mutate::type insert( Payload payload) override { return m_pool.insert( std::move( payload));}
            
            buffer::handle::mutate::type adopt( Payload payload, [[maybe_unused]] buffer::handle::type* inbound) override 
            {
               if constexpr( traits::is::adoptable_v< pool_type>)
                  return m_pool.adopt( std::move( payload), inbound);
               else
                  return m_pool.insert( std::move( payload));
            }

            payload::Send get( buffer::handle::type handle) override
            {
               auto& buffer = m_pool.get( handle);
               return payload::Send{ buffer.payload, buffer.transport( buffer.reserved()), buffer.reserved()};
            }

            payload::Send get( buffer::handle::type handle, platform::binary::size::type user_size) override
            {
               auto& buffer = m_pool.get( handle);
               return payload::Send{ buffer.payload, buffer.transport( user_size), buffer.reserved()};
            }

            Payload release( buffer::handle::type handle) override { return m_pool.release( handle).payload;}

            Payload release( buffer::handle::type handle, platform::binary::size::type user_size) override
            {
               log::line( log::category::buffer, "handle: ", handle, ", user_size: ", user_size);
               
               auto buffer = m_pool.release( handle);

               log::line( log::category::buffer, "pool::release - payload: ", buffer.payload, " - transport: ", buffer.transport( user_size));

               // Adjust the buffer size, with regards to the user size
               buffer.payload.data.erase( std::begin( buffer.payload.data) + buffer.transport( user_size), std::end( buffer.payload.data));
               return std::move( buffer.payload);
            }

            void print( std::ostream& out) override
            {
               stream::write( out, "{ types: ", pool_type::types());

               if constexpr( traits::is::loggable_v< pool_type>)
                  stream::write( out, ", pool: ", m_pool, '}');
               else
                  stream::write( out, '}');
            }

            void clear() override { m_pool.clear();}

            pool_type m_pool;
         };

         constexpr static auto manage_key( std::string_view key) 
         {
            return [ key]( const auto& concept){ return concept->manage( key);};
         };

         constexpr static auto manage_buffer( buffer::handle::type handle) 
         {
            return [ handle]( const auto& concept){ return concept->manage( handle);};
         };

         template< typename P>
         friend struct Registration;

         template< typename P, typename... Args>
         P* registration( Args&&... args)
         {
            for( auto&& type : P::types())
               if( algorithm::find_if( m_pools, manage_key( type)))
                  code::raise::error( code::casual::buffer_type_duplicate, "buffer type already registered: ", type);

            auto pool = std::make_unique< model< P>>( std::forward< Args>( args)...);
            auto address = pool->implementation();
            m_pools.push_back( std::move( pool));
            return address;
         }
         
         concept& get_pool( std::string_view type);
         concept& get_pool( buffer::handle::type handle);
         concept* find_pool( std::string_view type);

         const Payload& null_payload() const;

         buffer::handle::type m_inbound;
         std::vector< std::unique_ptr< concept>> m_pools;

      };

      inline Holder& holder() { return Holder::instance();}


      template< typename Pool>
      struct Registration
      {         
         static Pool& pool() noexcept { return *m_pool_instance;}
      private:
         inline static Pool* m_pool_instance = Holder::instance().registration< Pool>();
      };

      namespace implementation
      {
         template< typename B>
         struct Default 
         {
         private:

            constexpr static auto is_buffer( buffer::handle::type handle) 
            {
               return [ handle]( const auto& buffer){ return buffer.handle() == handle;};
            };

         public:
            using buffer_type = B;
            using buffers_type = std::vector< buffer_type>;
            using range_type = range::type_t< buffers_type>;

            Default() = default;
            Default( Default&&) noexcept = default;
            Default& operator = ( Default&&) noexcept = default;
            
            buffer::handle::mutate::type allocate( string::Argument type, platform::binary::size::type size)
            {
               auto& payload = m_buffers.emplace_back( std::move( type), size).payload;

               // Make sure we've got a handle
               if( ! payload.data.data())
                  payload.data.reserve( 1);

               return payload.handle();
            }

            buffer::handle::mutate::type reallocate( buffer::handle::type handle, platform::binary::size::type size)
            {
               if( auto found = algorithm::find_if( m_buffers, is_buffer( handle)))
               {
                  auto& payload = found->payload;

                  payload.data.resize( size);

                  // Make sure we've got a handle
                  if( ! payload.data.data())
                     payload.data.reserve( 1);

                  return payload.handle();
               }
               return {};
            }

            bool manage( buffer::handle::type handle) const noexcept 
            {
               return predicate::boolean( algorithm::find_if( m_buffers, is_buffer( handle)));
            }

            void deallocate( buffer::handle::type handle)
            {
               if( auto found = algorithm::find_if( m_buffers, is_buffer( handle)))
                  m_buffers.erase( std::begin( found));
            }

            buffer::handle::mutate::type insert( Payload payload)
            {
               auto& buffer = m_buffers.emplace_back( std::move( payload));

               if( ! buffer.payload.data.data())
                  buffer.payload.data.reserve( 1);

               return buffer.payload.handle();
            }

            buffer_type& get( buffer::handle::type handle)
            {
               if( auto found = algorithm::find_if( m_buffers, is_buffer( handle)))
                  return *found;

               code::raise::error( code::xatmi::argument, "failed to find buffer");
            }


            buffer_type release( buffer::handle::type handle)
            {
               if( auto found = algorithm::find_if( m_buffers, is_buffer( handle)))
                  return algorithm::container::extract( m_buffers, std::begin( found));

               code::raise::error( code::xatmi::argument, "failed to find buffer");
            }

            void clear()
            {
               // We don't do any automatic cleanup, since the user could have a
               // global handle to some buffer.
            }

            template< typename... Args>
            decltype( auto) emplace_back( Args&&... args)
            {
               return m_buffers.emplace_back( std::forward< Args>( args)...);
            }

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE_NAME( m_buffers, "buffers");
            )

         private:
            buffers_type m_buffers;
         };

         using default_buffer = implementation::Default< buffer::Buffer>;

      } // implementation



   } // common::buffer::pool
} // casual


