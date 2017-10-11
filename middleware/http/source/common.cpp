
#include "http/common.h"

#include "common/algorithm.h"
#include "common/buffer/type.h"

namespace casual
{
   namespace http
   {
      common::log::Stream log{ "casual.http"};

      namespace verbose
      {
         common::log::Stream log{ "casual.http.verbose"};
      } // verbose

      namespace trace
      {
         common::log::Stream log{ "casual.http.trace"};
      } // trace

      namespace protocol
      {
         const std::string& x_octet() { static const auto name = std::string("application/casual-binary"); return name;}
         const std::string& binary() { static const auto name = std::string("application/casual-x-octet"); return name;}
         const std::string& json() { static const auto name = std::string("application/json"); return name;}
         const std::string& xml() { static const auto name = std::string("application/xml"); return name;}

         namespace convert
         {
            namespace local
            {
               namespace
               {
                  template< typename C, typename K>
                  auto find( C& container, const K& key)
                  {
                     auto found = common::range::find( container, key);

                     if( found)
                        return found->second;

                     return decltype( found->second){};
                  }
               } // <unnamed>
            } // local
            namespace from
            {
               std::string buffer( const std::string& buffer)
               {
                  static const std::map< std::string, std::string> mapping{
                     { common::buffer::type::x_octet(), protocol::x_octet()},
                     { common::buffer::type::binary(), protocol::binary()},
                     { common::buffer::type::json(), protocol::json()},
                     { common::buffer::type::xml(), protocol::xml()},
                  };

                  return local::find( mapping, buffer);
               }
            } // from

            namespace to
            {
               std::string buffer( const std::string& content)
               {
                  static const std::map< std::string, std::string> mapping{
                     { protocol::x_octet(), common::buffer::type::x_octet()},
                     { protocol::binary(), common::buffer::type::binary()},
                     { protocol::json(), common::buffer::type::json()},
                     { protocol::xml(), common::buffer::type::xml()},
                  };

                  return local::find( mapping, content);
               }
            } // to
         } // convert


      } // protocol
   } // http
} // casual


