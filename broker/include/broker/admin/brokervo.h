//!
//! TODO: This should be generated with detoct...
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
            sf::platform::pid_type pid;
            sf::platform::queue_id_type queue;
            long state;
            long invoked;
            sf::platform::time_type last;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( pid);
               archive & CASUAL_MAKE_NVP( queue);
               archive & CASUAL_MAKE_NVP( state);
               archive & CASUAL_MAKE_NVP( invoked);
               archive & CASUAL_MAKE_NVP( last);
            }
         };

         struct ServerVO
         {
            std::string alias;
            std::string path;
            std::vector< InstanceVO> instances;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( alias);
               archive & CASUAL_MAKE_NVP( path);
               archive & CASUAL_MAKE_NVP( instances);
            }
         };

         struct ServiceVO
         {
            std::string name;
            //std::chrono::microseconds timeout;
            long long timeout = 0;
            std::vector< sf::platform::pid_type> instances;
            long lookedup = 0;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( timeout);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( lookedup);
            }
         };

         struct ShutdownVO
         {
            using pids = std::vector< sf::platform::pid_type>;

            pids online;
            pids offline;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( online);
               archive & CASUAL_MAKE_NVP( offline);
            })

         };

         namespace update
         {
            struct InstancesVO
            {
               std::string alias;
               std::size_t instances;

               template< typename A>
               void serialize( A& archive)
               {
                  archive & CASUAL_MAKE_NVP( alias);
                  archive & CASUAL_MAKE_NVP( instances);
               }
            };
         } // update



      } // admin
   } // broker


} // casual

#endif // BROKERVO_H_
