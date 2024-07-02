//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/execution/context.h"
#include "common/uuid.h"
#include "common/environment.h"
#include "common/exception/capture.h"


namespace casual
{
   namespace common::execution
   {
      namespace local
      {
         namespace
         {
            Context initialize()
            {
               Context result;

               if( auto value = environment::variable::get< execution::type>( environment::variable::name::execution::id))
                  result.id = *value;
               else 
                  result.id = execution::type::generate();

               result.span = strong::execution::span::id::generate();

               return result;
            }

            Context& context()
            {
               static Context singleton = initialize();
               return singleton;
            };


         } // <unnamed>
      } // local


      namespace context
      {
         const Context& get()
         {
            return local::context();
         }

         void clear()
         {
            local::context() = {};
         }

         void reset()
         {
            local::context().id = strong::execution::id::generate();
            local::context().span = strong::execution::span::id::generate();
            local::context().service = {};
            local::context().parent = {};
         };

         namespace id
         {
            strong::execution::id set( strong::execution::id id)
            {
               return std::exchange( local::context().id, id);
            }

            strong::execution::id reset()
            {
               return std::exchange( local::context().id, strong::execution::id::generate());
            }
            
         } // id

         namespace span
         {
            strong::execution::span::id set( strong::execution::span::id id)
            {
               return std::exchange( local::context().span, id);
            }

            strong::execution::span::id reset()
            {
               return std::exchange( local::context().span, strong::execution::span::id::generate());
            }      
         } // span
         
         namespace service
         {
            void set( const std::string& service)
            {
               local::context().service = service;
            }

            void clear()
            {
               local::context().service.clear();
            }
         } // service

         namespace parent
         {
            namespace service
            {
               void set( const std::string& service)
               {
                  local::context().parent.service = service;
               }
            } // service

            namespace span
            {
               void set( strong::execution::span::id id)
               {
                  local::context().parent.span = id;
               }
            } // span
            
         } // parent
         
      } // context

   } // common::execution
} // casual


