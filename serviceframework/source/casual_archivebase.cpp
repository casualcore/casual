/*
 * casual_archivebase.cpp
 *
 *  Created on: Sep 16, 2012
 *      Author: lazan
 */

#include "casual_archivebase.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         Base::Base()
         {

         }

         Base::~Base()
         {

         }

         void Base::handleStart( const char* name)
         {
            handle_start( name);
         }

         void Base::handleEnd( const char* name)
         {
            handle_end( name);
         }

         void Base::handleContainerStart()
         {
            handle_container_start();
         }

         void Base::handleContainerEnd()
         {
            handle_container_end();
         }

         void Base::handleSerialtypeStart()
         {
            handle_serialtype_start();
         }

         void Base::handleSerialtypeEnd()
         {
            handle_serialtype_end();
         }
      }

   }
}

