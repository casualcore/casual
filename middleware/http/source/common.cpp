
#include "http/common.h"

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

      } // protocol
   } // http
} // casual


