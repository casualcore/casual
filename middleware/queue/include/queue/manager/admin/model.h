//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"
#include "casual/platform.h"
#include "common/domain.h"


namespace casual
{
   namespace queue
   {
      namespace manager
      {
         namespace admin
         {
            namespace model
            {
               
               using size_type = platform::size::type;

               struct Group
               {
                  common::process::Handle process;

                  std::string name;
                  std::string queuebase;


                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( queuebase);
                  })
               };
               namespace remote
               {
                  struct Domain
                  {
                     common::process::Handle process;
                     size_type order = 0;
      
                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( process);
                        CASUAL_SERIALIZE( order);
                     })
                  };

                  struct Queue
                  {
                     std::string name;
                     common::strong::process::id pid;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( pid);
                     })

                     inline friend bool operator < ( const Queue& lhs, const Queue& rhs)
                     {
                        return std::tie( lhs.name, lhs.pid) 
                           < std::tie( rhs.name, rhs.pid);
                     }
                  };
                  
               } // remote



               struct Queue
               {
                  enum class Type : int
                  {
                     queue = 1,
                     error_queue = 2,
                  };

                  struct Retry 
                  {
                     size_type count = 0;
                     platform::time::unit delay{};

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( count);
                        CASUAL_SERIALIZE( delay);
                     })
                  };

                  inline Type type() const { return  error ? Type::queue : Type::error_queue;}

                  common::strong::process::id group;
                  common::strong::queue::id id;
                  std::string name;
                  Retry retry;
                  common::strong::queue::id error;

                  size_type count{};
                  size_type size{};
                  size_type uncommitted{};

                  struct
                  {
                     size_type dequeued{};
                     size_type enqueued{};

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( dequeued);
                        CASUAL_SERIALIZE( enqueued);
                     })

                  } metric;

                  platform::time::point::type last;
                  platform::time::point::type created;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( group);
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( retry);
                     CASUAL_SERIALIZE( error);
                     CASUAL_SERIALIZE( count);
                     CASUAL_SERIALIZE( size);
                     CASUAL_SERIALIZE( uncommitted);
                     CASUAL_SERIALIZE( metric);
                     CASUAL_SERIALIZE( last);
                     CASUAL_SERIALIZE( created);
                  })

                  inline friend bool operator < ( const Queue& lhs, const Queue& rhs)
                  {
                     if( lhs.type() != rhs.type())
                        return lhs.type() < rhs.type();
                     return lhs.name < rhs.name;
                  }
                  inline friend bool operator == ( const Queue& lhs, const std::string name) { return lhs.name == name;}
               };

               struct Message
               {
                  enum class State : int
                  {
                     enqueued = 1,
                     committed = 2,
                     dequeued = 3,
                  };

                  common::Uuid id;
                  common::strong::queue::id queue;
                  common::strong::queue::id origin;
                  platform::binary::type trid;
                  State state;
                  std::string reply;
                  size_type redelivered;
                  std::string type;

                  platform::time::point::type available;
                  platform::time::point::type timestamp;

                  size_type size;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( origin);
                     CASUAL_SERIALIZE( trid);
                     CASUAL_SERIALIZE( state);
                     CASUAL_SERIALIZE( reply);
                     CASUAL_SERIALIZE( redelivered);
                     CASUAL_SERIALIZE( type);
                     CASUAL_SERIALIZE( available);
                     CASUAL_SERIALIZE( timestamp);
                     CASUAL_SERIALIZE( size);
                  })
               };

               struct State
               {
                  std::vector< Group> groups;
                  std::vector< Queue> queues;

                  struct Remote
                  {
                     std::vector< remote::Domain> domains;
                     std::vector< remote::Queue> queues;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( domains);
                        CASUAL_SERIALIZE( queues);
                     })

                  } remote;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( groups);
                     CASUAL_SERIALIZE( queues);
                     CASUAL_SERIALIZE( remote);
                  })

               };


               struct Affected
               {
                  struct Queue
                  {
                     common::strong::queue::id id;
                     std::string name;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( id);
                        CASUAL_SERIALIZE( name);
                     })
                  };
                  
                  Queue queue;
                  size_type count = 0;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( count);
                  })
               };

            } // model
         } // admin
      } // manager
   } // queue
} // casual


