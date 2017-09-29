
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

   } // http
} // casual


