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
array<string,2> process_key_value_string(string str){
    size_t position=str.find(" ");
     
    if(position==string::npos){
        return {str,""};
    }

    string key=str.substr(0,position);

    string value=str.substr(position+1);
    return {key,value};
}

//ss(string) --process--> key(string):value(int int)\n
tuple<string,array<uint64_t,2>> process_table_string(string& ss){
    // cout<<"inside process_table_string\n";
    size_t position = ss.find(":");
    
    if(position==string::npos){
        return {ss,{0,0}};
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
    // array<int,2>arr={stoi(value_1),stoi(value_2)};
     array<uint64_t,2> arr = {
        stoull(value_1),
        stoull(value_2)
    };

    return {key,arr};
}

//UTIL TO SAVE KEY VALUE PAIR IN A PERSISTENT TXT FILE 
array<uint64_t,2> saveFileUtil(string key,string value){
    fstream fileout("storage.bin",ios::app|ios::binary|ios::ate);
    // cout<<"H\n"
    fileout.seekp(0,ios::end);
    uint64_t x=fileout.tellp();
    // fileout<<key+":"+value+"\0";

    uint32_t k=key.size();
    uint32_t v=value.size();

    fileout.write((char*)&k,sizeof(k));
    fileout.write(key.data(),k);

    fileout.write((char*)&v,sizeof(v));
    fileout.write(value.data(),v);

    uint64_t y=fileout.tellp();

    return {x,y};
}

// UTIL TO SAVE THE TABLE OF OFFSET IN A PERSISTENT FILE SO THAT WE CAN
// FIND THE KEY VALUE PAIR STORED IN STORAGE.TXT USING THIS TABLE
void saveTable(string key,array<uint64_t,2>offset_range){
    fstream fileout("table.txt",ios::app);

    fileout<<key+":"+to_string(offset_range[0])+" "+to_string(offset_range[1])+"\n";
    
}


// THE SAVED AND PERSISTENT OFFSET TABLE IS LOADED INTO MEMORY FOR USE
void loadTable(unordered_map<string,array<uint64_t,2>>&table){
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
            tuple<string,array<uint64_t,2>>arr=process_table_string(ss);
            // cout<<"H\n";
            table[get<0>(arr)]={get<1>(arr)[0],get<1>(arr)[1]};
        }
    }
}


string get_value(string key,unordered_map<string,array<uint64_t,2>>&table){

    if(table.find(key)==table.end())return "Key doesnt Exist\n";
    string ss="storage.bin";

    //storage format is [key_length] [key_content] [value_length] [value_content]
    fstream ff(ss,ios::in|ios::binary);
    if(!ff) return "FILE OPEN FAILED\n";
    uint64_t start_offset=table[key][0];


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



//JUST A UTIL FOR PRINTING THE OFF TABLE (FOR TESTING PURPOSES)
void printTable(unordered_map<string,array<uint64_t,2>>&mp){
    if(!mp.size()){
        cout<<"No table yet\n";
        return;
    }

    for(auto it:mp){
        cout<<it.first<<"-->"<<it.second[0]<<"-"<<it.second[1]<<"\n";
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

    unordered_map<string,array<uint64_t,2>>table;
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

            array<uint64_t,2>position=saveFileUtil(key,value);
            table[key]={position[0],position[1]};


        }
        else if(command=="GET"){
            //IMPLEMENT GET LOGIC
            string stored_val=get_value(key,table);
            cout<<stored_val<<"\n";

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
