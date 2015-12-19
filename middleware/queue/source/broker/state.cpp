//!
//! state.cpp
//!
//! Created on: Aug 16, 2015
//!     Author: Lazan
//!

#include "queue/broker/state.h"

#include "common/algorithm.h"

namespace casual
{
   namespace queue
   {
      namespace broker
      {

         State::State( common::ipc::receive::Queue& receive) : receive( receive) {}
         State::State() : State( common::ipc::receive::queue()) {}


         State::Group::Group() = default;
         State::Group::Group( std::string name, id_type process) : name( std::move( name)), process( std::move( process)) {}


         bool operator == ( const State::Group& lhs, State::Group::id_type process) { return lhs.process == process;}


         std::vector< common::platform::pid_type> State::processes() const
         {
            std::vector< common::platform::pid_type> result;

            for( auto& group : groups)
            {
               result.push_back( group.process.pid);
            }

            return result;
         }

         void State::operator() ( common::process::lifetime::Exit death)
         {
            {
               auto found = common::range::find_if( groups, [=]( const Group& g){
                  return g.process.pid == death.pid;
               });

               if( found)
               {
                  groups.erase( found.first);
               }
               else
               {
                  // error?
               }
            }

            //
            // Remove all queues for the group
            //
            {

               auto predicate = [=]( decltype( *queues.begin())& value){
                  return value.second.process.pid == death.pid;
               };

               auto range = common::range::make( queues);

               while( range)
               {
                  range = common::range::find_if( range, predicate);

                  if( range)
                  {
                     queues.erase( range.first);
                     range = common::range::make( queues);
                  }
               }
            }
            //
            // Invalidate xa-requests
            //
            {
               for( auto& corr : correlation)
               {
                  for( auto& reqeust : corr.second.requests)
                  {
                     if( reqeust.group.pid == death.pid && reqeust.stage <= Correlation::Stage::pending)
                     {
                        reqeust.stage = Correlation::Stage::error;
                     }
                  }
               }
            }
         }


         State::Correlation::Correlation( id_type caller, const common::Uuid& reply_correlation, std::vector< Group::id_type> groups)
            : caller( std::move( caller)), reply_correlation{ reply_correlation}
         {
            common::range::move( groups, requests);
         }

         State::Correlation::Request::Request() = default;
         State::Correlation::Request::Request( Group::id_type group) : group( std::move( group)) {}



         bool State::Correlation::replied() const
         {
            return common::range::all_of( requests, []( const Request& r){ return r.stage >= Stage::error;});
         }

         State::Correlation::Stage State::Correlation::stage() const
         {
            auto max = common::range::min( requests, []( const Request& lhs, const Request& rhs)
                  {
                     return lhs.stage < rhs.stage;
                  });

            if( max)
            {
               return max->stage;
            }
            return Stage::empty;
         }


         void State::Correlation::stage( const id_type& id, Stage stage)
         {
            auto found = common::range::find_if( requests, [&]( const Request& r){ return r.group.pid == id.pid;});

            if( found)
            {
               found->stage = stage;
            }
         }

      } // broker
   } // queue
} // casual
