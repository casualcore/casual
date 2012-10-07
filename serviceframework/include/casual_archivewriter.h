/*
 * casual_archivewriter.h
 *
 *  Created on: Sep 16, 2012
 *      Author: lazan
 */

#ifndef CASUAL_ARCHIVEWRITER_H_
#define CASUAL_ARCHIVEWRITER_H_


#include "casual_archivebase.h"
#include "casual_namevaluepair.h"


namespace casual
{
	namespace sf
	{


	   class ArchiveWriter : public ArchiveBase
	   {


	     public:

		   ArchiveWriter();
	         virtual ~OutputArchive();


	     private:

	         virtual void writePOD (const bool& value) = 0;

	         virtual void writePOD (const char& value) = 0;

	         virtual void writePOD (const short& value) = 0;

	         virtual void writePOD (const long& value) = 0;

	         virtual void writePOD (const float& value) = 0;

	         virtual void writePOD (const double& value) = 0;

	         virtual void writePOD (const std::string& value) = 0;

	         virtual void writePOD (const std::wstring& value) = 0;

	         virtual void writePOD (const std::vector< char>& value) = 0;









	         void write (const bool& value);

	         //void write (const signed char& value);

	         void write (const char& value);

	         //void write (const unsigned char& value);

	         void write (const short& value);

	         //void write (const unsigned short& value);

	         void write (const int& value);

	         //void write (const unsigned int& value);

	         void write (const long& value);

	         //void write (const unsigned long& value);

	         void write (const float& value);

	         void write (const double& value);

	         void write (const std::string& value);

	         void write (const std::wstring& value);

	         void write (const std::vector< char>& value);






	        template< typename T>
	        void write( const T& value);

	        template< typename T>
	        void write( const std::vector< T>& value);

	        template< typename T>
	        void write( const std::list< T>& value);

	        template< typename T>
	        void write( const std::deque< T>& value);

	        template< typename T>
	        void write( const std::set< T>& value);

	        template< typename K, typename V>
	        void write( const std::map< K, V>& value);

	        template< typename K, typename V>
	        void write( const std::pair< K, V>& valuePair);



	        template< typename T>
	        void marshalStdContainer( const T& container);


	        template< typename T>
	        friend  ArchiveWriter& operator << (ArchiveWriter& archive, const NameValuePair< T>& nameValuePair);




	   };



	}

}


#endif /* CASUAL_ARCHIVEWRITER_H_ */
