//!
//! casual_error.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "casual_error.h"


#include <string.h>
#include <errno.h>


#include "casual_logger.h"
#include "casual_exception.h"

#include "xatmi.h"


extern int tperrno;

//
// std
//
#include <map>



namespace casual
{
	namespace error
	{
	   namespace local
	   {
	      namespace
	      {
	         std::map< int, std::string> initializeTperrnoMapping()
            {
	            std::map< int, std::string> result;

	            //
	            // TODO: Make this locale-dependent?
	            //
	            result[ TPEBADDESC] = "TPEBADDESC - Not a valid call-descriptor";
	            result[ TPEBLOCK] = "TPEBLOCK - Blocking condition and TPNOBLOCK was not provided";
	            result[ TPEINVAL] = "TPEINVAL - Invalid arguments provided";
	            result[ TPELIMIT] = "TPELIMIT - Some limit has been reach, this should not be possible in casual...";
	            result[ TPENOENT] = "TPENOENT - Service is not available";
	            result[ TPEOS] = "TPEOS - OS error - Critical error has been detected at the OS level";
	            result[ TPEPROTO] = "TPEPROTO - Protocol error - Inconsistency in casual's internal protocol has been detected";
	            result[ TPESVCERR] = "TPESVCERR - ";
	            result[ TPESVCFAIL] = "TPESVCFAIL - ";
	            result[ TPESYSTEM] = "TPESYSTEM - ";
	            result[ TPETIME] = "TPETIME - Timeout - A time limit has been reach";
	            result[ TPETRAN] = "TPETRAN - ";
	            result[ TPGOTSIG] = "TPGOTSIG - ";
	            result[ TPEITYPE] = "TPEITYPE - ";
	            result[ TPEOTYPE] = "TPEOTYPE - ";
	            result[ TPEEVENT] = "TPEEVENT - ";
	            result[ TPEMATCH] = "TPEMATCH - ";



	            return result;
            }
	      }

	   }

		int handler()
		{
			try
			{
				throw;
			}
			catch( const exception::xatmi::severity::Critical& exception)
			{
			   // TODO: tperrno = exception.code();
				logger::error << tperrnoStringRepresentation( exception.code()) << " - " << exception.what();
			}
			catch( const exception::xatmi::severity::Information& exception)
         {
			   // TODO: tperrno = exception.code();
            logger::information << tperrnoStringRepresentation( exception.code()) << " - " << exception.what();
         }
			catch( const exception::xatmi::Base& exception)
         {
			   // TODO: tperrno = exception.code();
            logger::debug << tperrnoStringRepresentation( exception.code()) << " - " << exception.what();
         }
			catch( const std::exception& exception)
         {
			   // TODO: tperrno = TPESYSTEM;
			   logger::error << tperrnoStringRepresentation( TPESYSTEM) << " - " << exception.what();
         }


			return -1;
		}


		std::string stringFromErrno()
		{
			return strerror( errno);
		}

		const std::string& tperrnoStringRepresentation( int error)
		{
		   static const std::string noEntryFound = "No string representation was found";
		   static const std::map< int, std::string> mapping = local::initializeTperrnoMapping();

		   std::map< int, std::string>::const_iterator findIter = mapping.find( error);

		   if( findIter != mapping.end())
		   {
		      return findIter->second;
		   }
		   else
		   {
		      return noEntryFound;
		   }
		}

	}


}



