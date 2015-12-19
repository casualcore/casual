//!
//! queue.cpp
//!
//! Created on: May 31, 2015
//!     Author: Lazan
//!

#include "common/queue.h"



namespace casual
{
   namespace common
   {

      namespace queue
      {
         namespace policy
         {
            void Ignore::apply()
            {

            }

            void Timeout::apply()
            {
               try
               {
                  throw;
               }
               catch( const exception::signal::Timeout&)
               {
                  throw;
               }
               catch( const exception::Shutdown&)
               {
                  throw;
               }
               catch( const exception::queue::Unavailable&)
               {
                  throw;
               }
               catch( ...)
               {

               }
            }


            void NoAction::apply()
            {
               throw;
            }


            namespace callback
            {
               namespace on
               {
                  Terminate::Terminate( callback_type callback) : m_callback( std::move( callback)) {}

                  void Terminate::apply()
                  {
                     try
                     {
                        throw;
                     }
                     catch( const exception::signal::child::Terminate& exception)
                     {
                        auto terminated = process::lifetime::ended();
                        for( auto& death : terminated)
                        {
                           m_callback( death);
                        }
                     }
                  }
               } // on
            } // callback

         } // policy
      } // queue
   } // common
} // casual


