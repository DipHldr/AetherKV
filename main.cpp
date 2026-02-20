#include<iostream>
#include<vector>
#include<unordered_map>
#include<array>
#include<sstream>
#include<fstream>
#include<cstdint>
#include <filesystem>

using namespace std;
// ***********************
// Forward declarations
// ************************

unordered_map<string,uint64_t> recoverIndex(string& StorageFileName);

//new record format
// [crc][magic][record_type][key_len][value_len]-->header
//[header][data]-->New Format

#pragma pack(push,1);
struct Header{
uint32_t crc; //checksum
uint32_t magic;
uint16_t type;
uint32_t key_len;
uint32_t value_len;
};
#pragma pack(pop)

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

//CALCULATING CHECKSUM 
//This is for streaming data
//This is the "Engine". It never uses ~
uint32_t checksum_CRC32(const char* data, size_t length, uint32_t crc) {
    const uint8_t* byte_data = reinterpret_cast<const uint8_t*>(data);
    for (size_t i = 0; i < length; i++) {
        crc ^= byte_data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }

    //need to do ~crc operation after streaming data ends
    return crc; 
}


//UTIL TO SAVE KEY VALUE PAIR IN A PERSISTENT TXT FILE 
uint64_t saveFileUtil(string key,string value){
    fstream fileout("storage.bin",ios::app|ios::binary|ios::ate);
    
    fileout.seekp(0,ios::end);
    
    //record offset for indexing
    uint64_t x=fileout.tellp();
    
    uint32_t k=key.size();
    uint32_t v=value.size();
    
    fileout.write(reinterpret_cast<const char*>(&k),sizeof(k));
    fileout.write(key.data(),k);

    fileout.write(reinterpret_cast<const char*>(&v),sizeof(v));
    fileout.write(value.data(),v);

    return x;
}

// UTIL TO SAVE THE TABLE OF OFFSET IN A PERSISTENT FILE SO THAT WE CAN
// FIND THE KEY VALUE PAIR STORED IN STORAGE.TXT USING THIS TABLE
void saveTable(string key,uint64_t offset_range,const string& indexFile){
    fstream fileout(indexFile,ios::app|ios::binary|ios::ate);

    //key_length key value(we know its 8 bytes because its stored in uint64_t)
    uint32_t k_len=key.size();
    fileout.write(reinterpret_cast<const char*>(&k_len),sizeof(k_len));
    fileout.write(key.data(),k_len);
    fileout.write(reinterpret_cast<const char*>(&offset_range),sizeof(offset_range));
    fileout.close();
}


// THE SAVED AND PERSISTENT OFFSET TABLE INTO A BINARY FILE
//THIS ONE IS FOR USE DURING TAKING SNAPSHOT IF TABLE GETS TOO BIG
void persistTable(const unordered_map<string,uint64_t>&table,const string& indexFile){
    string tempFile=indexFile+".tmp";
    fstream fileout(tempFile,ios::binary|ios::app);


    if(!fileout.is_open()){
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

unordered_map<string,uint64_t> loadIndexTableFromFile(const string& filename){
    unordered_map<string,uint64_t> table;
    fstream ff(filename,ios::binary|ios::in);
    if(!ff.is_open()){
        cout<<"No Indexing Yet\n";
        //here we can call a recovery function too
        string originalFileName="storage.bin";
        table=recoverIndex(originalFileName);
        return table;
    }

    while(true){
        uint32_t k_len;
        ff.read(reinterpret_cast<char*>(&k_len),sizeof(k_len));
        if(ff.eof())break;
         if(!ff) {
            cout << "Corrupted index file (key length)\n";
            break;
        }
        string key(k_len,'\0');
        ff.read(key.data(),k_len);
        if(!ff) {
            cout << "Corrupted index file (key data)\n";
            break;
        }

        uint64_t offset;
        ff.read(reinterpret_cast<char*>(&offset),sizeof(offset));

        if(!ff) {
            cout << "Corrupted index file (offset)\n";
            break;
        }

        table[key]=offset;
    }

    ff.close();
    return table;
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
 unordered_map<string,uint64_t> recoverIndex(string& StorageFileName){
    string tableFile="table.bin";
    unordered_map<string,uint64_t>table;
    fstream ff(StorageFileName,ios::binary|ios::in);
    if(!ff){
        cout<<"File not found\n";
        cout<<"No Index To Recover\n";
        return table;
    }

    while(true){
        uint64_t offset=static_cast<uint64_t>(ff.tellg());

        uint32_t k_len;
        //getting key length
       if(!ff.read(reinterpret_cast<char*>(&k_len),sizeof(k_len)))break;

        string key(k_len,'\0');

        if(!ff.read(key.data(),k_len))break;

        uint32_t v_len;

        if(!ff.read(reinterpret_cast<char*>(&v_len),sizeof(v_len)))break;

        ff.seekg(v_len,ios::cur);

        table[key]=offset;
        saveTable(key,offset,tableFile);
    }
    cout<<"Recovered Index\n";
    printTable(table);
    return table;
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
    
    string indexFile="table.bin";
    table=loadIndexTableFromFile(indexFile);
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

            uint64_t offset=saveFileUtil(key,value);
            table[key]=offset;
            saveTable(key,offset,indexFile);

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


    return 0;
}
