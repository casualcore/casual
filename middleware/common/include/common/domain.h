//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"
#include "common/uuid.h"
#include "common/file.h"
#include "common/process.h"
#include "common/strong/id.h"

#include <string>
#include <iosfwd>

namespace casual
{
   namespace common::domain
   {

      struct Identity
      {
         Identity();
         Identity( const strong::domain::id& id, std::string name);
         explicit Identity( std::string name);

         strong::domain::id id;
         std::string name;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( id);
            CASUAL_SERIALIZE( name);
         )

         inline explicit operator bool() const noexcept { return predicate::boolean( id);}

         friend bool operator == ( const Identity& lhs, const Identity& rhs);
         inline friend bool operator != ( const Identity& lhs, const Identity& rhs) { return ! ( lhs == rhs);}
         friend bool operator < ( const Identity& lhs, const Identity& rhs);

         inline friend bool operator == ( const Identity& lhs, std::string_view rhs) { return lhs.name == rhs;}

      };

      const Identity& identity();

      //! sets the domain identity
      //! Should only be used by casual-domain
      //!
      //! @param value the new identity
      void identity( Identity value);

      namespace singleton
      {
         struct Model
         {
            process::Handle process;
            Identity identity;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( identity);
            )
         };

         //! using process::handle() and domain::identity() as information
         //! for the singleton file
         file::scoped::Path create();

         Model read();

      } // singleton

   } // common::domain
} // casual


