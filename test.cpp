#include<iostream>
#include<fstream>
using namespace std;
int main(){

    fstream ff("text.txt",ios::app|ios::ate);
    
    ff.seekp(0,ios::end);
    cout<<ff.tellp()<<"\n";
    ff<<"Hello again123456789\n";
    cout<<ff.tellp()<<"\n";

    // string ss="deep halder\n";
    // cout<<ss.data();
}
