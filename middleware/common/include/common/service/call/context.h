//!
//! casual
//!

#ifndef CASUAL_CALLING_CONTEXT_H_
#define CASUAL_CALLING_CONTEXT_H_

#include "common/service/call/state.h"
#include "common/service/call/flags.h"

#include "common/message/service.h"




#include <vector>


namespace casual
{
   namespace common
   {

      namespace server
      {
         class Context;
      }

      namespace service
      {
         namespace call
         {
            namespace reply
            {
               enum class State : int
               {
                  service_success = 0,
                  service_fail = TPESVCFAIL
               };

               struct Result
               {
                  common::buffer::Payload buffer;
                  long user = 0;
                  descriptor_type descriptor;
                  State state;
               };
            } // reply

            namespace sync
            {
               using State = reply::State;
               struct Result
               {
                  common::buffer::Payload buffer;
                  long user = 0;
                  State state;
               };
            } // sync

            class Context
            {
            public:
               static Context& instance();

               descriptor_type async( const std::string& service, common::buffer::payload::Send buffer, async::Flags flags);

               reply::Result reply( descriptor_type descriptor, reply::Flags flags);

               sync::Result sync( const std::string& service, common::buffer::payload::Send buffer, sync::Flags flags);

               void cancel( descriptor_type descriptor);

               void clean();

               //!
               //! @returns true if there are pending replies or associated transactions.
               //!  Hence, it's ok to do a service-forward if false is return
               //!
               bool pending() const;

            private:
               Context();
               bool receive( message::service::call::Reply& reply, descriptor_type descriptor, reply::Flags);

               State m_state;

            };
         } // call
      } // service
	} // common
} // casual


#endif /* CASUAL_CALLING_CONTEXT_H_ */
