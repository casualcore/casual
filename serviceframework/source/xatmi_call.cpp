//!
//! xatmi_call.cpp
//!
//! Created on: May 25, 2013
//!     Author: Lazan
//!

#include "sf/xatmi_call.h"
#include "sf/exception.h"

#include "common/error.h"


#include "xatmi.h"



namespace casual
{
   namespace sf
   {
      namespace xatmi
      {
         namespace service
         {

            constexpr long valid_sync_flags()
            {
               return TPNOCHANGE | TPSIGRSTRT | TPNOTIME;
            }

            void call( const std::string& service, buffer::Base& input, buffer::Base& output, long flags)
            {
               common::Trace trace{ "service::call"};

               buffer::Raw in = input.raw();
               buffer::Raw out = output.release();

               if( tpcall( service.data(), in.buffer, in.size, &out.buffer, &out.size, flags) == -1)
               {
                  throw exception::NotReallySureWhatToCallThisExcepion( common::error::xatmi::error( tperrno));
               }

               output.reset( out);
            }

            call_descriptor_type send( const std::string& service, buffer::Base& input, long flags)
            {
               buffer::Raw in = input.raw();

               auto callDescriptor = tpacall( service.c_str(), in.buffer, in.size, flags);

               if( callDescriptor == -1)
               {
                  throw exception::NotReallySureWhatToCallThisExcepion( common::error::xatmi::error( tperrno));
               }
               return callDescriptor;
            }

            bool receive( call_descriptor_type& callDescriptor, buffer::Base& output, long flags)
            {
               buffer::Raw out = output.release();

               auto cd = tpgetrply( &callDescriptor, &out.buffer, &out.size, flags);

               if( cd == -1)
               {
                  switch( tperrno)
                  {
                     case TPEBLOCK:
                        return false;
                     default:
                        throw exception::NotReallySureWhatToCallThisExcepion( common::error::xatmi::error( tperrno));
                  }
               }
               return true;
            }

            void cancel( call_descriptor_type cd)
            {
               tpcancel( cd);

            }

         } // service
      } // xatmi
   } // sf
} // casual
