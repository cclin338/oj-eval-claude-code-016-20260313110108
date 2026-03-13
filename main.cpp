#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <map>
#include <string>
#include <climits>

using namespace std;

class KeyValueStore {
private:
    map<pair<string, int>, bool> data;
    string filename;

    void loadFromFile() {
        ifstream file(filename, ios::binary);
        if (!file.is_open()) return;

        int count;
        if (file.read(reinterpret_cast<char*>(&count), sizeof(count))) {
            for (int i = 0; i < count; i++) {
                int keyLen;
                if (!file.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen))) break;

                string key;
                key.resize(keyLen);
                if (!file.read(&key[0], keyLen)) break;

                int value;
                if (!file.read(reinterpret_cast<char*>(&value), sizeof(value))) break;

                data.insert({{key, value}, true});
            }
        }
        file.close();
    }

    void saveToFile() {
        ofstream file(filename, ios::binary | ios::trunc);
        if (!file.is_open()) return;

        int count = data.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& entry : data) {
            const string& key = entry.first.first;
            int value = entry.first.second;

            int keyLen = key.length();
            file.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
            file.write(key.c_str(), keyLen);
            file.write(reinterpret_cast<const char*>(&value), sizeof(value));
        }

        file.close();
    }

public:
    KeyValueStore(const string& fname) : filename(fname) {
        loadFromFile();
    }

    ~KeyValueStore() {
        saveToFile();
    }

    void insert(const string& key, int value) {
        // Check if already exists
        auto range = data.equal_range({key, value});
        if (range.first == range.second) {
            data.insert({{key, value}, true});
        }
    }

    void remove(const string& key, int value) {
        auto range = data.equal_range({key, value});
        if (range.first != range.second) {
            data.erase(range.first);
        }
    }

    vector<int> find(const string& key) {
        vector<int> result;

        // Use lower_bound to jump directly to the first occurrence
        auto it = data.lower_bound({key, INT_MIN});

        while (it != data.end() && it->first.first == key) {
            result.push_back(it->first.second);
            ++it;
        }

        // Already sorted by map's ordering
        return result;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    KeyValueStore store("data.bpt");

    int n;
    cin >> n;

    string cmd, index;
    int value;

    for (int i = 0; i < n; i++) {
        cin >> cmd;

        if (cmd == "insert") {
            cin >> index >> value;
            store.insert(index, value);
        } else if (cmd == "delete") {
            cin >> index >> value;
            store.remove(index, value);
        } else if (cmd == "find") {
            cin >> index;
            vector<int> result = store.find(index);

            if (result.empty()) {
                cout << "null\n";
            } else {
                for (size_t j = 0; j < result.size(); j++) {
                    if (j > 0) cout << " ";
                    cout << result[j];
                }
                cout << "\n";
            }
        }
    }

    return 0;
}
