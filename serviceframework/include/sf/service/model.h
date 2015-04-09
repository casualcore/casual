//!
//! model.h
//!
//! Created on: Mar 14, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_SF_SERVICE_MODEL_H_
#define CASUAL_SF_SERVICE_MODEL_H_


#include "sf/namevaluepair.h"

#include <string>
#include <vector>

namespace casual
{
   namespace sf
   {
      namespace service
      {

         struct Model
         {

            struct Type
            {
               enum
               {
                  type_container = 1,
                  type_composite,

                  type_integer,
                  type_float,
                  type_char,
                  type_boolean,

                  type_string,
                  type_binary,
               };

               Type() = default;
               Type( const char* role, std::size_t type) : role( role == nullptr ? "" : role), type( type) {}

               std::string role;
               std::size_t type;
               std::vector< Type> attribues;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( role);
                  archive & CASUAL_MAKE_NVP( type);
                  archive & CASUAL_MAKE_NVP( attribues);
               })
            };

            struct arguments_t
            {
               std::vector< Type> input;
               std::vector< Type> output;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( input);
                  archive & CASUAL_MAKE_NVP( output);
               })

            } arguments;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( arguments);
            })

         };


      } // service
   } // sf



} // casual

#endif // MODEL_H_
