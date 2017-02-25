//!
//! casual
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

            void call( const std::string& service, buffer::Buffer& input, buffer::Buffer& output, long flags)
            {
               Trace trace{ "service::call"};

               log::sf << "input: " << input << std::endl;


               auto out = output.release();
               long size = out.size;

               if( tpcall( service.data(), input.data(), input.size(), &out.buffer, &size, flags) == -1)
               {
                  output.reset( out);
                  throw exception::NotReallySureWhatToCallThisExcepion( common::error::xatmi::error( tperrno));
               }

               out.size = size;
               output.reset( out);

               log::sf << "output: " << output << std::endl;


            }

            call_descriptor_type send( const std::string& service, buffer::Buffer& input, long flags)
            {
               Trace trace{ "service::send"};

               log::sf << "input: " << input << std::endl;

               auto callDescriptor = tpacall( service.c_str(), input.data(), input.size(), flags);

               if( callDescriptor == -1)
               {
                  throw exception::NotReallySureWhatToCallThisExcepion( common::error::xatmi::error( tperrno));
               }
               return callDescriptor;

            }

            bool receive( call_descriptor_type& callDescriptor, buffer::Buffer& output, long flags)
            {
               Trace trace{ "service::receive"};

               auto out = output.release();
               long size = out.size;
               auto cd = tpgetrply( &callDescriptor, &out.buffer, &size, flags);

               if( cd == -1)
               {
                 switch( tperrno)
                 {
                    case TPEBLOCK:
                    {
                       return false;
                    }
                    case TPETIME:
                    {
                       throw exception::xatmi::Timeout{ "deadline reached for descriptor: " + std::to_string( callDescriptor), __FILE__, __LINE__};
                    }
                    default:
                    {
                       output.reset( out);
                       throw exception::xatmi::System( common::error::xatmi::error( tperrno));
                    }
                 }
               }
               out.size = size;
               output.reset( out);

               log::sf << "output: " << output << std::endl;

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
