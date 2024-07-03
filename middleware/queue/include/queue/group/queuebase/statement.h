//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "sql/database.h"

namespace casual
{
   namespace queue::group::queuebase
   {
      struct Statement
      {
         sql::database::Statement enqueue;

         struct
         {
            sql::database::Statement first;
            sql::database::Statement first_id;
            sql::database::Statement first_match;

         } dequeue;

         struct
         {
            sql::database::Statement xid;
            sql::database::Statement nullxid;

         } state;

         sql::database::Statement commit1;
         sql::database::Statement commit2;
         sql::database::Statement commit3;

         sql::database::Statement rollback1;
         sql::database::Statement rollback2;
         sql::database::Statement rollback3;

         struct 
         {
            sql::database::Statement queues;
            sql::database::Statement message;
         } available;

         

         sql::database::Statement id;

         struct
         {
            sql::database::Statement queue;
            sql::database::Statement message;

         } information;

         struct
         {
            sql::database::Statement match;
            sql::database::Statement one_message;
         } peek;

         struct
         {
            sql::database::Statement first;
         } browse;

         
         sql::database::Statement restore;
         sql::database::Statement clear;

         struct
         {
            //! arguments: queue, id;
            sql::database::Statement remove;
         } message;

         struct
         {
            //! arguments: (queue)id;
            sql::database::Statement reset;
         } metric;

         struct
         {
            sql::database::Statement current;
         } size;
      };

      Statement statement( sql::database::Connection& connection);

   } // queue::group::queuebase
} // casual