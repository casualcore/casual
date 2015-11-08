//!
//! pre.h
//!
//! Created on: Dec 26, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_BUFFER_PRE_H_
#define CASUAL_COMMON_BUFFER_PRE_H_


#include "common/buffer/type.h"

#include "common/platform.h"


//
// std
//
#include <functional>


namespace casual
{
   namespace common
   {
      namespace buffer
      {
         namespace transport
         {
            struct Context;

            enum class Lifecycle
            {
               pre_call,
               post_call,
               pre_service,
               post_service
            };


            struct Context
            {
               using dispatch_type = std::function< void(platform::raw_buffer_type&, platform::raw_buffer_size&, const std::string&, Lifecycle, const buffer::Type&)>;

               static Context& instance();

               void registration( std::size_t order, std::vector< Lifecycle> lifecycles, std::vector< buffer::Type> types, dispatch_type callback);

               void dispatch(
                     platform::raw_buffer_type& buffer,
                     platform::raw_buffer_size& size,
                     const std::string& service,
                     Lifecycle lifecycle,
                     const buffer::Type& type);

               void dispatch(
                     platform::raw_buffer_type& buffer,
                     platform::raw_buffer_size& size,
                     const std::string& service,
                     Lifecycle lifecycle);

            private:

               Context();

               struct Callback
               {
                  Callback( std::size_t order, std::vector< Lifecycle> lifecycles, std::vector< buffer::Type> types, dispatch_type callback);

                  std::size_t order = 0;
                  std::vector< Lifecycle> lifecycles;
                  std::vector< buffer::Type> types;
                  dispatch_type dispatch;
               };

               std::vector< Callback> m_callbacks;

               friend bool operator < ( const Callback& lhs, const Callback& rhs);
            };



            template< typename C>
            struct basic_registration
            {

            };


         } // transport
      } // buffer
   } // common
} // casual

#endif // PRE_H_
