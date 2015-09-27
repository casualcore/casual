//!
//! casual_calling_context.h
//!
//! Created on: Jun 16, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_CALLING_CONTEXT_H_
#define CASUAL_CALLING_CONTEXT_H_

#include "common/call/state.h"

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

      namespace call
      {


         class Context
         {
         public:
            static Context& instance();


            descriptor_type async( const std::string& service, char* idata, long ilen, long flags);


            void reply( descriptor_type& descriptor, char** odata, long& olen, long flags);

            void sync( const std::string& service, char* idata, const long ilen, char*& odata, long& olen, const long flags);

            void cancel( descriptor_type cd);

            void clean();

            long user_code() const;
            void user_code( long code);

            //!
            //! @returns true if there are pending replies or associated transactions.
            //!  Hence, it's ok to do a service-forward if false is return
            //!
            bool pending() const;

         private:


            Context();

            bool receive( message::service::call::Reply& reply, descriptor_type descriptor, long flags);

            State m_state;

         };
      } // call
	} // common
} // casual


#endif /* CASUAL_CALLING_CONTEXT_H_ */
