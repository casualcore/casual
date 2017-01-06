//!
//! monitor.h
//!
//! Created on: Jul 13, 2013
//!     Author: Lazan
//!

#ifndef MONITOR_H_
#define MONITOR_H_

#include "transaction/manager/state.h"



//
// std
//
#include <string>

namespace casual
{


   namespace transaction
   {
      namespace environment
      {
         namespace log
         {
            std::string file();
         } // log

      } // environment


      struct Settings
      {
         Settings();

         std::string log;
         std::string configuration;
      };


      class Manager
      {
      public:

         Manager( const Settings& settings);
         ~Manager();

         void start();

         const State& state() const;

      private:

         void handle_pending();

         State m_state;
      };

      namespace message
      {
         void pump( State& state);

      } // message
   } // transaction
} // casual



#endif /* MONITOR_H_ */
