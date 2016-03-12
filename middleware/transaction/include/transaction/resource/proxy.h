//!
//! resource_proxy.h
//!
//! Created on: Aug 5, 2013
//!     Author: Lazan
//!

#ifndef RESOURCE_PROXY_H_
#define RESOURCE_PROXY_H_

#include "transaction/resource/proxy_server.h"

#include "common/platform.h"


#include "sf/namevaluepair.h"

namespace casual
{
   namespace transaction
   {

      namespace resource
      {


         struct State
         {
            std::size_t rm_id = 0;

            std::string rm_key;
            std::string rm_openinfo;
            std::string rm_closeinfo;

            casual_xa_switch_mapping* xaSwitches = nullptr;
            common::platform::queue_id_type tm_queue = 0;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( rm_id);
               archive & CASUAL_MAKE_NVP( rm_key);
               archive & CASUAL_MAKE_NVP( rm_openinfo);
               archive & CASUAL_MAKE_NVP( rm_closeinfo);
               archive & CASUAL_MAKE_NVP( tm_queue);
            }

         };

         namespace validate
         {
            void state( const State& state);
         } // validate


         class Proxy
         {
         public:

            Proxy( State&& state);
            ~Proxy();

            void start();

            State& state() { return m_state;}

         private:
            State m_state;
         };



      } // resource

   } // transaction

} // casual

#endif // RESOURCE_PROXY_H_
