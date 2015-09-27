//!
//! execution.h
//!
//! Created on: Jun 7, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_EXECUTION_H_
#define CASUAL_COMMON_EXECUTION_H_

#include <string>

namespace casual
{
   namespace common
   {
      struct Uuid;

      namespace execution
      {
         //!
         //! Sets current execution id
         //!
         void id( const Uuid& id);

         //!
         //! Gets current execution id
         //!
         const Uuid& id();


         //!
         //! Sets the current service
         //!
         void service( const std::string& service);

         //!
         //! Gets the current service (if any)
         //!
         const std::string& service();

         namespace parent
         {
            //!
            //! Sets the current parent service
            //!
            void service( const std::string& service);

            //!
            //! Gets the current parent service (if any)
            //!
            const std::string& service();

         } // parent



      } // execution
   } // common


} // casual

#endif // EXECUTION_H_
