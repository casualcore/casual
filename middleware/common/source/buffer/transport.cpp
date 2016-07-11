//!
//! casual
//!

#include "common/buffer/transport.h"
#include "common/buffer/pool.h"


#include "common/algorithm.h"


namespace casual
{
   namespace common
   {
      namespace buffer
      {
         namespace transport
         {


            bool operator < ( const Context::Callback& lhs, const Context::Callback& rhs)
            {
               return lhs.order < rhs.order;
            }


            Context& Context::instance()
            {
               static Context singleton;
               return singleton;
            }

            Context::Context()
            {

            }

            void Context::registration( std::size_t order, std::vector< Lifecycle> lifecycles, std::vector< buffer::Type> types, dispatch_type callback)
            {
               m_callbacks.emplace_back( order, std::move( lifecycles), std::move( types), std::move( callback));

               range::stable_sort( m_callbacks);
            }

            void Context::dispatch(
                  platform::raw_buffer_type& buffer,
                  platform::raw_buffer_size& size,
                  const std::string& service,
                  Lifecycle lifecycle,
                  const buffer::Type& type)
            {
               auto predicate = [&]( const Callback& c){
                  return (c.lifecycles.empty() || range::find( c.lifecycles, lifecycle))
                        && (c.types.empty() || range::find( c.types, type));
               };

               auto found = range::find_if( m_callbacks, predicate);

               while( found)
               {
                  found->dispatch( buffer, size, service, lifecycle, type);
                  found++;

                  found = range::find_if( found, predicate);
               }
            }

            void Context::dispatch(
                  platform::raw_buffer_type& buffer,
                  platform::raw_buffer_size& size,
                  const std::string& service,
                  Lifecycle lifecycle)
            {
               dispatch( buffer, size, service, lifecycle, buffer::pool::Holder::instance().type( buffer));
            }

            Context::Callback::Callback( std::size_t order, std::vector< Lifecycle> lifecycles, std::vector< buffer::Type> types, dispatch_type callback)
             : order( order), lifecycles( std::move( lifecycles)), types( std::move( types)), dispatch( std::move( callback)) {}



         } // transport
      } // buffer
   } // common
} // casual
