/*
 * call_service.cpp
 *
 *  Created on: May 5, 2016
 *      Author: kristone
 */

#include <stdexcept>
#include <iostream>
#include <string>
#include <map>
#include <cstring>

#include "xatmi.h"
#include "common/error.h"
#include "common/arguments.h"



std::string type_from_input( const std::string& input)
{
   //
   // TODO: More fancy
   //

   const std::map<std::string::value_type,std::string> mappings
   {
      { '[', CASUAL_BUFFER_INI_TYPE},
      { '{', CASUAL_BUFFER_JSON_TYPE},
      { '<', CASUAL_BUFFER_XML_TYPE},
      { '%', CASUAL_BUFFER_YAML_TYPE},
   };

   const auto result = mappings.find( input.at( 0));

   if( result != mappings.end())
   {
      std::clog << "assuming type " << result->second << std::endl;
      return result->second;
   }

   throw std::invalid_argument( "failed to deduce type from input");

}


int main( int argc, char* argv[])
{

   const std::string types
   {
      "["
         CASUAL_BUFFER_BINARY_TYPE
         "|"
         CASUAL_BUFFER_INI_TYPE
         "|"
         CASUAL_BUFFER_JSON_TYPE
         "|"
         CASUAL_BUFFER_XML_TYPE
         "|"
         CASUAL_BUFFER_YAML_TYPE
      "]"
   };


   try
   {
      std::string service;
      std::string type;

      {
         casual::common::Arguments parser{ "client to call an XATMI-service (request from stdin and response to stdout)",
         {
            casual::common::argument::directive( { "-s", "--service"}, "(mandatory) service to call", service),
            casual::common::argument::directive( { "-f", "--format"}, "(semi-optional) type of buffer " + types, type),
         }};

         //
         // TODO: See tickets #45 and #46
         //
         parser.parse( argc, argv);
      }

      //
      // Read data from stdin
      //

      std::string payload;
      while( std::cin.peek() != std::istream::traits_type::eof())
      {
         payload.push_back( std::cin.get());
      }

      //
      // Check is user provided a type
      //

      if( type.empty())
      {
         //
         // Try to deduce type
         //
         type = type_from_input( payload);
      }

      //
      // Allocate a buffer
      //
      auto buffer = tpalloc( type.c_str(), nullptr, payload.size());

      if( ! buffer)
      {
         throw std::runtime_error( tperrnostring( tperrno));
      }

      //
      // Copy payload to buffer
      //
      std::memcpy( buffer, payload.data(), payload.size());

      long len = payload.size();

      //
      // Call the service
      //
      const auto result = tpcall( service.c_str(), buffer, len, &buffer, &len, 0);

      const auto error = result == -1 ? tperrno : 0;

      payload.assign( buffer, len);

      tpfree( buffer);

      //
      // Check some result
      //

      if( error)
      {
         if( error != TPESVCFAIL)
         {
            throw std::runtime_error( tperrnostring( error));
         }
      }

      //
      // Print the result
      //
      std::cout << payload << std::endl;


      if( error)
      {
         //
         // Should only be with TPESVCFAIL
         //
         throw std::logic_error( tperrnostring( error));
      }

      return 0;

   }
   catch( const std::exception& e)
   {
      std::cerr << e.what() << std::endl;
   }

   return -1;

}

