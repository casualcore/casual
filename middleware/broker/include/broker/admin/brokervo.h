//!
//! casual
//!

#ifndef BROKERVO_H_
#define BROKERVO_H_
#include "sf/namevaluepair.h"
#include "sf/platform.h"


namespace casual
{
   namespace broker
   {
      namespace admin
      {
         struct InstanceVO
         {

            enum class State : char
            {
               idle,
               busy,
               remote,
            };

            common::process::Handle process;
            State state;
            std::size_t invoked;
            sf::platform::time_point last;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( process);
               archive & CASUAL_MAKE_NVP( state);
               archive & CASUAL_MAKE_NVP( invoked);
               archive & CASUAL_MAKE_NVP( last);
            })

            friend bool operator < ( const InstanceVO& lhs, const InstanceVO& rhs) { return lhs.process.pid < rhs.process.pid;}
            friend bool operator == ( const InstanceVO& lhs, const InstanceVO& rhs) { return lhs.process.pid == rhs.process.pid;}
         };


         struct ServiceVO
         {
            std::string name;
            std::chrono::microseconds timeout;
            std::vector< sf::platform::pid::type> instances;
            std::size_t lookedup = 0;
            std::size_t type = 0;
            std::size_t transaction = 0;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( timeout);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( lookedup);
               archive & CASUAL_MAKE_NVP( type);
               archive & CASUAL_MAKE_NVP( transaction);
            })
         };

         struct PendingVO
         {
            std::string requested;
            common::process::Handle process;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( requested);
               archive & CASUAL_MAKE_NVP( process);
            })
         };

         struct StateVO
         {

            std::vector< InstanceVO> instances;
            std::vector< ServiceVO> services;
            std::vector< PendingVO> pending;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( services);
               archive & CASUAL_MAKE_NVP( pending);
            })

         };

      } // admin
   } // broker


} // casual

#endif // BROKERVO_H_
