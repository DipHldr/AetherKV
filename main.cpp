#include<iostream>
#include<vector>
#include<unordered_map>
#include<array>
#include<sstream>
#include<fstream>
#include<cstdint>
using namespace std;

//structure of a table entry
class TableEntry{
    public:
    int key;
    int offset;    
    TableEntry(int key,int offset):key(key),offset(offset){}
};


//Table contains table entry and a bunch of other utilities
class Table{
    public:
    vector<TableEntry>table;

    void set(int key,int offset){
        TableEntry t(key,offset);
        table.push_back(t);
    }

    TableEntry get(int key){
        TableEntry t(-1,-1);
        for(const auto &it:table){
            if(it.key==key){
                t=it;
                break;
            }
        }
        return t;
    }
};

//function to process string and get the key value pair
//the entry is a whole string so we will need to parse it to get 
//the key and value pair
/* NEEDS A WHOLE LOT OF WORK ON THIS, ITS JUST A BASIC VERSION */
array<string,2> processed_string(string str){
    size_t position=str.find(":");
     
    if(position==string::npos){
        return {str,""};
    }

    string key=str.substr(0,position);

    string value=str.substr(position+1);
    return {key,value};
}

//ss(string) --process--> key(string):value(int int)\n
tuple<string,array<int,2>> process_table_string(string& ss){
    // cout<<"inside process_table_string\n";
    size_t position = ss.find(":");
    
    if(position==string::npos){
        return {ss,{-1,-1}};
    }
    
    string key=ss.substr(0,position);
    string value=ss.substr(position+1);
    
    size_t find_space=value.find(" ");
    // cout<<position+1<<"\n";
    // cout<<find_space<<"\n";
    string value_1=value.substr(0,find_space);
    // cout<<value_1<<"\n";
    // cout<<"H\n";
    string value_2=value.substr(find_space+1);
    array<int,2>arr={stoi(value_1),stoi(value_2)};

    return {key,arr};
}

//UTIL TO SAVE KEY VALUE PAIR IN A PERSISTENT TXT FILE 
array<int,2> saveFileUtil(string key,string value){
    fstream fileout("storage.bin",ios::app|ios::binary|ios::ate);
    // cout<<"H\n"
    fileout.seekp(0,ios::end);
    int x=fileout.tellp();
    // fileout<<key+":"+value+"\0";

    uint32_t k=key.size();
    uint32_t v=value.size();

    fileout.write((char*)&k,sizeof(k));
    fileout.write(key.data(),k);

    fileout.write((char*)&v,sizeof(v));
    fileout.write(value.data(),v);

    int y=fileout.tellp();

    return {x,y};
}

// UTIL TO SAVE THE TABLE OF OFFSET IN A PERSISTENT FILE SO THAT WE CAN
// FIND THE KEY VALUE PAIR STORED IN STORAGE.TXT USING THIS TABLE
void saveTable(string key,array<int,2>offset_range){
    fstream fileout("table.txt",ios::app);

    fileout<<key+":"+to_string(offset_range[0])+" "+to_string(offset_range[1])+"\n";
    
}


// THE SAVED AND PERSISTENT OFFSET TABLE IS LOADED INTO MEMORY FOR USE
void loadTable(unordered_map<string,array<int,2>>&table){
    fstream filein("table.txt",ios::in);

    if(!filein){
        cout<<"File doesn't exist\n";
    }else{
        string ss="";
        // char x;
        /*while(1){
            filein>>x;
            
            if(filein.eof()){
                break;
                }
                ss+=x;
                if(x==','){
                    ss.pop_back();
                    array<string,2>arr=processed_string(ss);
                    ss="";
                    table[arr[0]]=arr[1];
                    }
                    }*/
        
        cout<<"Inside load table\n";
        while(getline(filein,ss)){
            // cout<<ss<<"\n";
            tuple<string,array<int,2>>arr=process_table_string(ss);
            // cout<<"H\n";
            table[get<0>(arr)]={get<1>(arr)[0],get<1>(arr)[1]};
        }
    }
}


//JUST A UTIL FOR PRINTING THE OFF TABLE (FOR TESTING PURPOSES)
void printTable(unordered_map<string,array<int,2>>&mp){
    if(!mp.size()){
        cout<<"No table yet\n";
        return;
    }

    for(auto it:mp){
        cout<<it.first<<"-->"<<it.second[0]<<"-"<<it.second[1]<<"\n";
    }
}

int main(){
    cout<<"Welcome to key value Storage\n";
    // Table t; // this table is for storing the key offset to find data in a file

    unordered_map<string,array<int,2>>table;
    // cout<<"H\n";
    loadTable(table);
    printTable(table);

    while(true){
        string ss="";
        cout<<">";
        getline(cin,ss);//key value pair
        
        if(ss=="exit"){
            break;
        }

        array<string,2>arr=processed_string(ss);
        
        if(arr[1]==""){
            cout<<"cannot store null value\n";
            continue;
        }

        array<int,2>position=saveFileUtil(arr[0],arr[1]);
        // table[arr[0]]=to_string(arr[0].length()+arr[1].length()+1);
        table[arr[0]]={position[0],position[1]};

    }

    //deleting contents of table file so that i can write updated values
    fstream dfile("table.txt",ios::out|ios::trunc);

    if(dfile.is_open()){
        dfile.close();
    }else{
        cout<<"Error opening file\n";
    }

    cout<<"saving entries...\n";
    for(auto it:table){
        saveTable(it.first,it.second);
    }

    return 0;
}
