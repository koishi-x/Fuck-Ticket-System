#ifndef FUCK_TICKET_SYSTEM_BPLUSTREE_H
#define FUCK_TICKET_SYSTEM_BPLUSTREE_H

#include<iostream>
#include<cstdio>
#include<cstring>
#include<fstream>

using std::pair;


class string_t {
public:
    char s[32];

    string_t(){memset(s, 0, sizeof s);}

    string_t(const string_t &obj) {
        strcpy(s, obj.s);
    }
    bool operator<(const string_t &obj) const {
        return strcmp(s, obj.s) < 0;
    }
    bool operator!=(const string_t &obj) const {
        return strcmp(s, obj.s) != 0;
    }
    bool operator==(const string_t &obj) const {
        return strcmp(s, obj.s) == 0;
    }
    string_t &operator=(const string_t &obj) {
        if(this == &obj) return *this;
        strcpy(s, obj.s);
        return *this;
    }
    bool empty() {
        return strlen(s) == 0;
    }

    void assign(std::string t) {
        strcpy(s, t.c_str());
    }
    friend std::istream& operator>>(std::istream &is, string_t &obj);
    friend std::ostream& operator<<(std::ostream &os, const string_t &obj);
};

std::ostream& operator<<(std::ostream &os, const string_t &obj) {
    os << obj.s;
    return os;
}

std::istream& operator>>(std::istream &is, string_t &obj) {
    is >> obj.s;
    return is;
}

template<class Key, class T>
class BPT {
    static constexpr int BLOCK_SIZE = 16384;
    static constexpr int M = (BLOCK_SIZE - 9) / (4 + sizeof(Key));
    static constexpr int L = (BLOCK_SIZE - 13) / (sizeof(Key) + sizeof(T)) - 1;
    char filename[20];
    int siz;
    int root;
    int last;
    std::fstream io;

    class ZHANWEIFU {
        bool awa[BLOCK_SIZE];
    };
    struct dataInfo {
        Key key;
        T value;
        dataInfo() {};
        dataInfo(Key K, T V): key(K), value(V) {};
        bool operator<(const dataInfo &obj) const {
            if(key != obj.key) return key < obj.key;
            return value < obj.value;
        }
        bool operator==(const dataInfo &obj) const {
            return key == obj.key && value == obj.value;
        }
        dataInfo &operator=(const dataInfo &obj) {
            if (this == &obj) return *this;
            key = obj.key;
            value = obj.value;
            return *this;
        }
    };
    struct bodyNode {
        //restriction: n<=M-1
        int n;
        Key key[M];
        int addr[M+1];
        bodyNode() : n(0) {
            memset(addr, -1, sizeof addr);
            memset(key, 0, sizeof key);
        }
        bodyNode &operator=(const bodyNode &obj) {
            if (this == &obj) return *this;
            n = obj.n;
            for (int i = 0; i < n; ++i) key[i] = obj.key[i];
            for (int i = 0; i <= n; ++i) addr[i] = obj.addr[i];
            return *this;
        }
    };
    struct leafNode {
        int n;
        dataInfo data[L+1];
        int preAddr, nextAddr;
        leafNode() : n(0), preAddr(-1), nextAddr(-1) {memset(data, 0, sizeof data);}
        leafNode &operator=(const leafNode &obj) {
            if (this == &obj) return *this;
            n = obj.n;
            for (int i = 0; i < n; ++i) data[i] = obj.data[i];
            preAddr = obj.preAddr;
            nextAddr = obj.nextAddr;
            return *this;
        }
    };

    int newPos() {
        //todo:recycle space
        last += BLOCK_SIZE;
        write(ZHANWEIFU(), last);
        return last;
    }

    void recycle(int &addr) {
        //todo:recycle space
        addr = -1;
    }
    void clearBin() {
        last = 0;
    }

    template<typename readType>
    void read(readType &tmp, int addr = -1) {
        if(addr != -1) io.seekg(addr);
        io.read(reinterpret_cast<char*>(&tmp), sizeof(readType));
    }

    template<typename writeType>
    void write(const writeType &tmp, int addr = -1) {
        if(addr != -1) io.seekp(addr);
        io.write(reinterpret_cast<const char*>(&tmp), sizeof(writeType));
    }

    pair<bool, pair<Key, int> > Insert(int &addr, const dataInfo &curData) {
        bool nodeType;
        read(nodeType, addr);
        if(!nodeType) { //body node
            bodyNode tmp;
            read(tmp);

            int pos = 0;
            for(int i = tmp.n - 1; i >= 0; --i) {
                if(!(curData.key < tmp.key[i])) {
                    pos = i + 1;
                    break;
                }
            }
            pair<bool, pair<Key, int> > result = Insert(tmp.addr[pos], curData);
            if (result.first == false) {
                return make_pair(false, pair<Key, int>());
            } else {
                for (int i = tmp.n; i > pos; --i) {
                    tmp.key[i] = tmp.key[i - 1];
                    tmp.addr[i + 1] = tmp.addr[i];
                }
                tmp.key[pos] = result.second.first;
                tmp.addr[pos + 1] = result.second.second;
                tmp.n++;

                if (tmp.n < M) {
                    write(tmp, addr + sizeof(bool));
                    return make_pair(false, pair<Key, int>());
                } else {    //split the body node.
                    int addr2 = newPos();
                    bodyNode tmp2;
                    tmp2.n = tmp.n / 2;
                    tmp.n = tmp.n - tmp2.n - 1;

                    for (int i = 0; i < tmp2.n; ++i) {
                        tmp2.key[i] = tmp.key[i+tmp.n+1];
                        tmp2.addr[i+1] = tmp.addr[i+tmp.n+2];
                    }
                    tmp2.addr[0] = tmp.addr[tmp.n+1];

                    write(tmp, addr + sizeof(bool));
                    write(false, addr2);
                    write(tmp2);

                    return make_pair(true, pair<Key, int>(tmp.key[tmp.n], addr2));
                }
            }
        } else {    //leaf node
            leafNode tmp;
            read(tmp);
            int pos = 0;
            for(int i = tmp.n - 1; i >= 0; --i) {
                if(!(curData < tmp.data[i])) {
                    pos = i + 1;
                    break;
                }
            }
            for (int i = tmp.n; i > pos; --i) {
                tmp.data[i] = tmp.data[i-1];
            }
            tmp.data[pos] = curData;
            tmp.n++;
            if (tmp.n <= L) {
                write(tmp, addr + sizeof(bool));
                return {false, std::pair<Key, int>()};
            } else {    //split the leaf node.
                int addr2 = newPos();
                leafNode tmp2;
                // tmp.n = (L + 1) / 2, tmp2.n = L + 1 - tmp.n;
                tmp2.n = tmp.n / 2;
                tmp.n = tmp.n - tmp2.n;
                for (int i = 0; i < tmp2.n; ++i) {
                    tmp2.data[i] = tmp.data[i+tmp.n];
                }

                if (tmp.nextAddr != -1) {
                    leafNode nextNode;
                    read(nextNode, tmp.nextAddr + sizeof(bool));
                    nextNode.preAddr = addr2;
                    write(nextNode, tmp.nextAddr + sizeof(bool));
                }
                tmp2.nextAddr = tmp.nextAddr;
                tmp.nextAddr = addr2;
                tmp2.preAddr = addr;

                write(tmp, addr + sizeof(bool));
                write(true, addr2);
                write(tmp2);
                return make_pair(true, std::pair<Key, int>(tmp2.data[0].key, addr2));
            }
        }
    }

    //first value is true if delete successfully.
    //second value is true, if the son node size < L/2 and need to be merged
    pair<bool, bool> Erase(int addr, const Key &curKey) {
        bool nodeType;
        read(nodeType, addr);
        if (!nodeType) {    //body node
            bodyNode tmp;
            read(tmp);
            int pos = 0;
            for(int i = tmp.n - 1; i >= 0; --i) {
                if(!(curKey < tmp.key[i])) {
                    pos = i + 1;
                    break;
                }
            }

            pair<bool, bool> result = Erase(tmp.addr[pos], curKey);
            if (!result.second) {
                return result;
            }
            bool sonType;
            read(sonType, tmp.addr[pos]);
            if (!sonType) { //type of son is body
                bodyNode self;
                bodyNode neighbor;
                read(self, tmp.addr[pos] + sizeof(bool));

                if (pos == tmp.n) { //special check
                    read(neighbor, tmp.addr[pos-1] + sizeof(bool));
                    if (neighbor.n > (M - 1) / 2) {
                        --neighbor.n;
                        ++self.n;
                        for (int i = self.n - 1; i > 0; --i) {
                            self.key[i] = self.key[i-1];
                            self.addr[i+1] = self.addr[i];
                        }
                        self.addr[1] = self.addr[0];

                        self.key[0] = tmp.key[pos-1];
                        self.addr[0] = neighbor.addr[neighbor.n+1];
                        tmp.key[pos-1] = neighbor.key[neighbor.n];
                        write(tmp, addr + sizeof(bool));
                        write(neighbor, tmp.addr[pos-1] + sizeof(bool));
                        write(self, tmp.addr[pos] + sizeof(bool));
                        return {true, false};
                    } else {    //neighbor.n <= (M - 1)/2
                        --tmp.n;
                        for (int i = 0; i < self.n; ++i) {
                            neighbor.key[i+neighbor.n+1] = self.key[i];
                            neighbor.addr[i+neighbor.n+1] = self.addr[i];
                        }
                        neighbor.addr[self.n+neighbor.n+1] = self.addr[self.n];
                        neighbor.key[neighbor.n] = tmp.key[pos-1];
                        neighbor.n += self.n + 1;
                        recycle(tmp.addr[pos]);
                        write(tmp, addr + sizeof(bool));
                        write(neighbor, tmp.addr[pos-1] + sizeof(bool));
                        if (tmp.n < (M - 1) / 2) {
                            return {true, true};
                        } else {
                            return {true, false};
                        }
                    }
                } else {    //if pos < tmp.n
                    read(neighbor, tmp.addr[pos+1] + sizeof(bool));
                    if (neighbor.n > (M - 1) / 2) {
                        ++self.n;
                        --neighbor.n;
                        self.key[self.n-1] = tmp.key[pos];
                        self.addr[self.n] = neighbor.addr[0];
                        tmp.key[pos] = neighbor.key[0];
                        for (int i = 0; i < neighbor.n; ++i) {
                            neighbor.key[i] = neighbor.key[i+1];
                            neighbor.addr[i] = neighbor.addr[i+1];
                        }
                        neighbor.addr[neighbor.n] = neighbor.addr[neighbor.n+1];
                        write(tmp, addr + sizeof(bool));
                        write(self, tmp.addr[pos] + sizeof(bool));
                        write(neighbor, tmp.addr[pos+1] + sizeof(bool));
                        return {true, false};

                    } else {    //neighbor.n <= (M - 1) / 2
                        --tmp.n;
                        for(int i = 0; i < neighbor.n; ++i) {
                            self.key[i+self.n+1] = neighbor.key[i];
                            self.addr[i+self.n+1] = neighbor.addr[i];
                        }
                        self.addr[neighbor.n+self.n+1] = neighbor.addr[neighbor.n];
                        self.key[self.n] = tmp.key[pos];
                        self.n += neighbor.n + 1;
                        recycle(tmp.addr[pos+1]);
                        for (int i = pos; i < tmp.n; ++i) {
                            tmp.key[i] = tmp.key[i+1];
                            tmp.addr[i+1] = tmp.addr[i+2];
                        }
                        write(tmp, addr + sizeof(bool));
                        write(self, tmp.addr[pos] + sizeof(bool));
                        if (tmp.n < (M - 1) / 2) {
                            return {true, true};
                        } else {
                            return {true, false};
                        }
                    }
                }
            } else {    //type of son is leaf
                leafNode self;
                leafNode neighbor;
                read(self, tmp.addr[pos] + sizeof(bool));
                if (pos == tmp.n) { //special check
                    read(neighbor, tmp.addr[pos-1] + sizeof(bool));
                    if (neighbor.n > L / 2) {
                        --neighbor.n;
                        ++self.n;
                        for (int i = self.n - 1; i > 0; --i) {
                            self.data[i] = self.data[i-1];
                        }
                        self.data[0] = neighbor.data[neighbor.n];
                        tmp.key[pos-1] = self.data[0].key;
                        write(tmp, addr + sizeof(bool));
                        write(neighbor, tmp.addr[pos-1] + sizeof(bool));
                        write(self, tmp.addr[pos] + sizeof(bool));
                        return {true, false};
                    } else {    //neighbor.n <= L/2
                        --tmp.n;
                        for (int i = 0; i < self.n; ++i) {
                            neighbor.data[i+neighbor.n] = self.data[i];
                        }
                        neighbor.n += self.n;
                        neighbor.nextAddr = self.nextAddr;
                        if (self.nextAddr != -1) {
                            leafNode selfNext;
                            read(selfNext, self.nextAddr + sizeof(bool));
                            selfNext.preAddr = self.preAddr;
                            write(selfNext, self.nextAddr + sizeof(bool));
                        }
                        write(tmp, addr + sizeof(bool));
                        write(neighbor, tmp.addr[pos-1] + sizeof(bool));
                        recycle(tmp.addr[pos]);
                        if (tmp.n < (M - 1) / 2) {
                            return {true, true};
                        } else {
                            return {true, false};
                        }
                    }
                } else {    //if pos < tmp.n
                    read(neighbor, tmp.addr[pos+1] + sizeof(bool));
                    if (neighbor.n > L / 2) {
                        --neighbor.n;
                        ++self.n;
                        self.data[self.n-1] = neighbor.data[0];
                        for (int i = 0; i < neighbor.n; ++i) {
                            neighbor.data[i] = neighbor.data[i+1];
                        }
                        tmp.key[pos] = neighbor.data[0].key;
                        if(pos > 0) tmp.key[pos-1] = self.data[0].key;
                        write(tmp, addr + sizeof(bool));
                        write(self, tmp.addr[pos] + sizeof(bool));
                        write(neighbor, tmp.addr[pos+1] + sizeof(bool));
                        return {true, false};
                    } else {    //neighbor.n <= L/2
                        --tmp.n;
                        for (int i = 0; i < neighbor.n; ++i) {
                            self.data[i+self.n] = neighbor.data[i];
                        }
                        self.n += neighbor.n;
                        self.nextAddr = neighbor.nextAddr;
                        if (neighbor.nextAddr != -1) {
                            leafNode neighborNext;
                            read(neighborNext, neighbor.nextAddr + sizeof(bool));
                            neighborNext.preAddr = neighbor.preAddr;
                            write(neighborNext, neighbor.nextAddr + sizeof(bool));
                        }
                        recycle(tmp.addr[pos+1]);
                        for (int i = pos; i < tmp.n; ++i) {
                            tmp.key[i] = tmp.key[i+1];
                            tmp.addr[i+1] = tmp.addr[i+2];
                        }
                        if(pos > 0) tmp.key[pos-1] = self.data[0].key;
                        write(tmp, addr + sizeof(bool));
                        write(self, tmp.addr[pos] + sizeof(bool));
                        if (tmp.n < (M - 1) / 2) {
                            return {true, true};
                        } else {
                            return {true, false};
                        }
                    }
                }
            }



        } else {    //leaf node
            leafNode tmp;
            read(tmp);
            int pos = -1;
            for (int i = 0; i < tmp.n; ++i) {
                if (tmp.data[i].key == curKey) {
                    pos = i;
                    break;
                }
            }
            if (pos == -1) {
                return {false, false};
            }
            tmp.n--;
            for (int i = pos; i < tmp.n; ++i) {
                tmp.data[i] = tmp.data[i+1];
            }
            write(tmp, addr + sizeof(bool));
            if (tmp.n < L / 2) {
                return {true, true};
            } else {
                return {true, false};
            }
        }
    }
    bool Modify(int addr, const Key &curKey, const T &curValue) {
        bool nodeType;
        read(nodeType, addr);
        if (!nodeType) {    //body node
            bodyNode tmp;
            read(tmp);
            int pos = 0;
            for(int i = tmp.n - 1; i >= 0; --i) {
                if(!(curKey < tmp.key[i])) {
                    pos = i + 1;
                    break;
                }
            }
            return Modify(tmp.addr[pos], curKey, curValue);
        } else {
            leafNode tmp;
            read(tmp);
            int pos = -1;
            for (int i = 0; i < tmp.n; ++i) {
                if (tmp.data[i].key == curKey) {
                    pos = i;
                    break;
                }
            }
            if (pos != -1) {
                tmp.data[pos].value = curValue;
                write(tmp, addr + sizeof(bool));
                return true;
            }
            return false;
        }
    }
    pair<bool, T> Find(int addr, const Key &key) {
        bool nodeType;
        read(nodeType, addr);
        //std::cerr << nodeType << std::endl;

        if (!nodeType) {    //body node
            bodyNode tmp;
            read(tmp);
            int pos = 0;
            for (int i = tmp.n - 1; i >= 0; --i) {
                if (!(key < tmp.key[i])) {
                    pos = i + 1;
                    break;
                }
            }
            return Find(tmp.addr[pos], key);
        } else {    //leaf node
            leafNode tmp;
            read(tmp);
            if (key < tmp.data[0].key) {
                return pair<bool, T>(false, T());
            }
            /*
            if (key < tmp.data[n-1].key) {
                bool ok = false;
                for (int i = 0; i < n; ++i) {
                    if (tmp.data[i].key == key) {
                        ok = true;
                        std::cout<<tmp.data[i].value<<' ';
                    }
                }
                return ok;
        }*/
            int n = tmp.n;
            for (int i = 0; i < n; ++i) {
                if (tmp.data[i].key == key) {
                    return pair<bool, T>(true, tmp.data[i].value);
                } else if (key < tmp.data[i].key) {
                    break;
                }
            }
            return pair<bool, T>(false, T());
        }
    }
public:

    BPT() : BPT("file") {}
    explicit BPT(char *FILE) {
        strcpy(filename, FILE);
        io.open(filename, std::fstream::binary|std::fstream::in | std::fstream::out);
        if(!io) {
            io.close();
            io.open(filename, std::fstream::binary | std::fstream::out);
            siz = 0;
            last = 0;
            //write(ZHANWEIFU(), 0);
            io.close();
            io.open(filename, std::fstream::binary|std::fstream::in | std::fstream::out);
        }
        else {
            read(siz, 0);
            read(root);
            read(last);
        }
        //std::cout<<sizeof(bodyNode)<<' '<<sizeof(leafNode)<<'\n';
    }

    void clear() {
        siz = 0;
        last = 0;
    }

    ~BPT() {
        write(siz, 0);
        write(root);
        write(last);
        io.close();
    }


    void insert(const Key &key, const T &value) {
        dataInfo curData(key, value);
        if (empty()) {
            clearBin();
            root = newPos();
            leafNode tmp;
            tmp.n = 1;
            tmp.data[0] = curData;
            write(true, root);
            write(tmp);
            ++siz;
            return;
        }
        ++siz;
        bool rootType;
        read(rootType, root);
        if (rootType) {    //root is leaf
            int pos = 0;
            leafNode rootNode;
            read(rootNode);
            for(int i = rootNode.n - 1; i >= 0; --i) {
                if (!(curData < rootNode.data[i])) {
                    pos = i + 1;
                    break;
                }
            }
            for (int i = rootNode.n; i > pos; --i) {
                rootNode.data[i] = rootNode.data[i-1];
            }
            rootNode.data[pos] = curData;
            rootNode.n++;
            if (rootNode.n <= L) {
                write(rootNode, root + sizeof(bool));
                return;
            } else {
                int addr1 = root;
                int addr2 = newPos();
                root = newPos();
                leafNode tmp;
                tmp.n = rootNode.n / 2;
                rootNode.n -= tmp.n;
                for (int i = 0; i < tmp.n; ++i) {
                    tmp.data[i] = rootNode.data[i+rootNode.n];
                }
                rootNode.nextAddr = addr2;
                tmp.preAddr = addr1;

                write(rootNode, addr1 + sizeof(bool));
                write(true, addr2);
                write(tmp);

                bodyNode newRootNode;
                newRootNode.n = 1;
                newRootNode.addr[0] = addr1;
                newRootNode.addr[1] = addr2;
                newRootNode.key[0] = tmp.data[0].key;

                write(false, root);
                write(newRootNode);
            }
        } else {
            bodyNode rootNode;
            read(rootNode);
            int pos = 0;
            for(int i = rootNode.n - 1; i >= 0; --i) {
                if(!(curData.key < rootNode.key[i])) {
                    pos = i + 1;
                    break;
                }
            }
            pair<bool, pair<Key, int> > result = Insert(rootNode.addr[pos], curData);
            //todo:xiu gai yi xia
            if (!result.first) {
                return;
            } else {
                for (int i = rootNode.n; i > pos; --i) {
                    rootNode.key[i] = rootNode.key[i-1];
                    rootNode.addr[i+1] = rootNode.addr[i];
                }
                rootNode.key[pos] = result.second.first;
                rootNode.addr[pos+1] = result.second.second;
                rootNode.n++;
                if (rootNode.n < M) {
                    write(rootNode, root + sizeof(bool));
                    return;
                } else {
                    int addr1 = root;
                    int addr2 = newPos();
                    root = newPos();

                    bodyNode tmp2;
                    tmp2.n = rootNode.n / 2;
                    rootNode.n = rootNode.n - tmp2.n - 1;

                    for (int i = 0; i < tmp2.n; ++i) {
                        tmp2.key[i] = rootNode.key[i+rootNode.n+1];
                        tmp2.addr[i+1] = rootNode.addr[i+rootNode.n+2];
                    }
                    tmp2.addr[0] = rootNode.addr[rootNode.n+1];

                    /*tmp2.addr[0] = -1;
                    for (int i = 0; i < tmp2.n; ++i) {
                        tmp2.key[i] = rootNode.key[i+rootNode.n];
                        tmp2.addr[i+1] = rootNode.addr[i+1+rootNode.n];
                    }*/
                    write(rootNode, addr1 + sizeof(bool));
                    write(false, addr2);
                    write(tmp2);


                    bodyNode newRootNode;
                    newRootNode.n = 1;
                    newRootNode.key[0] = rootNode.key[rootNode.n];
                    newRootNode.addr[0] = addr1;
                    newRootNode.addr[1] = addr2;
                    write(false, root);
                    write(newRootNode);
                }
            }
        }
    }

    bool erase(const Key &key) {   //return true if delete successfully.
        //    dataInfo curData(key, value);
        if (empty()) {
            return false;
        }
        bool rootType;
        read(rootType, root);
        if (!rootType) {    //root is body node.
            pair<bool, bool> result = Erase(root, key);
            if (!result.first) {
                return false;
            }
            --siz;
            bodyNode rootNode;
            read(rootNode, root + sizeof(bool));
            if (rootNode.n > 0) {
                //write(rootNode, root + sizeof(bool));
                return true;
            }
            recycle(root);
            root = rootNode.addr[0];
            return true;

        } else {    //root is leaf node.
            leafNode rootNode;
            read(rootNode);
            int pos = -1;
            for (int i = 0; i < rootNode.n; ++i) {
                if (rootNode.data[i].key == key) {
                    pos = i;
                    break;
                }
            }
            if (pos == -1) {
                return false;
            } else {
                --rootNode.n;
                --siz;
                for (int i = pos; i < rootNode.n; ++i) {
                    rootNode.data[i] = rootNode.data[i+1];
                }
                write(rootNode, root + sizeof(bool));
                return true;
            }
        }
    }

    bool modify(const Key &key, const T &value) {
        if (empty()) {
            return false;
        }
        return Modify(root, key, value);
    }

    pair<bool, T> find(const Key &key) {
        if (empty()) {
            return pair<bool, T>(false, T());
        }
        return Find(root, key);

    }
    inline bool empty() {
        return size() == 0;
    }

    inline int size() {
        return siz;
    }
};

#endif //FUCK_TICKET_SYSTEM_BPLUSTREE_H
