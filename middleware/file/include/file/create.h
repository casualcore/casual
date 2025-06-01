//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "file/message.h"

#include "transaction/context.h"

#include "common/process.h"

namespace casual
{
   namespace file::message
   {

      namespace create
      {
         namespace blocking
         {
            auto request( std::filesystem::path path)
            {
               auto& transaction = transaction::context().current();

               if( transaction)
               {
                  // make sure to trigger an interaction with the TM
                  transaction.external();
               }
               else
               {
                  common::code::raise::error( common::code::casual::preconditions, "no transaction");
               }

               message::reserve::Request result;

               result.process = common::process::handle();

               result.trid = transaction.trid;
               result.path = std::move( path);

               return result;
            }
         } // blocking


         namespace non
         {
            namespace blocking
            {
               auto request( std::filesystem::path path)
               {
                  auto result = create::blocking::request( std::move( path));
                  result.wait = false;
                  return result;
               }
            } // blocking
         } // non

      } // create

   } // file::message
} // casual