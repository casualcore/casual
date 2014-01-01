//!
//! log.h
//!
//! Created on: Dec 14, 2013
//!     Author: Lazan
//!

#ifndef LOG_H_
#define LOG_H_

#include "common/log.h"

#include "sf/namevaluepair.h"
#include "sf/archive_log.h"


#include <sstream>



namespace casual
{

   namespace sf
   {

      namespace log
      {
         using common::log::debug;
         using common::log::trace;
         using common::log::parameter;
         using common::log::information;
         using common::log::warning;
         using common::log::error;
      } // log


      template <typename T, typename R>
      std::ostream& operator << ( std::ostream& out, NameValuePair< T, R>&& value)
      {
         if( out.good())
         {
            sf::archive::log::Writer writer( out);
            writer << value;
         }
         return out;
      }

   } // sf


} // casual

#endif // LOG_H_
