//!
//! casual_error.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "common/error.h"
#include "common/logger.h"
#include "common/exception.h"
#include "common/transaction_context.h"


#include <string.h>
#include <errno.h>



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


         int handler()
         {
            try
            {
               throw;
            }
            catch( const exception::xatmi::severity::Error& exception)
            {
               logger::error << tperrnoStringRepresentation( exception.code()) << " - " << exception.what();
               return exception.code();
            }
            catch( const exception::xatmi::severity::Information& exception)
            {
               logger::information << tperrnoStringRepresentation( exception.code()) << " - " << exception.what();
               return exception.code();
            }
            catch( const exception::xatmi::severity::User& exception)
            {
               logger::debug << tperrnoStringRepresentation( exception.code()) << " - " << exception.what();
               return exception.code();
            }
            /*
            catch( const exception::tx::severity::Error& exception)
            {
               logger::error << transaction::txError( exception.code()) << " - " << exception.what();
               return exception.code();
            }
            catch( const exception::tx::severity::Information& exception)
            {
               logger::information << transaction::txError( exception.code()) << " - " << exception.what();
               return exception.code();
            }
            catch( const exception::tx::severity::User& exception)
            {
               logger::debug << transaction::txError( exception.code()) << " - " << exception.what();
               return exception.code();
            }
            */
            catch( const std::exception& exception)
            {
               logger::error << tperrnoStringRepresentation( TPESYSTEM) << " - " << exception.what();
               return TPESYSTEM;
            }
            catch( ...)
            {
               logger::error << tperrnoStringRepresentation( TPESYSTEM) << " uexpected exception";
               return TPESYSTEM;
            }


            return -1;
         }


         std::string stringFromErrno()
         {
            return strerror( errno);
         }

         const std::string& tperrnoStringRepresentation( int error)
         {
            static const std::map< int, std::string> mapping{
               { TPEBADDESC, "TPEBADDESC"},
               { TPEBLOCK, "TPEBLOCK"},
               { TPEINVAL, "TPEINVAL"},
               { TPELIMIT, "TPELIMIT"},
               { TPENOENT, "TPENOENT"},
               { TPEOS, "TPEOS"},
               { TPEPROTO, "TPEPROTO"},
               { TPESVCERR, "TPESVCERR"},
               { TPESVCFAIL, "TPESVCFAIL"},
               { TPESYSTEM, "TPESYSTEM"},
               { TPETIME, "TPETIME"},
               { TPETRAN, "TPETRAN"},
               { TPGOTSIG, "TPGOTSIG"},
               { TPEITYPE, "TPEITYPE"},
               { TPEOTYPE, "TPEOTYPE"},
               { TPEEVENT, "TPEEVENT"},
               { TPEMATCH, "TPEMATCH"},
            };


            auto findIter = mapping.find( error);

            if( findIter != mapping.end())
            {
               return findIter->second;
            }
            else
            {
               static const std::string noEntryFound = "No string representation was found";
               return noEntryFound;
            }
         }
      } // error
	} // utility
} // casual



