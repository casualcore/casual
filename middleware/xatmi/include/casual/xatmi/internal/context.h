//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/header.h"

#include "common/flag/xatmi.h"

namespace casual
{
   namespace xatmi::internal
   {
      namespace header
      {
         struct Context
         {
            void associate( common::buffer::handle::type handle, casual::header::Fields fields);
            void disassociate( common::buffer::handle::type handle) noexcept;

            const casual::header::Fields* find( common::buffer::handle::type handle) noexcept;

            //! find the `old_handle` in the context and update it to `new_handle`
            //! used when a buffer is reallocated, to keep the association
            void update_handle( common::buffer::handle::type old_handle, common::buffer::handle::type new_handle) noexcept;
            void clear() noexcept;

         private:
            struct Holder
            {
               common::buffer::handle::type handle;
               casual::header::Fields fields;

               friend bool operator == ( const Holder& lhs, common::buffer::handle::type rhs) { return lhs.handle == rhs; }
            };

            std::vector< Holder> m_fields;
         };
         
      } // header

      namespace state
      {
         struct Jump
         {
            enum Location : int
            {
               c_no_jump = 0,
               c_return = 10,
               c_forward = 20
            };

            platform::jump::buffer environment;

            struct Buffer
            {
               common::buffer::handle::type data;
               platform::buffer::raw::size::type size = 0;
            } buffer;

            struct State
            {
               common::flag::xatmi::Return value = common::flag::xatmi::Return::success;
               long code = 0;
            } state;

            struct Forward
            {
               std::string service;

            } forward;

            friend std::ostream& operator << ( std::ostream& out, const Jump& value);
         };

         struct Parameter
         {
            //XXX   service::invoke::Parameter argument;
            TPSVCINFO argument;
         };


      } // state

      struct State
      {
         state::Jump jump;
         // saved copy of service routine argument. Needed to support TPSVCSTART
         // that retrieves this information in a "callback" from the service. Part
         // of Cobol api support. 
         state::Parameter information;
         // The Cobol API TPSVCSTART also need the buffer type and subtype
         // so we save them also... 
         std::string buffer_type;
         std::string buffer_subtype;

         // The C api returns to casual via a long_jump in "tpreturn", while
         // the COBOL api uses a normal return after calling TPRETURN.
         // Casual does not know if a service uses the COBOL api or the
         // C api. To allow casual to detect that TPRETURN was called
         // before a normal return this flag will be set to false 
         // invocation of a service. It will be set to true if/when
         // the service return data is set via TPRETURN and
         // Context::normal_return(). It is left unchanged in
         // Context::jump_return().
         // NOTE: In theory a sequence 
         //   call TPRETURN
         //   call tpreturn (without a "return" after the call to TPRETURN)
         // is possible. I have ignored this for now. It is an illegal
         // sequence and is very unliklely as the Cobol COPY that calls TPRETURN
         // includes an EXIT PROGRAM statement.
         // If it should happen the jump_return will overwrite the information
         // saved by TPRETURN, leading to a resource leak.
         //
         // NOTE: Should perhaps be renamed to "service_normal_return" or something
         // something like that. It is set to true by Context::normal_return()...  
         bool TPRETURN_called{};
      };

      //! @attention Only used for XATMI. To hold stuff that is used in the XATMI context
      struct Context
      {
         //! @returns the current header context
         static Context& instance();

         //! Being called from tpreturn
         void jump_return( common::flag::xatmi::Return rval, long rcode, char* data, long len);

         //! Being called from TPRETURN (via ...)
         void normal_return( common::flag::xatmi::Return rval, long rcode, char* data, long len);

         //! called from extern casual_service_forward
         void forward( const char* service, char* data, long size);

         inline State& state() noexcept { return m_state; }
         inline header::Context& header() noexcept { return m_header; }

         void finalize();

      
      private:
         Context() = default;

         State m_state;
         header::Context m_header; //! holds the header context
         
      };

      //! @attention Only used for XATMI.
      inline Context& context() { return Context::instance();}

   } // xatmi::internal
   
} // casual
