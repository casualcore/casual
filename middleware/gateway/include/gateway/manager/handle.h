//!
//! handle.h
//!
//! Created on: Nov 8, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_HANDLE_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_HANDLE_H_

#include "gateway/manager/state.h"

namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace handle
         {

            struct Base
            {
               Base( State& state) : m_state( &state) {}

            protected:
               State& state() { return *m_state;}
            private:
               State* m_state;
            };


            namespace inbound
            {
               struct Connect : Base
               {
                  using Base::Base;


               };

            } // inbound



         } // handle

      } // manager

   } // gateway


} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_HANDLE_H_
