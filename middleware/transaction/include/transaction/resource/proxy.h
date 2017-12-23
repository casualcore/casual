//!
//! casual
//!

#ifndef RESOURCE_PROXY_H_
#define RESOURCE_PROXY_H_

#include "transaction/resource/proxy_server.h"

#include "common/platform.h"
#include "common/strong/id.h"


#include "sf/namevaluepair.h"

namespace casual
{
   namespace transaction
   {

      namespace resource
      {
         namespace proxy 
         {
            struct Settings
            {
               common::strong::resource::id::value_type id;
               std::string key;
               std::string openinfo;
               std::string closeinfo;
            };

            struct State
            {
               State( Settings&& settings, casual_xa_switch_mapping* switches)
                  : rm_id( settings.id), 
                  rm_key( std::move( settings.key)),
                  rm_openinfo( std::move( settings.openinfo)),
                  rm_closeinfo( std::move( settings.closeinfo)),
                  xa_switches( switches)
               {}

               common::strong::resource::id rm_id;

               std::string rm_key;
               std::string rm_openinfo;
               std::string rm_closeinfo;

               casual_xa_switch_mapping* xa_switches = nullptr;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( rm_id);
                  archive & CASUAL_MAKE_NVP( rm_key);
                  archive & CASUAL_MAKE_NVP( rm_openinfo);
                  archive & CASUAL_MAKE_NVP( rm_closeinfo);
               })

            };

            namespace validate
            {
               void state( const proxy::State& state);
            } // validate
         } // proxy 


         class Proxy
         {
         public:

            Proxy( proxy::Settings&& settings, casual_xa_switch_mapping* switches);
            ~Proxy();

            void start();

            proxy::State& state() { return m_state;}

         private:
            proxy::State m_state;
         };



      } // resource

   } // transaction

} // casual

#endif // RESOURCE_PROXY_H_
