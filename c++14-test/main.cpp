#include <iostream>
void generic_lambda()
{
   auto lambda = [](auto x, auto y) {return x + y;};
   std::cout << lambda( 1, 2) << std::endl;
}

[[deprecated]] void deprecated()
{

}

auto deduceReturnType()
{
    return 12;
}


int main()
{
   generic_lambda();
   deprecated();
   std::cout << deduceReturnType() << std::endl;
   return 0;
}

