//!
//! casual
//!


#include "common/error.h"
#include "common/log/category.h"
#include "common/exception.h"
#include "common/exception/xatmi.h"
#include "common/exception/tx.h"
#include "common/transaction/context.h"
#include "common/process.h"


#include <cstring>
#include <cerrno>



#include <xatmi.h>

//
// std
//
#include <map>



namespace casual
{
   namespace common
   {

      namespace error
      {

         int last()
         {
            return errno;
         }

         std::error_condition condition()
         {
            return std::error_condition{ last(), std::system_category()};
         }


         int handler()
         {
            try
            {
               throw;
            }
            catch( const exception::Shutdown& exception)
            {
               log::category::information << "off-line" << std::endl;
               return 0;
            }
            catch( const exception::signal::Terminate& exception)
            {
               log::category::information << "terminated - " <<  exception << std::endl;
               log::debug << exception << std::endl;
               return 0;
            }
            catch( const exception::invalid::Process& exception)
            {
               log::category::error << exception << std::endl;
            }
            catch( const exception::invalid::Flags& exception)
            {
               log::category::error << exception << std::endl;
               return TPEINVAL;
            }

            //
            // xatmi stuff
            //
            catch( const exception::xatmi::exception& exception)
            {
               code::stream( exception.type()) << exception << std::endl;
               return exception.code().value();
            }

            //
            // tx stuff
            //
            catch( const exception::tx::exception& exception)
            {
               code::stream( exception.type()) << exception << std::endl;
               return exception.code().value();
            }


            catch( const std::system_error& exception)
            {
               log::category::error << exception << std::endl;
            }
            catch( const std::exception& exception)
            {
               log::category::error << exception << std::endl;
            }
            catch( ...)
            {
               log::category::error << " - unexpected exception" << std::endl;
            }

            return -1;
         }


         std::string string()
         {
            return string( errno);
         }

         std::string string( const int code)
         {
            return std::string( std::strerror( code)) + " (" + std::to_string( code) + ")";
         }



      } // error
	} // common
} // casual



