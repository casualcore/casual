
#ifndef CASUAL_NAMEVALUEPAIR_H
#define sCASUAL_NAMEVALUEPAIR_H 1


#include <utility>

#include <iostream>

#include <utility>
#include <type_traits>

namespace casual
{
	namespace sf
    {

	//std::true_type

	   template <typename T, typename R>
		class NameValuePair;


	   //!
	   //!
	   //!
	   template <typename T>
	   class NameValuePair< T, std::true_type> : public std::pair< const char*, T*>
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
	   class NameValuePair< const T, std::true_type> : public std::pair< const char*, const T*>
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
	   class NameValuePair< T, std::false_type> : public std::pair< const char*, T>
	   {

		 public:

			 explicit NameValuePair (const char* name, T&& value)
			  :  std::pair< const char*, T>( name, std::forward( value)) {}

			 const char* getName () const
			 {
				 return this->first;
			 }

			 const T& getConstValue () const
			 {
				 return this->second;
			 }
	   };


	   template< typename T>
	   NameValuePair< T, typename std::is_lvalue_reference<T>::type> makeNameValuePair( const char* name, T&& value)
	   {
		  return NameValuePair< T, typename std::is_lvalue_reference<T>::type>( name, std::forward< T>( value));
	   }

	   /*

	   template< typename T>
	   NameValuePair< const T> makeNameValuePair( const char* name, const T& value)
	   {
		  return NameValuePair< const T>( name, value);
	   }

	   template< typename T>
	   NameRValuePair< const T> makeNameValuePair( const char* name, const T&& value)
	   {
		  return NameRValuePair< const T>( name, std::forward( value));
	   }
      */

    } // sf
} // casual


#define CASUAL_MAKE_NVP( member) \
   casual::sf::makeNameValuePair( #member, member)

//#define CASUAL_READ_WRITE( )



#endif
