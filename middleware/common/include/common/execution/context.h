//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/strong/id.h"
#include <string>

namespace casual
{
   namespace common::execution
   {

      using type = strong::execution::id;

      namespace context
      {
         struct Parent
         {
            strong::execution::span::id span;
            std::string service;

            inline friend bool operator == ( const Parent&, const Parent&) = default;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( span);
               CASUAL_SERIALIZE( service);
            )
         };
         
      } // context

      struct Context
      {
         strong::execution::id id;
         strong::execution::span::id span;
         std::string service;
         context::Parent parent;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( id);
            CASUAL_SERIALIZE( span);
            CASUAL_SERIALIZE( service);
            CASUAL_SERIALIZE( parent);  
         )
      };

      namespace context
      {
         //! @returns the curren execution context
         const Context& get();

         //! clears all execution context state
         void clear();

         //! resets/regenerate id, span. clears parent
         void reset();

         namespace id
         {
            //! sets a new execution id. @returns the old.
            strong::execution::id set( strong::execution::id id);

            //! sets a new random execution id.
            strong::execution::id reset();
         } // id

         namespace span
         {
            strong::execution::span::id set( strong::execution::span::id id);
            strong::execution::span::id reset();            
         } // span
         
         namespace service
         {
            //! Sets the current service
            void set( const std::string& service);

            //! clear the service name
            void clear();
         } // service

         namespace parent
         {
            namespace service
            {
               void set( const std::string& service);
            } // service

            namespace span
            {
               void set( strong::execution::span::id id);
            } // span
            
         } // parent
         
      } // context

   } // common::execution
} // casual
