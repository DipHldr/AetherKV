#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>

using namespace std;

int main() {
   float ff=3.143f;
   
   int *t=reinterpret_cast<int*>(&ff);
   int x=*t;

   cout<<ff<<"\n";
   cout<<x<<"\n";
   cout<<t<<"\n";

   return 0;
}
