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

#include "xatmi.h"

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
	            result[ TPEBADDESC] = "Not a valid call-descriptor";
	            result[ TPEBLOCK] = "Blocking condition and TPNOBLOCK was not provided";
	            result[ TPEINVAL] = "Invalid arguments provided";
	            result[ TPELIMIT] = "Some limit has been reach, this should not be possible in casual...";
	            result[ TPENOENT] = "Service is not available";
	            result[ TPEOS] = "OS error - Critical error has been detected at the OS level";
	            result[ TPEPROTO] = "Protocol error - Inconsistency in casual's internal protocol has been detected";
	            result[ TPESVCERR] = "";
	            result[ TPESVCFAIL] = "";
	            result[ TPESYSTEM] = "";
	            result[ TPETIME] = "Timeout - A time limit has been reach";
	            result[ TPETRAN] = "";
	            result[ TPGOTSIG] = "";
	            result[ TPEITYPE] = "";
	            result[ TPEOTYPE] = "";
	            result[ TPEEVENT] = "";
	            result[ TPEMATCH] = "";



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
			catch( const std::exception& exception)
			{
				logger::error << "exception: " << exception.what();
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



