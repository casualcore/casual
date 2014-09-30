//!
//! casual_error.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "common/error.h"
#include "common/log.h"
#include "common/exception.h"
#include "common/transaction_context.h"
#include "common/process.h"


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
            catch( const exception::signal::Terminate& exception)
            {
               log::information << file::basename( process::path()) << " is off-line - " <<  exception << std::endl;
               return 0;
            }
            catch( const exception::invalid::Process& exception)
            {
               log::error << exception << std::endl;
            }
            catch( const exception::xatmi::severity::Error& exception)
            {
               log::error << xatmi::error( exception.code()) << " - " << exception.what() << std::endl;
               return exception.code();
            }
            catch( const exception::xatmi::severity::Information& exception)
            {
               log::information << xatmi::error( exception.code()) << " - " << exception.what() << std::endl;
               return exception.code();
            }
            catch( const exception::xatmi::severity::User& exception)
            {
               log::debug << xatmi::error( exception.code()) << " - " << exception.what() << std::endl;
               return exception.code();
            }
            /*
            catch( const exception::tx::severity::Error& exception)
            {
               log::error << transaction::txError( exception.code()) << " - " << exception.what();
               return exception.code();
            }
            catch( const exception::tx::severity::Information& exception)
            {
               log::information << transaction::txError( exception.code()) << " - " << exception.what();
               return exception.code();
            }
            catch( const exception::tx::severity::User& exception)
            {
               log::debug << transaction::txError( exception.code()) << " - " << exception.what();
               return exception.code();
            }
            */
            catch( const exception::Base& exception)
            {
               log::error << xatmi::error( TPESYSTEM) << " - " << exception.what() << std::endl;
               return TPESYSTEM;
            }
            catch( const std::exception& exception)
            {
               log::error << xatmi::error( TPESYSTEM) << " - " << exception.what() << std::endl;
               return TPESYSTEM;
            }
            catch( ...)
            {
               log::error << xatmi::error( TPESYSTEM) << " unexpected exception" << std::endl;
               return TPESYSTEM;
            }


            return -1;
         }


         std::string string()
         {
            return string( errno);
         }

         std::string string( int code)
         {
            return std::string( strerror( code)) + " (" + std::to_string( code) + ")";
         }

         namespace xatmi
         {
            std::string error( int code)
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


               auto findIter = mapping.find( code);

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
         } // xatmi


         namespace xa
         {
            const char* error( int code)
            {
               static const std::map< int, const char*> mapping{
                  { XA_RBROLLBACK, "XA_RBROLLBACK"},
                  { XA_RBCOMMFAIL, "XA_RBCOMMFAIL"},
                  { XA_RBDEADLOCK, "XA_RBDEADLOCK"},
                  { XA_RBINTEGRITY, "XA_RBINTEGRITY"},
                  { XA_RBOTHER, "XA_RBOTHER"},
                  { XA_RBPROTO, "XA_RBPROTO"},
                  { XA_RBTIMEOUT, "XA_RBTIMEOUT"},
                  { XA_RBTRANSIENT, "XA_RBTRANSIENT"},
                  { XA_NOMIGRATE, "XA_NOMIGRATE"},
                  { XA_HEURHAZ, "XA_HEURHAZ"},
                  { XA_HEURCOM, "XA_HEURCOM"},
                  { XA_HEURRB, "XA_HEURRB"},
                  { XA_HEURMIX, "XA_HEURMIX"},
                  { XA_RETRY, "XA_RETRY"},
                  { XA_RDONLY, "XA_RDONLY"},
                  { XA_OK, "XA_OK"},
                  { XAER_ASYNC, "XAER_ASYNC"},
                  { XAER_RMERR, "XAER_RMERR"},
                  { XAER_NOTA, "XAER_NOTA"},
                  { XAER_INVAL, "XAER_INVAL"},
                  { XAER_PROTO, "XAER_PROTO"},
                  { XAER_RMFAIL, "XAER_RMFAIL"},
                  { XAER_DUPID, "XAER_DUPID"},
                  { XAER_OUTSIDE, "XAER_OUTSIDE"}
               };

               return mapping.at( code);
            }
         } // xa

         namespace tx
         {
            const char* error( int code)
            {
               static const std::map< int, const char*> mapping{
                  { TX_NOT_SUPPORTED, "TX_NOT_SUPPORTED"},
                  { TX_OK, "TX_OK"},
                  { TX_OUTSIDE, "TX_OUTSIDE"},
                  { TX_ROLLBACK, "TX_ROLLBACK"},
                  { TX_MIXED, "TX_MIXED"},
                  { TX_HAZARD, "TX_HAZARD"},
                  { TX_PROTOCOL_ERROR, "TX_PROTOCOL_ERROR"},
                  { TX_ERROR, "TX_ERROR"},
                  { TX_FAIL, "TX_FAIL"},
                  { TX_EINVAL, "TX_EINVAL"},
                  { TX_COMMITTED, "TX_COMMITTED"},
                  { TX_NO_BEGIN, "TX_NO_BEGIN"},
                  { TX_ROLLBACK_NO_BEGIN, "TX_ROLLBACK_NO_BEGIN"},
                  { TX_MIXED_NO_BEGIN, "TX_MIXED_NO_BEGIN"},
                  { TX_HAZARD_NO_BEGIN, "TX_HAZARD_NO_BEGIN"},
                  { TX_COMMITTED_NO_BEGIN, "TX_COMMITTED_NO_BEGIN"}
               };

               return mapping.at( code);
            }
         } // tx

      } // error
	} // common
} // casual



