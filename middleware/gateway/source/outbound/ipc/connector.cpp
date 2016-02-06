//!
//! connector.cpp
//!
//! Created on: Dec 24, 2015
//!     Author: Lazan
//!

#include "gateway/outbound/ipc/connector.h"


namespace casual
{
   namespace gateway
   {
      namespace outbound
      {

         namespace ipc
         {

            State::State( Settings settings)
               : domain_path{ std::move( settings.domain_path)}
            {

            }


            Connector::Connector( Settings settings)
               : m_state{ std::move( settings)}
            {

            }

            void Connector::start()
            {
            }

         } // ipc

      } // outbound
   } // gateway
} // casual
