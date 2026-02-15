#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>

using namespace std;

int main() {
    const string filename = "test_storage.bin";

    string key = "demoKey";
    string value = "This is a multiline-ish value\nLine2\nLine3";
    uint32_t k_len = key.size();
    uint32_t v_len = value.size();

    fstream out(filename, ios::binary | ios::out | ios::app);

    if (!out) {
        cout << "Failed to open file for writing\n";
        return 1;
    }

    uint64_t start = out.tellp();
    out.write((char*)&k_len, sizeof(k_len));
    out.write(key.data(), k_len);

    out.write((char*)&v_len, sizeof(v_len));
    out.write(value.data(), v_len);

    uint64_t end = out.tellp();
    out.close();
    cout << "Written record\n";
    cout << "Start offset: " << start << "\n";
    cout << "End offset  : " << end << "\n\n";


    fstream in(filename, ios::binary | ios::in);

    if (!in) {
        cout << "Failed to open file for reading\n";
        return 1;
    }
    in.seekg(start);

    uint32_t rk_len, rv_len;

    in.read((char*)&rk_len, sizeof(rk_len));

    string rkey(rk_len, ' ');
    in.read(&rkey[0], rk_len);
    in.read((char*)&rv_len, sizeof(rv_len));
    string rvalue(rv_len, ' ');
    in.read(&rvalue[0], rv_len);
    in.close();
    cout << "Read back:\n";
    cout << "Key   : " << rkey << "\n";
    cout << "Value : \n" << rvalue << "\n";

    return 0;
}
