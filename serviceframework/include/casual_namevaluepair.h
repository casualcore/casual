
#ifndef CASUAL_NAMEVALUEPAIR_H
#define sCASUAL_NAMEVALUEPAIR_H 1


#include <utility>

#include <iostream>


namespace casual
{
	namespace sf
    {


	   //!
	   //!
	   //!
	   template <typename T>
	   class NameValuePair : public std::pair< const char*, T*>
	   {

		 public:

			 explicit NameValuePair (const char* name, T& value)
			  :  std::pair< const char*, T*>( name, &value) {}

			 const char* getName () const
			 {
				 return this->first;
			 }

			 T& getValue () const
			 {
				 return *( this->second);
			 }

			 const T& getConstValue () const
			 {
				 return *( this->second);
			 }
	   };

	   template <typename T>
	   class NameValuePair< const T> : public std::pair< const char*, const T*>
	   {

		 public:

			 explicit NameValuePair (const char* name, const T& value)
			  :  std::pair< const char*, const T*>( name, &value) {}

			 const char* getName () const
			 {
				 return this->first;
			 }

			 const T& getConstValue () const
			 {
				 return *( this->second);
			 }
	   };


	   template <typename T>
	   class NameRValuePair : public std::pair< const char*, T>
	   {

		 public:

			 explicit NameRValuePair (const char* name, T&& value)
			  :  std::pair< const char*, T>( name, std::move( value)) {}

			 const char* getName () const
			 {
				 return this->first;
			 }

			 const T& getConstValue () const
			 {
				 return this->second;
			 }
	   };


	   namespace internal
	   {
	   	   template< typename T>
	   	   struct traits
	   	   {
	   		   typedef const T& value_type;
	   		static void print() { std::cout << "T" << std::endl; }
	   	   };

			template< typename T>
			struct traits< const T&>
			{
				typedef const T& value_type;
				static void print() { std::cout << "const T&" << std::endl; }
			};

			template< typename T>
			struct traits< T&>
			{
				typedef T& value_type;
				static void print() { std::cout << "T&" << std::endl; }
			};

	   }




	   template< typename T>
	   NameValuePair< T> makeNameValuePair( const char* name, T& value)
	   {
		  return NameValuePair< T>( name, value);
	   }


	   template< typename T>
	   NameValuePair< const T> makeNameValuePair( const char* name, const T& value)
	   {
		  return NameValuePair< const T>( name, value);
	   }

	   template< typename T>
	   NameRValuePair< const T> makeNameValuePair( const char* name, const T&& value)
	   {
		  return NameRValuePair< const T>( name, std::move( value));
	   }


    } // sf
} // casual


#define CASUAL_MAKE_NVP( member) \
   casual::sf::makeNameValuePair( #member, member)

//#define CASUAL_READ_WRITE( )



#endif
