

#include "casual_archivereader.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {

         Reader::Reader()
         {

         }

         Reader::~Reader()
         {

         }

         void Reader::read( bool& value)
         {
            readPOD( value);
         }

         void Reader::read( char& value)
         {
            readPOD( value);
         }

         void Reader::read( short& value)
         {
            readPOD( value);
         }

         void Reader::read( int& value)
         {
            long temp;
            readPOD( temp);
            value = temp;
         }

         //void read (const unsigned int& value);

         void Reader::read( long& value)
         {
            readPOD( value);
         }

         void Reader::read (unsigned long& value)
         {
            long temp;
            readPOD( temp);
            value = temp;
         }

         void Reader::read( float& value)
         {
            readPOD( value);
         }

         void Reader::read( double& value)
         {
            readPOD( value);
         }

         void Reader::read( std::string& value)
         {
            readPOD( value);
         }

         void Reader::read( std::vector< char>& value)
         {
            readPOD( value);
         }
      }
   }
}

