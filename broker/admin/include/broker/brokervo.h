//!
//! TODO: This should be generated with detoct...
//!

#ifndef BROKERVO_H_
#define BROKERVO_H_
#include "sf/namevaluepair.h"
#include "sf/types.h"
#include "common/types.h"


namespace casual
{
   namespace broker
   {
      namespace admin
      {
         struct InstanceVO
         {
            long pid;
            long queueId;
            long state;
            long invoked;
            common::time_type last;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( pid);
               archive & CASUAL_MAKE_NVP( queueId);
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
            long timeout;
            std::vector< long> instances;
            long lookedup;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( timeout);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( lookedup);
            }
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
