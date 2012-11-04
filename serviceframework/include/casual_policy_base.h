//!
//! casual_policy_base.h
//!
//! Created on: Oct 31, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_POLICY_BASE_H_
#define CASUAL_POLICY_BASE_H_

namespace casual
{
   namespace sf
   {
      namespace policy
      {
         //!
         //! Default implementation for the "handle-events", so policies doesn't have to
         //! implement these.
         //!
         struct Base
         {

            inline void handle_start( const char* name) { /* no op */}

            inline void handle_end( const char* name) { /* no op */}

            //inline std::size_t handle_container_start( std::size_t size) { /* no op */}

            inline void handle_container_end() { /* no op */}

            inline void handle_serialtype_start() { /* no op */}

            inline void handle_serialtype_end() { /* no op */}


         };

      }
   }
}


#endif /* CASUAL_POLICY_BASE_H_ */
