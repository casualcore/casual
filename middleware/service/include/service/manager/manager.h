//!
//! casual
//!

#ifndef CASUAL_SERVICE_MANAGER_MANAGER_H_
#define CASUAL_SERVICE_MANAGER_MANAGER_H_


#include "service/manager/state.h"

#include <string>


namespace casual
{
   namespace service
   {
      namespace manager
      {
         struct Settings
         {
            std::string forward;
         };
      } // manager


      class Manager
      {
      public:

         Manager( manager::Settings&& settings);
         ~Manager();

         void start();

         inline const manager::State& state() const { return m_state; }

      private:
         manager::State m_state;
      };

   } // service
} // casual




#endif /* CASUAL_BROKER_H_ */
