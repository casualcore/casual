//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once



#include "queue/group/database.h"

#include "common/platform.h"
#include "common/message/pending.h"

#include <string>

namespace casual
{
   namespace queue
   {
      namespace group
      {
         using queue_id_type = common::platform::size::type;
         using size_type = common::platform::size::type;

         struct Settings
         {
            std::string queuebase;
            std::string name;
         };


         struct State
         {
            State( std::string filename, std::string name)
               : queuebase( std::move( filename), std::move( name)) 
            {
               queuebase.begin();
            }

            ~State() 
            {
               queuebase.commit();
            }


            std::unordered_map< std::string, queue_id_type> queue_id;

            Database queuebase;

            inline const std::string& name() const { return queuebase.name();}

            //! persist the queuebase ( commit and begin)
            inline void persist()
            {
               queuebase.commit();
               queuebase.begin();
            }
         
            struct  
            {
               template< typename M>
               void persist( M&& message, const common::process::Handle& destinations)
               {
                  persistent.emplace_back( std::forward< M>( message), destinations);
               }

               std::vector< common::message::pending::Message> persistent;
            } pending;

            

            //! A log to know if we already have notified TM about
            //! a given transaction.
            std::vector< common::transaction::ID> involved;
         };

         namespace message
         {
            void pump( group::State& state);
         } // message

         struct Server
         {
            Server( Settings settings);
            ~Server();


            int start() noexcept;

         private:
            State m_state;
         };
      } // group

   } // queue

} // casual


