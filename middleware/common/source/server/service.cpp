//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/server/service.h"
#include "common/server/context.h"
#include "common/log.h"

#include "common/algorithm.h"
#include "common/memory.h"
#include "common/buffer/pool.h"

#include "common/exception/capture.h"
#include "common/code/raise.h"
#include "common/code/xatmi.h"
#include "common/code/casual.h"

#include "casual/xatmi/flag.h" 
#include "common/transaction/context.h"

namespace casual
{
   namespace common
   {
      namespace server
      {

         Service::Service( string::Argument name, function_type function, service::transaction::Type transaction, service::visibility::Type visibility, string::Argument category)
           : name( std::move( name)), function( std::move( function)), transaction( transaction), visibility{ visibility}, category( std::move( category)) {}

         Service::Service( string::Argument name, function_type function)
         : name( std::move( name)), function( std::move( function)) {}

         service::invoke::Result Service::operator () ( service::invoke::Parameter&& argument)
         {
            return function( std::move( argument));
         }

         bool operator == ( const Service& lhs, const Service& rhs)
         {
            return lhs == rhs.compare;
         }

         bool operator == ( const Service& lhs, const void* rhs)
         {
            if( lhs.compare && rhs)
            {
               return lhs.compare == rhs;
            }
            return false;
         }
         bool operator != ( const Service& lhs, const Service& rhs)
         {
            return ! ( lhs == rhs);
         }

         bool operator == ( const Service& lhs, const std::string& rhs)
         {
            return lhs.name == rhs;
         }

         namespace xatmi
         {
            namespace local
            {
               namespace
               {
                  namespace transform
                  {
                     TPSVCINFO information( service::invoke::Parameter& argument)
                     {
                        Trace trace{ "server::xatmi::local::transform::information"};

                        TPSVCINFO result{};

                        // Before we call the user function we have to add the buffer to the "buffer-pool"
                        algorithm::copy_max( argument.service.name, range::make( result.name));

                        result.len = argument.payload.data.size();
                        result.cd = argument.descriptor.value();
                        result.flags = argument.flags.underlying();

                        // This is the only place where we use adopt
                        result.data = buffer::pool::Holder::instance().adopt( std::move( argument.payload)).underlying();

                        return result;
                     }

                     buffer::Payload payload( const server::state::Jump& jump)
                     {
                        if( jump.buffer.data)
                           return buffer::pool::holder().release( jump.buffer.data, jump.buffer.size);

                        return { nullptr};
                     }

                     service::invoke::Result result( const server::state::Jump& jump)
                     {
                        service::invoke::Result result{ transform::payload( jump)};

                        result.code = jump.state.code;
                        result.transaction = jump.state.value == flag::xatmi::Return::success ?
                              service::invoke::Result::Transaction::commit : service::invoke::Result::Transaction::rollback;

                        return result;
                     }

                     service::invoke::Forward forward( const server::state::Jump& jump)
                     {
                        service::invoke::Forward result;

                        result.parameter.payload =  transform::payload( jump);
                        result.parameter.service.name = jump.forward.service;

                        return result;
                     }

                  } // transform

                  struct Invoke
                  {
                     Invoke( function_type function) : m_function( std::move( function))
                     {
                     }

                     service::invoke::Result operator () ( service::invoke::Parameter&& argument)
                     {
                        auto& state = server::context().state();

                        // Set destination for the coming jump...
                        // we can't wrap the jump in some abstraction since it's
                        // UB (http://en.cppreference.com/w/cpp/utility/program/setjmp)
                        switch( ::setjmp( state.jump.environment))
                        {
                           case state::Jump::Location::c_no_jump:
                           {
                              invoke( argument);
                              
                              // User service returned, not by tpreturn.
                              // We will get here when COBOL Api  TPRETURN is called
                              // followed by COBOL program returning to its caller (=us! Possibly 
                              // thru intervening Cobol runtime stuff.) In that case this may be 
                              // a normal path instead of an error. We need to check if TPRETURN
                              // has been called, and pick upp the response it saved in context.
                              // This means doing the "same" as in the c_return case below.
                              if (state.TPRETURN_called) {
                                 log::line( log::debug, "user called TPRETURN");
                                 return transform::result( state.jump);
                              } else {
                                 code::raise::error( code::xatmi::service_error, "service did not call tpreturn - ", argument.service.name);
                              }
                           }
                           case state::Jump::Location::c_forward:
                           {
                              log::line( log::debug, "user called tpforward");
                              throw transform::forward( state.jump);
                           }
                           default:
                           {
                              code::raise::error( code::casual::internal_unexpected_value, "unexpected value from setjmp");
                           }
                           case state::Jump::Location::c_return:
                           {
                              log::line( log::debug, "user called tpreturn");
                              return transform::result( state.jump);
                           }
                        }
                     }

                  private:

                     void invoke( service::invoke::Parameter& argument)
                     {
                        auto& state = server::context().state();

                        // Type of buffer needed by Cobol API TPSVCSTART(), so save information.
                        // dismantle() returns a tuple with two "range".
                        // This seems to do what we want. (Could save "combined" type in state
                        // instead... Then split it in TPSVCSTART when/if needed. Might be nicer)
                        // casual::common::string has a split that returns a vector<string>. Would perhaps
                        // be nicer to use that. More "natural" than to use the ranges currently returned
                        // by dismantle()... Should dismantle() return vector<string> instead? 
                        auto split_type=casual::common::buffer::type::dismantle(argument.payload.type);
                        state.buffer_type=std::string {std::begin(std::get<0>(split_type)), std::end(std::get<0>(split_type))};
                        state.buffer_subtype=std::string {std::begin(std::get<1>(split_type)), std::end(std::get<1>(split_type))};
                        // Note that saving buffer tpe information need to be done before
                        // transform::information() below. The transform  "move" data from
                        // argument and looses the buffer type information.

                        // Also takes care of buffer to pool
                        TPSVCINFO information = transform::information( argument);

                        // save service argument for possible use from Cobol api TPSVCSTART
                        state.information.argument = information;

                        // Initialize flag for detecting service normal "return" after a call
                        // to TPRETURN, i.e. The COBOL api method of returning service data
                        // before a normal return. The C api tpreturn() saves the service data
                        // and uses a long_jump. When using the C api a normal return is an error,
                        // but it is "normal" when the Cobol API is used, as long as the Cobol
                        // API TPRETURN has been called to supply the service "result".  
                        state.TPRETURN_called = false;

                        try
                        {
                           m_function( &information);
                        }
                        catch( ...)
                        {
                           log::line( log::category::error, code::casual::invalid_semantics, 
                              " exception thrown from service: ", argument.service.name, " - ", exception::capture());
                        }
                     }

                     function_type m_function;
                  };



               } // <unnamed>
            } // local

            server::Service service( std::string name, function_type function, service::transaction::Type transaction, service::visibility::Type visibility, std::string category)
            {
               auto compare = address( function);
               server::Service result{ std::move( name), local::Invoke{ std::move( function)}, transaction, visibility, std::move( category)};
               result.compare = compare;

               return result;
            }

            server::Service service( std::string name, function_type function)
            {
               return service( std::move( name), std::move( function), service::transaction::Type::automatic, service::visibility::Type::discoverable, {});
            }

            const void* address( const function_type& function)
            {
               auto target = function.target<void(*)(TPSVCINFO*)>();

               if( target) { return reinterpret_cast< void*>( *target);}

               return target;
            }

         } // xatmi



      } // server
   } // common
} // casual
