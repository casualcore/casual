
#ifndef CASUAL_NAMEVALUEPAIR_H
#define CASUAL_NAMEVALUEPAIR_H 1


#include <utility>

#include <type_traits>

#include <map>

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
			  :  std::pair< const char*, T>( name, std::forward< T>( value)) {}

			 const char* getName () const
			 {
				 return this->first;
			 }

			 const T& getConstValue () const
			 {
				 return this->second;
			 }

			 T& getValue () const
         {
           return *( this->second);
         }
	   };

	   namespace internal
	   {
	      template< typename T>
	      struct nvp_traits
	      {
	         typedef NameValuePair< typename std::remove_reference< T>::type, typename std::is_lvalue_reference<T>::type> type;
	      };

	   }

	   template< typename T>
	   typename internal::nvp_traits< T>::type makeNameValuePair( const char* name, T&& value)
	   {
		  return typename internal::nvp_traits< T>::type( name, std::forward< T>( value));
	   }



    } // sf
} // casual


#define CASUAL_MAKE_NVP( member) \
   casual::sf::makeNameValuePair( #member, member)

//#define CASUAL_READ_WRITE( )



#endif
