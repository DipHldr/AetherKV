#include<iostream>
#include<fstream>
#include<cstdint>
using namespace std;
int main(){

    fstream ff("test.txt",ios::app|ios::ate);
    
    string key="demodemo";
    uint32_t k=key.size();
    string value="voila";
    uint32_t v=value.size();

    ff.seekp(0,ios::end);
    cout<<ff.tellp()<<"\n";
    ff.write((char*)&k,sizeof(k));
    ff.write(key.data(),k);

    ff.write((char*)&v,sizeof(v));
    ff.write(value.data(),v);
    cout<<ff.tellp()<<"\n";

    // string ss="deep halder\n";
    // cout<<ss.data();
}
