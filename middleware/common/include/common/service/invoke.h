//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_INVOKE_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_INVOKE_H_

#include "common/buffer/type.h"
#include "common/flag.h"
#include "common/service/header.h"

#include "xatmi/defines.h"

namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace invoke
         {
            struct Service
            {
               std::string name;
            };
            struct Parameter
            {
               enum class Flag : long
               {
                  no_transaction = TPNOTRAN,
                  send_only = TPSENDONLY,
                  receive_only = TPRECVONLY,
                  no_time = TPNOTIME,
               };
               using Flags = common::Flags< Flag>;

               Parameter() = default;
               Parameter( buffer::Payload&& payload) : payload( std::move( payload)) {}

               Flags flags;
               Service service;
               std::string parent;
               buffer::Payload payload;
               std::vector< common::service::header::Field> header;
               platform::descriptor::type descriptor = 0;
            };

            struct Result
            {
               enum class Transaction : int
               {
                  commit = TPSUCCESS,
                  rollback = TPFAIL
               };

               Result() = default;
               Result( buffer::Payload&& payload) : payload( std::move( payload)) {}

               buffer::Payload payload;
               long code = 0;
               Transaction transaction = Transaction::commit;

               friend std::ostream& operator << ( std::ostream& out, Transaction value);
               friend std::ostream& operator << ( std::ostream& out, const Result& value);
            };

            struct Forward
            {
               Parameter parameter;
            };

         } // invoke
      } // service
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_INVOKE_H_
