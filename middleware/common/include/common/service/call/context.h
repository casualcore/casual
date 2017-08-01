//!
//! casual
//!

#ifndef CASUAL_CALLING_CONTEXT_H_
#define CASUAL_CALLING_CONTEXT_H_

#include "common/service/call/state.h"
#include "common/flag/service/call.h"

//#include "common/flag/xa.h"

#include "common/message/service.h"
#include "common/exception/xatmi.h"




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
            namespace async = flag::service::call::async;

            namespace reply
            {
               using namespace flag::service::call::reply;
               struct Result
               {
                  common::buffer::Payload buffer;
                  long user = 0;
                  descriptor_type descriptor;
               };
            } // reply

            namespace sync
            {
               using namespace flag::service::call::sync;
               struct Result
               {
                  common::buffer::Payload buffer;
                  long user = 0;
               };
            } // sync

            //!
            //! Will be thrown if service fails (application error)
            //!
            struct Fail : common::exception::xatmi::service::Fail
            {
               using common::exception::xatmi::service::Fail::Fail;
               reply::Result result;
            };

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

            inline Context& context() { return Context::instance();}

         } // call
      } // service
	} // common
} // casual


#endif /* CASUAL_CALLING_CONTEXT_H_ */
