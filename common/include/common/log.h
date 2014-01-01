//!
//! casual_logger.h
//!
//! Created on: Jun 21, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_LOGGER_H_
#define CASUAL_LOGGER_H_

#include <string>


#include <ostream>

#include "common/platform.h"

namespace casual
{
   namespace common
   {
      namespace log
      {

         namespace category
         {
            enum class Type
            {
               //
               // casual internal logging
               casual_debug,
               casual_trace,
               casual_transaction,

               //
               // Public logging
               debug = 10,
               trace,
               parameter,
               information,
               warning,
               error,
            };

            const char* name( Type type);

         } // category


         //!
         //! Log with category 'debug'
         //!
         extern std::ostream debug;

         //!
         //! Log with category 'trace'
         //!
         extern std::ostream trace;


         //!
         //! Log with category 'parameter'
         //!
         extern std::ostream parameter;

         //!
         //! Log with category 'information'
         //!
         extern std::ostream information;

         //!
         //! Log with category 'warning'
         //!
         //! @note should be used very sparsely. Either it is an error or it's not...
         //!
         extern std::ostream warning;

         //!
         //! Log with category 'error'
         //!
         //! @note always active
         //!
         extern std::ostream error;


         //!
         //! @return true if the log-category is active.
         //!
         bool active( category::Type category);

         void activate( category::Type category);

         void deactivate( category::Type category);



         void write( category::Type category, const char* message);

         void write( category::Type category, const std::string& message);

      } // log


   } // common

} // casual

#endif /* CASUAL_LOGGER_H_ */
