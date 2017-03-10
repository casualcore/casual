//!
//! casual
//!


#include "common/error.h"
#include "common/log/category.h"
#include "common/exception.h"
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
         namespace local
         {
            namespace
            {
               namespace log
               {
                  template< typename E>
                  common::log::Stream& stream( E&& exception)
                  {
                     switch( exception.category())
                     {
                        case exception::code::log::Category::debug: return common::log::debug;
                        case exception::code::log::Category::information: return common::log::category::information;
                        default: return common::log::category::error;
                     }
                  }

               } // log


            } // <unnamed>
         } // local

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
            catch( const exception::xatmi::base& exception)
            {
               local::log::stream( exception) << "xatmi - " << exception << std::endl;
               return exception.code();
            }

            //
            // tx stuff
            //
            catch( const exception::tx::Fail& exception)
            {
               log::category::error << exception.what() << std::endl;
               return TX_FAIL;
            }
            catch( const exception::code::base& exception)
            {
               local::log::stream( exception) << exception.tag_name() << " - " << exception << std::endl;
               return exception.code();
            }
            catch( const exception::base& exception)
            {
               log::category::error << xatmi::error( TPESYSTEM) << " - " << exception.what() << std::endl;
               return TPESYSTEM;
            }
            catch( const std::exception& exception)
            {
               log::category::error << xatmi::error( TPESYSTEM) << " - " << exception.what() << " (" << type::name( exception) << ')' << std::endl;
               return TPESYSTEM;
            }
            catch( ...)
            {
               log::category::error << xatmi::error( TPESYSTEM) << " - unexpected exception" << std::endl;
               return TPESYSTEM;
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

         namespace xatmi
         {
            const std::string& error( const int code)
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


               const auto findIter = mapping.find( code);

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
            const char* error( const int code)
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

            const char* error( const int code)
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

            int handler()
            {
               try
               {
                  throw;
               }
               catch( const exception::tx::Protocol& exception)
               {
                  log::category::error << "TX_PROTOCOL_ERROR " << exception << std::endl;
                  return TX_PROTOCOL_ERROR;
               }
               catch( const exception::tx::Fail& exception)
               {
                  log::category::error << "TX_FAIL " << exception << std::endl;
                  return TX_FAIL;
               }
               catch( const exception::tx::no::Support& exception)
               {
                  log::category::transaction << "TX_NOT_SUPPORTED " << exception << std::endl;
                  return TX_NOT_SUPPORTED;
               }
               catch( const exception::tx::no::Begin& exception)
               {
                  log::category::transaction << "TX_NO_BEGIN " << exception << std::endl;
                  return TX_NO_BEGIN;
               }
               catch( ...)
               {
                  error::handler();
                  return TX_FAIL;
               }
            }

         } // tx

      } // error
	} // common
} // casual



