#include<iostream>
#include<vector>
#include<unordered_map>
#include<array>
#include<sstream>
#include<fstream>
#include<cstdint>
#include <filesystem>

using namespace std;

//Index table structure--> key:record offset

//function to process string and get the key value pair
//the entry is a whole string so we will need to parse it to get 
//the key and value pair
/* NEEDS A WHOLE LOT OF WORK ON THIS, ITS JUST A BASIC VERSION */
array<string,2> process_key_value_string(string str){
    size_t position=str.find(" ");
     
    if(position==string::npos){
        return {str,""};
    }

    string key=str.substr(0,position);

    string value=str.substr(position+1);
    return {key,value};
}

//ss(string) --process--> key(string):uint64_t\n
tuple<string,uint64_t> process_table_string(string& ss){
    // cout<<"inside process_table_string\n";
    size_t position = ss.find(":");
    
    if(position==string::npos){
        return {ss,0};
    }
    
    string key=ss.substr(0,position);
    string value=ss.substr(position+1);
    
    

    return {key,stoull(value)};
}

//UTIL TO SAVE KEY VALUE PAIR IN A PERSISTENT TXT FILE 
uint64_t saveFileUtil(string key,string value){
    fstream fileout("storage.bin",ios::app|ios::binary|ios::ate);
    
    fileout.seekp(0,ios::end);
    
    //record offset for indexing
    uint64_t x=fileout.tellp();
    
    uint32_t k=key.size();
    uint32_t v=value.size();
    
    fileout.write((char*)&k,sizeof(k));
    fileout.write(key.data(),k);

    fileout.write((char*)&v,sizeof(v));
    fileout.write(value.data(),v);

    return x;
}

// UTIL TO SAVE THE TABLE OF OFFSET IN A PERSISTENT FILE SO THAT WE CAN
// FIND THE KEY VALUE PAIR STORED IN STORAGE.TXT USING THIS TABLE
void saveTable(string key,uint64_t offset_range){
    fstream fileout("table.bin",ios::app|ios::binary|ios::ate);

    //key_length key value(we know its 8 bytes because its stored in uint64_t)
    uint32_t k_len=key.size();
    fileout.write((char*)&k_len,sizeof(k_len));
    fileout.write(key.data(),k_len);
    fileout.write(reinterpret_cast<const char*>(&offset_range),sizeof(offset_range));
    fileout.close();
}


// THE SAVED AND PERSISTENT OFFSET TABLE IS LOADED INTO MEMORY FOR USE
void loadTable(const unordered_map<string,uint64_t>&table,const string& indexFile){
    string tempFile=indexFile+".tmp";
    fstream fileout(tempFile,ios::binary);

    if(!fileout){
        cout<<"File doesn't exist\n";
    }
    for(const auto&[key,offset]:table){
        uint32_t k_len=key.size();

        fileout.write((char*)&k_len,sizeof(k_len));
        fileout.write(key.data(),k_len);
        fileout.write(reinterpret_cast<const char*>(&offset),sizeof(offset));
    }

    fileout.close();

    filesystem::rename(tempFile,indexFile);   

}

//needs to get fixed due to change in table
string get_value(string key,unordered_map<string,uint64_t>&table){

    if(table.find(key)==table.end())return "Key doesnt Exist\n";
    string ss="storage.bin";

    //storage format is [key_length] [key_content] [value_length] [value_content]
    fstream ff(ss,ios::in|ios::binary);
    if(!ff) return "FILE OPEN FAILED\n";
    uint64_t start_offset=table[key];


    //pointing offset to proper place for reading
    ff.seekg(start_offset);

    //we used uint32_t previously to store the key value
    uint32_t k_len=0,v_len=0;
    
    //reading from the curent off set for size of k_len bytes
    //and pooring it into k_len
    ff.read((char*)&k_len,sizeof(k_len));
    if(!ff) return "READ FAIL KLEN\n";

    //moving the read pointer by k_len (cuz we already know the key)
    ff.seekg(k_len,ios::cur);

    //not reading the value length (same mechanism as we did in key length above)
    ff.read((char*)&v_len,sizeof(v_len));
    if(!ff) return "READ FAIL VLEN\n";

    if(v_len > 10'000'000)  // sanity guard
        return "CORRUPTED LENGTH\n";

    //creating a string of v_len characters pre-filled with spaces.
    string value(v_len,'\0');

    //in c++ string data is stored contiguously, thats why providing the address 
    //of the first character allows .read() to fill the entire string's memory block 
    //directly from the file.
    ff.read(&value[0],v_len);

    return value;
}

//function for recovering Index table
/*void recoverIndex(unordered_map<string,uint64_t>&table,string& filename){
    fstream ff(filename,ios::binary);
    if(!ff){
        cout<<"File not found\n";
        cout<<"No Index To Recover\n";
        return;
    }
    uint64_t len;
    while(!ff.eof()){
        //getting key length
        ff.read((char*)&len,sizeof(len));
        //pointing readpointer
        ff.seekg(sizeof(len));
        ff.


    }
}
    */








//JUST A UTIL FOR PRINTING THE OFF TABLE (FOR TESTING PURPOSES)
void printTable(unordered_map<string,uint64_t>&mp){
    if(!mp.size()){
        cout<<"No table yet\n";
        return;
    }

    for(auto it:mp){
        cout<<it.first<<"-->"<<it.second<<"\n";
    }
}

array<string,2> parseCommand(const string& ss){
    int pos=ss.find(" ");
    string command=ss.substr(0,pos);
    string value=ss.substr(pos+1);
    return {command,value};
}

int main(){
    cout<<"Welcome to key value Storage\n";
    // Table t; // this table is for storing the key offset to find data in a file

    unordered_map<string,uint64_t>table;
    // cout<<"H\n";
    loadTable(table);
    printTable(table);

    while(true){
        string input="";
        cout<<">";
        //TAKING INPUT
        getline(cin,input);
        stringstream ss(input);
        
        //EXTRACT THE COMMAND KEY and VALUE (<COMMAND> <KEY> <VALUE>)
        string command,key,value;
        ss>>command>>key;

        string remaining;
        getline(ss,remaining);
        if(!remaining.empty()&&remaining[0]==' ')remaining.erase(0,1);
        if(remaining=="<<"){
            cout<<"Welcome to multiline mode\n";
            value="";
            string line;
            while(getline(cin,line)&&line!="END"){
                value+=line+"\n";
            }
        }
        else{
            value=remaining;
        }       

        /*
        Commands:
        SET key:value
        GET key -->gets a key
        DEL key --> deletes a key
        UPDATE key --> updates a key
        EXISTS key -->checks if a key exists
        KEYS --> lists all keys
        COMPACT -->runs compaction algo
        STATS -->shows database stats
        FLUSHALL -->deletes the database
        */
        
        if(command=="SET"){
          //IMPLEMENT SET LOGIC

            if(value==""){
                cout<<"cannot store null value\n";
                continue;
            }

            uint64_t position=saveFileUtil(key,value);
            table[key]=position;


        }
        else if(command=="GET"){
            //IMPLEMENT GET LOGIC
            string stored_val=get_value(key,table);
            cout<<stored_val<<"\n";

        }
        else if(command=="UPDATE"){
            //IMPLEMENT UPDATE LOGIC
        }
        else if(command=="DEL"){
            //IMPLEMENT DEL LOGIC
            table.erase(key);//DEMO LOGIC
            
        }
        else if(command=="EXISTS"){
            //IMPLEMENT EXISTS LOGIC
            
        }
        else if(command=="KEYS"){
            //IMPLEMENT KEYS LOGIC

        }
        else if(command=="COMPACT"){
            //IMPLEMENT COMPACT LOGIC
                        
        }
        else if(command=="STATS"){
            //IMPLEMENT STATS LOGIC
            
        }
        else if(command=="FLUSHALL"){
            //IMPLEMENT FLUSHALL LOGIC
            
        }
        else if(command=="EXIT"||command=="exit"){
            break;
        }

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
