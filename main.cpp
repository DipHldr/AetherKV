#include<iostream>
#include<vector>
#include<unordered_map>
#include<array>
#include<sstream>
#include<fstream>
#include<cstdint>
#include <filesystem>

using namespace std;


//***************
// NAMESPACES
//***************

//CONFIGS FOR KV STORE
namespace EngineConfig{
    constexpr uint32_t MAGIC = 0x4B565331; // KVS1
    constexpr uint32_t MAX_VALUE_SIZE = 10'000'000;
    constexpr const char* STORAGE_FILE = "storage.bin";
    constexpr const char* INDEX_FILE = "table.bin";
}


// ***********************
// Forward declarations
// ************************

// unordered_map<string,uint64_t> recoverIndex(string& StorageFileName);

//*********************
// STRUCT DEFINITIONS
//*********************

//new record format
// [crc][magic][record_type][key_len][value_len]-->header
//[header][data]-->New Format

#pragma pack(push,1)
struct Header{
uint32_t crc; //checksum
uint32_t magic;
uint16_t type;
uint32_t key_len;
uint32_t value_len;
};
#pragma pack(pop)

//enumerating type of the record
enum RecordType : uint16_t {
    RECORD_SET = 1,
    RECORD_DELETE = 2
};


//********************
// HELPER FUNCTIONS
//********************

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

//UTIL FOR CALCULATING CHECKSUM 
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

//*******************
// ENGINE FUNCTIONS
//*******************

//UTIL TO SAVE KEY VALUE PAIR IN A PERSISTENT TXT FILE 
uint64_t saveFileUtil(string key,string value, RecordType type=RECORD_SET){
    fstream fileout(EngineConfig::STORAGE_FILE,ios::app|ios::binary|ios::ate);
    
    if(!fileout){
        cout<<"Could not open storage file\n";
        return 0;
    }

    fileout.seekp(0,ios::end);
    
    //record offset for indexing
    uint64_t offset=fileout.tellp();

    Header head;
    head.magic=EngineConfig::MAGIC;
    head.type=type;
    head.key_len=static_cast<uint32_t>(key.size());
    head.value_len=static_cast<uint32_t>(value.size());

    //feeding CRC streaming
    uint32_t crc_state=0xFFFFFFFF; //initital state
    crc_state=checksum_CRC32(reinterpret_cast<const char*>(&head)+4,sizeof(Header)-4,crc_state);
    crc_state=checksum_CRC32(key.data(),key.size(),crc_state);
    crc_state=checksum_CRC32(value.data(),value.size(),crc_state);
    head.crc=~crc_state;

    if(!fileout.write(reinterpret_cast<const char*>(&head),sizeof(Header))){
        cout<<"Failed to write header to storage file\n";
        return 0;
    }
    if(!fileout.write(key.data(),head.key_len)){
        cout<<"failed to write key data to storage file\n";
        return 0;
    }
    if(!fileout.write(value.data(),head.value_len)){
        cout<<"Failed to write value data to storage file\n";
        return 0;
    }
    fileout.flush();

      if (fileout.fail()) {
        cout << "Error occurred while writing to file\n";
        return 0;
    }

    return offset;

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
    fileout.flush();
}
//function for recovering Index table
 unordered_map<string,uint64_t> recoverIndex(){
    string tableFile="table.bin";
    unordered_map<string,uint64_t>table;
    fstream ff(EngineConfig::STORAGE_FILE,ios::binary|ios::in);
    if(!ff){
        cout<<"File not found\n";
        cout<<"No Index To Recover\n";
        return table;
    }
    ff.seekg(0,ios::end);
    uint64_t file_size=ff.tellg();
    ff.seekg(0,ios::beg);

    while(static_cast<uint64_t>(ff.tellg())<file_size){

        uint64_t current_offset=ff.tellg();

        Header head;
        if(!ff.read(reinterpret_cast<char*>(&head),sizeof(Header)))break;

        if(head.magic!=EngineConfig::MAGIC){
            ff.seekg(current_offset+1);
            continue;
        }

        string key(head.key_len,'\0');
        if(!ff.read(&key[0],head.key_len)){
            ff.seekg(current_offset+1);
            continue;
        }

        ff.seekg(head.value_len,ios::cur);

        if(head.type==RECORD_SET){
            table[key]=current_offset;
        }else if(head.type==RECORD_DELETE){
            table.erase(key);
        }
        saveTable(key,current_offset,tableFile);
    }
    cout<<"Recovered Index\n";
    // printTable(table);
    return table;
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
    fstream ff(EngineConfig::INDEX_FILE,ios::binary|ios::in);
    if(!ff.is_open()){
        cout<<"No Indexing Yet\n";
        //here we can call a recovery function too
        table=recoverIndex();
        persistTable(table,EngineConfig::INDEX_FILE);
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
    fstream ff(EngineConfig::STORAGE_FILE,ios::in|ios::binary);
    if(!ff) return "FILE OPEN FAILED\n";
    uint64_t start_offset=table[key];


    //pointing offset to proper place for reading
    ff.seekg(start_offset);
    Header head;
    ff.read(reinterpret_cast<char*>(&head),sizeof(Header));

    string key_data(head.key_len,'\0');
    ff.read(&key_data[0],head.key_len);

    string value_data(head.value_len,'\0');
    ff.read(&value_data[0],head.value_len);

    uint32_t crc_check = 0xFFFFFFFF;
    crc_check=checksum_CRC32(reinterpret_cast<const char*>(&head)+4,sizeof(Header)-4,crc_check);
    crc_check = checksum_CRC32(key_data.data(),key_data.size(),crc_check);
    crc_check = checksum_CRC32(value_data.data(),value_data.size(),crc_check);
    crc_check=~crc_check;

    if(crc_check!=head.crc){
        return "ERROR: Data corruption detected (CRC mismatch)\n";
    }


    return (head.type==RECORD_DELETE)?"KEY DELETED":value_data;

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
