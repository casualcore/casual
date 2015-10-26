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
         struct Process
         {
            sf::platform::pid_type pid;
            sf::platform::queue_id_type queue;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( pid);
               archive & CASUAL_MAKE_NVP( queue);
            })
         };

         struct GroupVO
         {
            struct ResourceVO
            {
               std::size_t id;
               std::size_t instances;
               std::string key;
               std::string openinfo;
               std::string closeinfo;

               CASUAL_CONST_CORRECT_SERIALIZE({
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( instances);
                  archive & CASUAL_MAKE_NVP( key);
                  archive & CASUAL_MAKE_NVP( openinfo);
                  archive & CASUAL_MAKE_NVP( closeinfo);
               })
            };

            std::size_t id;
            std::string name;
            std::string note;

            std::vector< ResourceVO> resources;
            std::vector< std::size_t> dependencies;

            CASUAL_CONST_CORRECT_SERIALIZE({
               archive & CASUAL_MAKE_NVP( id);
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( note);
               archive & CASUAL_MAKE_NVP( resources);
               archive & CASUAL_MAKE_NVP( dependencies);
            })

            friend bool operator < ( const GroupVO& lhs, const GroupVO& rhs);
         };

         struct InstanceVO
         {

            enum class State : std::size_t
            {
               booted = 1,
               idle,
               busy,
               shutdown
            };

            Process process;
            State state;
            std::size_t invoked;
            sf::platform::time_point last;
            std::size_t server;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( process);
               archive & CASUAL_MAKE_NVP( state);
               archive & CASUAL_MAKE_NVP( invoked);
               archive & CASUAL_MAKE_NVP( last);
               archive & CASUAL_MAKE_NVP( server);
            })

            friend bool operator < ( const InstanceVO& lhs, const InstanceVO& rhs) { return lhs.process.pid < rhs.process.pid;}
            friend bool operator == ( const InstanceVO& lhs, const InstanceVO& rhs) { return lhs.process.pid == rhs.process.pid;}
         };

         struct ExecutableVO
         {
            std::size_t id;
            std::string alias;
            std::string path;
            std::vector< sf::platform::pid_type> instances;
            std::size_t configured_instances;

            bool restart;
            std::size_t deaths;

            std::vector< std::size_t> memberships;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( id);
               archive & CASUAL_MAKE_NVP( alias);
               archive & CASUAL_MAKE_NVP( path);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( configured_instances);
               archive & CASUAL_MAKE_NVP( restart);
               archive & CASUAL_MAKE_NVP( deaths);
               archive & CASUAL_MAKE_NVP( memberships);
            })
         };

         struct ServerVO : ExecutableVO
         {
            std::size_t invoked;
            std::vector< std::string> restrictions;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               ExecutableVO::serialize( archive);
               archive & CASUAL_MAKE_NVP( invoked);
               archive & CASUAL_MAKE_NVP( restrictions);
            })
         };

         struct ServiceVO
         {
            std::string name;
            std::chrono::microseconds timeout;
            std::vector< sf::platform::pid_type> instances;
            std::size_t lookedup = 0;
            std::size_t type = 0;
            std::size_t mode = 0;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( timeout);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( lookedup);
               archive & CASUAL_MAKE_NVP( type);
               archive & CASUAL_MAKE_NVP( mode);
            })
         };

         struct PendingVO
         {
            std::string requested;
            Process process;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( requested);
               archive & CASUAL_MAKE_NVP( process);
            })
         };

         struct StateVO
         {
            std::vector< GroupVO> groups;
            std::vector< ServerVO> servers;
            std::vector< ExecutableVO> executables;
            std::vector< InstanceVO> instances;
            std::vector< ServiceVO> services;
            std::vector< PendingVO> pending;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( groups);
               archive & CASUAL_MAKE_NVP( servers);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( services);
               archive & CASUAL_MAKE_NVP( pending);
            })

         };


         struct ShutdownVO
         {
            using pids = std::vector< sf::platform::pid_type>;

            StateVO state;

            pids online;
            pids offline;

            enum class Error : char
            {
               error,
               shutdown
            } error;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( state);
               archive & CASUAL_MAKE_NVP( online);
               archive & CASUAL_MAKE_NVP( offline);
               archive & CASUAL_MAKE_NVP( error);
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
