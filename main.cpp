#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <set>

using namespace std;

const int MAX_KEY_SIZE = 65;
const int ORDER = 100; // B+ Tree order

struct KeyValue {
    char key[MAX_KEY_SIZE];
    int value;

    KeyValue() {
        memset(key, 0, sizeof(key));
        value = 0;
    }

    KeyValue(const char* k, int v) {
        strncpy(key, k, MAX_KEY_SIZE - 1);
        key[MAX_KEY_SIZE - 1] = '\0';
        value = v;
    }

    bool operator<(const KeyValue& other) const {
        int cmp = strcmp(key, other.key);
        if (cmp != 0) return cmp < 0;
        return value < other.value;
    }

    bool operator==(const KeyValue& other) const {
        return strcmp(key, other.key) == 0 && value == other.value;
    }
};

struct Node {
    bool isLeaf;
    int numKeys;
    KeyValue keys[ORDER];
    int children[ORDER + 1]; // File positions for child nodes
    int next; // For leaf nodes, link to next leaf

    Node() : isLeaf(true), numKeys(0), next(-1) {
        for (int i = 0; i <= ORDER; i++) {
            children[i] = -1;
        }
    }
};

class BPlusTree {
private:
    string filename;
    int rootPos;
    int freePos;
    fstream file;

    void writeNode(int pos, const Node& node) {
        file.seekp(pos);
        file.write((char*)&node, sizeof(Node));
        file.flush();
    }

    Node readNode(int pos) {
        Node node;
        file.seekg(pos);
        file.read((char*)&node, sizeof(Node));
        return node;
    }

    int allocateNode() {
        int pos = freePos;
        freePos += sizeof(Node);
        return pos;
    }

    void writeHeader() {
        file.seekp(0);
        file.write((char*)&rootPos, sizeof(int));
        file.write((char*)&freePos, sizeof(int));
        file.flush();
    }

    void readHeader() {
        file.seekg(0);
        file.read((char*)&rootPos, sizeof(int));
        file.read((char*)&freePos, sizeof(int));
    }

    void splitChild(int parentPos, int childIndex, Node& parent) {
        Node child = readNode(parent.children[childIndex]);
        Node newNode;
        newNode.isLeaf = child.isLeaf;

        int mid = ORDER / 2;
        newNode.numKeys = child.numKeys - mid;

        for (int i = 0; i < newNode.numKeys; i++) {
            newNode.keys[i] = child.keys[mid + i];
        }

        if (!child.isLeaf) {
            for (int i = 0; i <= newNode.numKeys; i++) {
                newNode.children[i] = child.children[mid + i];
            }
        } else {
            newNode.next = child.next;
            child.next = allocateNode();
        }

        child.numKeys = mid;

        int newNodePos = child.isLeaf ? child.next : allocateNode();
        writeNode(newNodePos, newNode);
        writeNode(parent.children[childIndex], child);

        for (int i = parent.numKeys; i > childIndex; i--) {
            parent.keys[i] = parent.keys[i - 1];
            parent.children[i + 1] = parent.children[i];
        }

        parent.keys[childIndex] = newNode.keys[0];
        parent.children[childIndex + 1] = newNodePos;
        parent.numKeys++;
    }

    void insertNonFull(int nodePos, const KeyValue& kv) {
        Node node = readNode(nodePos);

        if (node.isLeaf) {
            int i = node.numKeys - 1;
            while (i >= 0 && kv < node.keys[i]) {
                node.keys[i + 1] = node.keys[i];
                i--;
            }
            node.keys[i + 1] = kv;
            node.numKeys++;
            writeNode(nodePos, node);
        } else {
            int i = node.numKeys - 1;
            while (i >= 0 && kv < node.keys[i]) {
                i--;
            }
            i++;

            Node child = readNode(node.children[i]);
            if (child.numKeys == ORDER) {
                splitChild(nodePos, i, node);
                node = readNode(nodePos);
                if (node.keys[i] < kv) {
                    i++;
                }
            }
            insertNonFull(node.children[i], kv);
        }
    }

    void deleteFromNode(int nodePos, const KeyValue& kv) {
        Node node = readNode(nodePos);

        if (node.isLeaf) {
            int i = 0;
            while (i < node.numKeys && node.keys[i] < kv) {
                i++;
            }

            if (i < node.numKeys && node.keys[i] == kv) {
                for (int j = i; j < node.numKeys - 1; j++) {
                    node.keys[j] = node.keys[j + 1];
                }
                node.numKeys--;
                writeNode(nodePos, node);
            }
        } else {
            int i = 0;
            while (i < node.numKeys && node.keys[i] < kv) {
                i++;
            }
            deleteFromNode(node.children[i], kv);
        }
    }

public:
    BPlusTree(const string& fname) : filename(fname) {
        file.open(filename, ios::in | ios::out | ios::binary);

        if (!file.is_open()) {
            file.open(filename, ios::out | ios::binary);
            file.close();
            file.open(filename, ios::in | ios::out | ios::binary);

            rootPos = 2 * sizeof(int);
            freePos = rootPos + sizeof(Node);

            Node root;
            writeNode(rootPos, root);
            writeHeader();
        } else {
            readHeader();
        }
    }

    ~BPlusTree() {
        if (file.is_open()) {
            file.close();
        }
    }

    void insert(const char* key, int value) {
        KeyValue kv(key, value);

        Node root = readNode(rootPos);
        if (root.numKeys == ORDER) {
            Node newRoot;
            newRoot.isLeaf = false;
            newRoot.numKeys = 0;
            newRoot.children[0] = rootPos;

            int newRootPos = allocateNode();
            writeNode(newRootPos, newRoot);

            splitChild(newRootPos, 0, newRoot);
            insertNonFull(newRootPos, kv);

            rootPos = newRootPos;
            writeHeader();
        } else {
            insertNonFull(rootPos, kv);
        }
    }

    void remove(const char* key, int value) {
        KeyValue kv(key, value);
        deleteFromNode(rootPos, kv);
    }

    vector<int> find(const char* key) {
        vector<int> result;
        int nodePos = rootPos;

        while (true) {
            Node node = readNode(nodePos);

            if (node.isLeaf) {
                for (int i = 0; i < node.numKeys; i++) {
                    if (strcmp(node.keys[i].key, key) == 0) {
                        result.push_back(node.keys[i].value);
                    }
                }

                if (node.next != -1) {
                    nodePos = node.next;
                } else {
                    break;
                }
            } else {
                int i = 0;
                while (i < node.numKeys && strcmp(key, node.keys[i].key) >= 0) {
                    i++;
                }
                nodePos = node.children[i];
            }
        }

        sort(result.begin(), result.end());
        return result;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    BPlusTree tree("data.bpt");

    int n;
    cin >> n;

    string cmd;
    char index[MAX_KEY_SIZE];
    int value;

    for (int i = 0; i < n; i++) {
        cin >> cmd;

        if (cmd == "insert") {
            cin >> index >> value;
            tree.insert(index, value);
        } else if (cmd == "delete") {
            cin >> index >> value;
            tree.remove(index, value);
        } else if (cmd == "find") {
            cin >> index;
            vector<int> result = tree.find(index);

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
