//!
//! casual
//!

#ifndef CASUAL_COMMON_BUFFER_TRANSPORT_H_
#define CASUAL_COMMON_BUFFER_TRANSPORT_H_


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
            using size_type = platform::size::type;

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
               using dispatch_type = std::function< void( platform::buffer::raw::type&, platform::buffer::raw::size::type&, const std::string&, Lifecycle, const std::string&)>;

               static Context& instance();

               void registration( size_type order, std::vector< Lifecycle> lifecycles, std::vector< std::string> types, dispatch_type callback);

               void dispatch(
                     platform::buffer::raw::type& buffer,
                     platform::buffer::raw::size::type& size,
                     const std::string& service,
                     Lifecycle lifecycle,
                     const std::string& type);

               void dispatch(
                     platform::buffer::raw::type& buffer,
                     platform::buffer::raw::size::type& size,
                     const std::string& service,
                     Lifecycle lifecycle);

            private:

               Context();

               struct Callback
               {
                  Callback( size_type order, std::vector< Lifecycle> lifecycles, std::vector< std::string> types, dispatch_type callback);

                  size_type order = 0;
                  std::vector< Lifecycle> lifecycles;
                  std::vector< std::string> types;
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
