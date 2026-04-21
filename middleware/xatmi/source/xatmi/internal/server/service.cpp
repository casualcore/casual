//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/xatmi/internal/server/service.h"
#include "casual/xatmi/internal/context.h"

#include "server/service/invoke.h"
#include "server/context.h"


#include "common/log.h"
#include "common/buffer/pool.h"



namespace casual
{
   namespace xatmi::internal::server::service
   {

      namespace local
      {
         namespace
         {
            namespace transform  
            {
               TPSVCINFO information( casual::server::service::invoke::Parameter& argument)
               {
                  common::Trace trace{ "server::xatmi::local::transform::information"};
                  common::log::debug( "argument: ", argument);

                  TPSVCINFO result{};

                  // Before we call the user function we have to add the buffer to the "buffer-pool"
                  common::algorithm::copy_max( argument.service.name, common::range::make( result.name));

                  result.len = argument.payload.data.size();
                  result.cd = argument.descriptor.value();
                  result.flags = std::to_underlying( argument.flags);

                  // This is the only place where we use adopt
                  result.data = common::buffer::pool::holder().adopt( std::move( argument.payload)).raw();

                  // if we have a header, we associate it with the buffer handle -> give user access to it via handle
                  if( ! argument.header.empty())
                     context().header.associate( common::buffer::handle::type{ result.data}, std::move( argument.header));

                  return result;
               }

               common::buffer::Payload payload( const internal::state::Jump& jump)
               {
                  if( jump.buffer.data)
                  {
                     context().header.disassociate( common::buffer::handle::type{ jump.buffer.data});
                     return common::buffer::pool::holder().release( jump.buffer.data, jump.buffer.size);
                  }

                  return { nullptr};
               }

               casual::header::Fields header( const internal::state::Jump& jump)
               {
                  auto handle = common::buffer::handle::type{ jump.buffer.data};

                  if( auto found = internal::context().header.find( handle))
                     return *found;

                  return {};
               }

               casual::server::service::invoke::Result result( const internal::state::Jump& jump)
               {
                  return casual::server::service::invoke::Result{ 
                     .header = transform::header( jump),
                     .payload = transform::payload( jump),
                     .code = { .result = jump.state.value, .user = jump.state.code}
                  };
               }

               casual::server::service::invoke::Forward forward( const internal::state::Jump& jump)
               {
                  casual::server::service::invoke::Forward result;

                  result.parameter.payload =  transform::payload( jump);
                  result.parameter.header = transform::header( jump);
                  result.parameter.service.name = jump.forward.service;
                  
                  return result;
               }

            } // transform

            struct Invoke
            {
               Invoke( function_type function) : m_function( std::move( function))
               {
               }

               casual::server::service::invoke::Result operator () ( casual::server::service::invoke::Parameter&& argument)
               {
                  auto& xatmi_state = xatmi::internal::context().state;

                  auto finalize_guard = common::execute::scope( [](){ xatmi::internal::context().finalize();});

                  // Set destination for the coming jump...
                  // we can't wrap the jump in some abstraction since it's
                  // UB (http://en.cppreference.com/w/cpp/utility/program/setjmp)
                  switch( ::setjmp( xatmi_state.jump.environment))
                  {
                     case xatmi::internal::state::Jump::Location::c_no_jump:
                     {
                        invoke( argument);
                        
                        // User service returned, not by tpreturn.
                        // We will get here when COBOL Api  TPRETURN is called
                        // followed by COBOL program returning to its caller (=us! Possibly 
                        // thru intervening Cobol runtime stuff.) In that case this may be 
                        // a normal path instead of an error. We need to check if TPRETURN
                        // has been called, and pick upp the response it saved in context.
                        // This means doing the "same" as in the c_return case below.
                        if (xatmi_state.TPRETURN_called) 
                        {
                           common::log::debug( "user called TPRETURN");
                           return transform::result( xatmi_state.jump);
                        } 
                        else 
                        {
                           common::code::raise::error( common::code::xatmi::service_error, "service did not call tpreturn - ", argument.service.name);
                        }
                     }
                     case xatmi::internal::state::Jump::Location::c_forward:
                     {
                        common::log::debug( "user called tpforward");
                        throw transform::forward( xatmi_state.jump);
                     }
                     default:
                     {
                        common::code::raise::error( common::code::casual::internal_unexpected_value, "unexpected value from setjmp");
                     }
                     case xatmi::internal::state::Jump::Location::c_return:
                     {
                        common::log::debug( "user called tpreturn");
                        return transform::result( xatmi_state.jump);
                     }
                  }
               }

            private:

               void invoke( casual::server::service::invoke::Parameter& argument)
               {

                  auto& xatmi_state = casual::xatmi::internal::context().state;

                  // Type of buffer needed by Cobol API TPSVCSTART(), so save information.
                  // dismantle() returns a tuple with two "range".
                  // This seems to do what we want. (Could save "combined" type in state
                  // instead... Then split it in TPSVCSTART when/if needed. Might be nicer)
                  // casual::common::string has a split that returns a vector<string>. Would perhaps
                  // be nicer to use that. More "natural" than to use the ranges currently returned
                  // by dismantle()... Should dismantle() return vector<string> instead? 
                  
                  auto [ type, subtype] =  casual::common::buffer::type::dismantle(argument.payload.type);
                  xatmi_state.buffer_type = std::string{ std::begin( type), std::end( type)};
                  xatmi_state.buffer_subtype = std::string{ std::begin( subtype), std::end( subtype)};

                  // Note that saving buffer tpe information need to be done before
                  // transform::information() below. The transform  "move" data from
                  // argument and looses the buffer type information.

                  // Also takes care of buffer to pool
                  TPSVCINFO information = transform::information( argument);

                  // save service argument for possible use from Cobol api TPSVCSTART
                  xatmi_state.information.argument = information;

                  // Initialize flag for detecting service normal "return" after a call
                  // to TPRETURN, i.e. The COBOL api method of returning service data
                  // before a normal return. The C api tpreturn() saves the service data
                  // and uses a long_jump. When using the C api a normal return is an error,
                  // but it is "normal" when the Cobol API is used, as long as the Cobol
                  // API TPRETURN has been called to supply the service "result".  
                  xatmi_state.TPRETURN_called = false;

                  try
                  {
                     m_function( &information);
                  }
                  catch( ...)
                  {
                     common::log::error( common::code::casual::invalid_semantics, 
                        "exception thrown from service: ", argument.service.name, " - ", common::exception::capture());
                  }
               }

               function_type m_function;
            };

            const void* address( const function_type& function)
            {
               auto target = function.target<void(*)(TPSVCINFO*)>();

               if( target) 
                  return reinterpret_cast< void*>( *target);

               return target;
            }


         } // <unnamed>
      } // local

      casual::server::Service create( std::string name, function_type function, common::service::transaction::Type transaction, common::service::visibility::Type visibility, std::string category)
      {
         auto compare = local::address( function);

         return casual::server::Service{
            .name = std::move( name),
            .function = local::Invoke{ std::move( function)},
            .transaction = transaction,
            .visibility = visibility,
            .category = std::move( category),
            .compare = compare
         };
      }


      casual::server::Service create( std::string name, function_type function)
      {
         return create( std::move( name), std::move( function), common::service::transaction::Type::automatic, common::service::visibility::Type::discoverable, {});
      }
      
   } // xatmi::internal::server::service
   
} // casual
