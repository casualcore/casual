/*
 * casual_archivewriter.cpp
 *
 *  Created on: Sep 16, 2012
 *      Author: lazan
 */

#include "casual_archivewriter.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {

         Writer::Writer()
         {

         }

         Writer::~Writer()
         {

         }

         void Writer::write( const bool value)
         {
            writePOD( value);
         }

         void Writer::write( const char value)
         {
            writePOD( value);
         }

         void Writer::write( const short value)
         {
            writePOD( value);
         }

         void Writer::write( const int value)
         {
            writePOD( static_cast< long>( value));
         }

         //void write (const unsigned int& value);

         void Writer::write( const long value)
         {
            writePOD( value);
         }

         void Writer::write (const unsigned long value)
         {
            writePOD( static_cast< long>( value));
         }

         void Writer::write( const float value)
         {
            writePOD( value);
         }

         void Writer::write( const double value)
         {
            writePOD( value);
         }

         void Writer::write( const std::string& value)
         {
            writePOD( value);
         }

         void Writer::write( const std::vector< char>& value)
         {
            writePOD( value);
         }
      }
   }
}

