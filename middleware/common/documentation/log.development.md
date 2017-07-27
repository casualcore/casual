# log development

## create new categories


```c++

// header file

namespace log
{
   extern casual::log::Category gateway;

   namespace verbose
   {
      extern casual::log::Category gateway;
   }
}

// TU

namespace log
{
   casual::log::Category gateway{ "casual.gateway"};

   namespace verbose
   {
      casual::log::Category gateway{ "casual.gateway.verbose"};
   }
}

```

Then just start logging to that ostream.

