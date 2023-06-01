#include "BPlusTree.h"
//#define DISABLE_buy_ticket
//#define DISABLE_query_profile
#define DISABLE_query_ticket
#define DISABLE_query_transfer
#define DISABLE_query_order

int sumMonth[] = {0, 31,59,90,120,151,181,212,243,273,304,334,365};
inline int calcDay(int month, int day) {
    return sumMonth[month-1] + day;
}

inline int calcMinute(int hour, int minute) {
    return hour * 60 + minute;
}

inline int calcMinute(int month, int day, int hour, int minute) {
    return (calcDay(month, day) - 1) * 24 * 60 + calcMinute(hour, minute);
}

inline void printDate(int date) {   //print as "xx-xx"
    for (int i = 12; i >= 1; --i) {
        if (date > sumMonth[i-1]) {
            if (i < 10) putchar('0');
            printf("%d-", i);
            date -= sumMonth[i-1];
            if (date < 10) putchar('0');
            printf("%d", date);
            break;
        }
    }
}

inline int getDateByMinute(int minute) {
    return minute / 1440 + 1;
}

inline void printMinute(int minute) {   //print as "xx:xx"
    int hh = minute / 60, mm = minute % 60;
    if (hh < 10) putchar('0');
    printf("%d:", hh);
    if (mm < 10) putchar('0');
    printf("%d", mm);
}

inline void printDateAndMinute(int minute) {    //print as "xx-xx xx:xx"
    int date = minute / 1440 + 1;
    minute %= 1440;
    printDate(date);
    putchar(' ');
    printMinute(minute);
}

template<class T>
class wrapFstream {
    std::fstream io;
    int last{0};
public:
    explicit wrapFstream(char *FILE) {
        io.open(FILE, std::fstream::binary|std::fstream::in | std::fstream::out);
        if (!io) {
            io.close();
            io.open(FILE, std::fstream::binary | std::fstream::out);
            last = 0;
            io.close();
            io.open(FILE, std::fstream::binary|std::fstream::in | std::fstream::out);
        } else {
            read(last, 0);
        }
    }
    ~wrapFstream() {
        write(last, 0);
        io.close();
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

    int newPos() {
        last += sizeof(T);
        write(T(), last);
        return last;
    }

    void clear() {
        int last = 4;
        write(last, 0);
    }
};

class Station {
public:
    int head;
    Station(): head(-1) {}
    bool operator<(const Station &obj) const {
        return 0;   //no meaning
    }
};

struct trainOnStation {
    string_t trainID;
    int ordinal{0};
    int startTime{0};  //hh*60+mm, local time
    int saleDateBegin{0}, saleDateEnd{0};
    int arriveTime{0}, leaveTime{0}, arrivePrice{0}; //the time(minute) and price from start station to current station.
    int nextAddr{-1};
    //trainOnStation(): nextAddr(-1), trainID(){}
    bool operator<(const trainOnStation &obj) const {
        return trainID < obj.trainID;
    }
    inline bool checkLeaveDate(int date) {
        return date >= getDateByMinute((saleDateBegin - 1) * 1440 + startTime + leaveTime) && date <= getDateByMinute((saleDateEnd - 1) * 1440 + startTime + leaveTime);
    }
};

struct tripOnStation {
    string_t trainID;
    int startTime{0};  //local time
    int arriveTime{0}; //global time
    int takeTime{0};
    //Can be calctulated by arriveTime - startTime(x)
    int price{0};
    int seat{0};
    int nextAddr{-1};
    //tripOnStation(): nextAddr(-1) {}
};

class Train {   //All arrays are 0-base
    static constexpr int MAX_STATION_NUM = 100;
public:
    string_t trainID;
    int stationNum{0};
    string_t stations[MAX_STATION_NUM];
    int seatNum{0};
    int prices [MAX_STATION_NUM-1]{0};
    int startTime{0};  //calculated by hh * 60 + mm
    int travelTimes[MAX_STATION_NUM-1]{0};
    int stopoverTimes[MAX_STATION_NUM-1]{0};   //stopoverTimes[0] has no meaning.
    int saleDateBegin{0}, saleDateEnd{0};
    char type{0};
    Train() = default;
    Train(const Train &obj) {
        trainID = obj.trainID;
        stationNum = obj.stationNum;
        for (int i = 0; i < stationNum; ++i) stations[i] = obj.stations[i];
        seatNum = obj.seatNum;
        for (int i = 0; i < stationNum - 1; ++i) prices[i] = obj.prices[i], travelTimes[i] = obj.travelTimes[i];
        startTime = obj.startTime;
        for (int i = 1; i < stationNum - 1; ++i) stopoverTimes[i] = obj.stopoverTimes[i];
        saleDateBegin = obj.saleDateBegin;
        saleDateEnd = obj.saleDateEnd;
        type = obj.type;
    }
    bool operator<(const Train &obj) const {
        return trainID < obj.trainID;
    }
};

class Trip {    //车次
    static constexpr int MAX_STATION_NUM = 100;
public:
    //string_t trainID;
    //int Date;
    //todo: add some parameter
    int stationNum{0};
    int restSeat[MAX_STATION_NUM-1]{0};
    //int head{-1};
    bool operator<(const Trip &obj) const {
        return 0;   //no meaning
    }

    Trip() {}
    explicit Trip(int SN, int seat) {
        stationNum = SN;
        for (int i = 0; i < stationNum - 1; ++i) {
            restSeat[i] = seat;
        }
    }
    Trip(const Trip &obj) {
        stationNum = obj.stationNum;
        for (int i = 0; i < stationNum - 1; ++i) restSeat[i] = obj.restSeat[i];
    }
}tmpTrip[100];  //0-base

struct orderOnTrip {
    string_t username;
    int timestamp{0};
    int need{0};
    int sOrdinal{0};
    int tOrdinal{0};
    //int nextAddr{-1};
    orderOnTrip() {}
    bool operator<(const orderOnTrip &obj) const{
        return false;   //no meaning
    }
};

class Account {
public:
    string_t username;
    string_t password;
    string_t name;
    string_t mailAddr;
    int privilege{-1};
    //int head{-1};
    int orderNum{0};
    Account(): privilege(-1) {}
    bool operator<(const Account &obj) const {
        return username < obj.username;
    }
};

struct orderOnAccount {
    string_t trainID;
    string_t fromStationID, toStationID;
    int sOrdinal{0};
    int tOrdinal{0};
    int startDate{0};
    int leaveTime{0};
    int arriveTime{0};  //global time
    int price{0};
    int seat{0};
    int timeStamp{0};
    enum{success, pending, refunded} status{success};
    //int nextAddr{-1};
    bool operator<(const orderOnAccount &obj) const {
        return false;   //no meaning
    }
};

int queryTicketNum;
struct queryTicketResult {
    string_t trainID;
    int leaveTime{0};
    int takeTime{0};
    int price{0};
    int seat{0};
}FUCK[10001], FUCKTS[10001];

struct queryTransferResult {
    string_t trainID1, trainID2;
    int startTime1{0};     //local time
    int arriveTime1{0};    //global time
    int startTime2{0};     //global time;
    int arriveTime2{0};    //global time;
    string_t transferStation;
    int time{0};
    int price{0};
    int price1{0};
    int price2{0};
    int seat1{0};
    int seat2{0};

};

bool cmpQueryTicketResult(const queryTicketResult &a, const queryTicketResult &b, int p) {
    if (p == 0) {
        if (a.takeTime != b.takeTime) {
            return a.takeTime < b.takeTime;
        }
        return a.trainID < b.trainID;
    } else {
        if (a.price != b.price) {
            return a.price < b.price;
        }
        return a.trainID < b.trainID;
    }
}

bool cmpQueryTransferResult(const queryTransferResult &a, const queryTransferResult &b, int p) {
    if (p == 0) {
        if (a.time != b.time) return a.time < b.time;
        if (a.price != b.price) return a.price < b.price;
        if (a.trainID1 != b.trainID1) return a.trainID1 < b.trainID1;
        return a.trainID2 < b.trainID2;
    } else {
        if (a.price != b.price) return a.price < b.price;
        if (a.time != b.time) return a.time < b.time;
        if (a.trainID1 != b.trainID1) return a.trainID1 < b.trainID1;
        return a.trainID2 < b.trainID2;
    }
}

void mergeSort(int l, int r, int p) {
    if (l == r) return;
    int mid = (l + r) >> 1;
    mergeSort(l, mid, p);
    mergeSort(mid + 1, r, p);
    int i = l, j = mid + 1, k = l;
    while(i <= mid && j <= r) {
        if(cmpQueryTicketResult(FUCK[i], FUCK[j], p)) std::swap(FUCKTS[k++], FUCK[i++]);//FUCKTS[k++] = FUCK[i++];
        else std::swap(FUCKTS[k++], FUCK[j++]);
    }
    while(i <= mid) std::swap(FUCKTS[k++], FUCK[i++]);  //FUCKTS[k++] = FUCK[i++];
    while(j <= r) std::swap(FUCKTS[k++], FUCK[j++]);    //FUCKTS[k++] = FUCK[j++];
    for(k = l; k <= r; ++k) std::swap(FUCK[k], FUCKTS[k]);  //FUCK[k] = FUCKTS[k];
}

void quickSort(int l, int r, int p) {
    if (l >= r) return;
    int i = l, j = r;
    auto x = FUCK[l + (998244353 % (r - l + 1))];
    while (i <= j) {
        while (i <= j && cmpQueryTicketResult(FUCK[i], x, p)) ++i;
        while (i <= j && cmpQueryTicketResult(x, FUCK[j], p)) --j;
        if (i > j) break;
        std::swap(FUCK[i], FUCK[j]);
        ++i;
        --j;
    }
    quickSort(l, j, p);
    quickSort(i, r, p);
}

void sort(int l, int r, int p) {

    if (r <= 5) {
        for (int i = r; i >= l; --i)
            for (int j = l; j < i; ++j)
                if (cmpQueryTicketResult(FUCK[j+1], FUCK[j], p)) std::swap(FUCK[j+1], FUCK[j]);
    } else mergeSort(l, r, p);

    quickSort(l, r, p);
}

int switchTsToInt(char timeStamp[]) {
    int ts = 0;
    for (int i = 1; timeStamp[i] != ']'; ++i) ts = ts * 10 + timeStamp[i] - '0';
    return ts;
}

BPT<string_t, Account> userList("1");
BPT<string_t, bool> loginList("2");
BPT<string_t, Train> trainList("3");
BPT<string_t, bool> releaseList("4");
//BPT<string_t, Station> stationList("5");
//wrapFstream<trainOnStation> ioStation("6");
BPT<pair<string_t, int>, Trip> tripList("7");
//wrapFstream<orderOnTrip> ioOrder("8");
wrapFstream<tripOnStation> ioTrip("9");
BPT<string_t, Station> stationTfList("10");
//wrapFstream<orderOnAccount> ioAccount("11");
BPT<pair<string_t, int>, int> checkYourCode("12");

BPT<pair<string_t, int>, orderOnAccount> userToOrder("13");
BPT<pair<pair<string_t, int>, int>, orderOnTrip> tripToOrder("14");
BPT<pair<string_t, string_t>, trainOnStation> stationToTrain("15");

void satisfiyOrder(const string_t &username, int ts) {
    /*
    Account curUser = userList.find(username).second;
    int ptr = curUser.head;
    orderOnAccount orderA;
    while (ptr != -1) {
        ioAccount.read(orderA, ptr);
        if (orderA.timeStamp == ts) {
            assert(orderA.status == orderOnAccount::pending);
            orderA.status = orderOnAccount::success;
            ioAccount.write(orderA, ptr);
            break;
        }
        ptr = orderA.nextAddr;
    }
    int ptr = checkYourCode.find(pair<string_t, int>(username, ts)).second;
    checkYourCode.erase(pair<string_t, int>(username, ts));
    orderOnAccount orderA;
    ioAccount.read(orderA, ptr);
    orderA.status = orderOnAccount::success;
    ioAccount.write(orderA, ptr);*/

    auto orderA = userToOrder.find(pair<string_t, int>(username, ts)).second;
    orderA.status = orderOnAccount::success;
    userToOrder.modify(pair<string_t, int>(username, ts), orderA);
}


void query_order(BPT<pair<string_t, int>, orderOnAccount>::iterator &it, const string_t &username) {
    orderOnAccount orderA;
    it.getSecond(orderA);
    if (it.plusplus()) {
        pair<string_t, int> curKey;
        it.getFirst(curKey);
        if (curKey.first == username) query_order(it, username);
    }
    switch (orderA.status) {
        case orderOnAccount::success: printf("[success] "); break;
        case orderOnAccount::refunded: printf("[refunded] "); break;
        case orderOnAccount::pending: printf("[pending] "); break;
    }
    printf("%s %s ", orderA.trainID.s, orderA.fromStationID.s);
    printDateAndMinute(orderA.leaveTime);
    printf(" -> %s ", orderA.toStationID.s);
    printDateAndMinute(orderA.arriveTime);
    printf(" %d %d\n", orderA.price, orderA.seat);
}

char _key[50], operationName[20];
int timeStamp;
char tmpStr[10010];
int main() {
    string_t MIN_STRING_T;
    MIN_STRING_T.s[0] = 1;
    MIN_STRING_T.s[1] = 0;
    freopen("testcases/basic_2/1.in", "r", stdin);
    freopen("my.out", "w", stdout);
    bool isFirstUser = userList.empty();
    //std::string timeStamp, operationName, _key;

    scanf("%s", _key);

    while (true) {

        timeStamp = switchTsToInt(_key);
        scanf("%s", operationName);

        if (!strcmp(operationName, "add_user")) {
            Account newAcc;
            string_t curUsername;
            while (true) {
                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 'c': scanf("%s", curUsername.s); break;
                    case 'u': scanf("%s", newAcc.username.s); break;
                    case 'p': scanf("%s", newAcc.password.s); break;
                    case 'n': scanf("%s", newAcc.name.s); break;
                    case 'm': scanf("%s", newAcc.mailAddr.s); break;
                    case 'g': scanf("%d", &newAcc.privilege); break;
                }
            }

            if (isFirstUser) {
                newAcc.privilege = 10;
                userList.insert(newAcc.username, newAcc);
                isFirstUser = false;
                printf("[%d] 0\n", timeStamp);
            } else {
                bool isLogin = loginList.find(curUsername).first;
                auto curUser = userList.find(curUsername).second;
                //if (curUser.first || curUser.second.privilege > newAcc.privilege) {
                if (isLogin && curUser.privilege > newAcc.privilege) {
                    userList.insert(newAcc.username, newAcc);
                    printf("[%d] 0\n", timeStamp);
                } else {
                    printf("[%d] -1\n", timeStamp);
                }
            }
        }
        else if (!strcmp(operationName, "login")) {
            string_t username, password;
            while (true) {
                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 'u': scanf("%s", username.s); break;
                    case 'p': scanf("%s", password.s); break;
                }
            }
            auto curUser = userList.find(username);
            if (curUser.first && curUser.second.password == password && !loginList.find(username).first) {
                loginList.insert(username, true);
                printf("[%d] 0\n", timeStamp);
            } else {
                printf("[%d] -1\n", timeStamp);
            }
        }
        else if (!strcmp(operationName, "logout")) {
            string_t username;
            while (true) {
                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 'u': scanf("%s", username.s); break;
                }
            }

            if (loginList.erase(username)) {
                printf("[%d] 0\n", timeStamp);
            } else {
                printf("[%d] -1\n", timeStamp);
            }
        }
        else if (!strcmp(operationName, "query_profile")) {
            string_t curUsername, username;
            while (true) {
                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 'u': scanf("%s", username.s); break;
                    case 'c': scanf("%s", curUsername.s); break;
                }
            }

#ifndef DISABLE_query_profile
            bool ok = false;
            if (loginList.find(curUsername).first) {
                auto queryUser = userList.find(username);
                if (queryUser.first) {
                    auto curUser = userList.find(curUsername);
                    if (curUser.second.privilege > queryUser.second.privilege || curUsername == username) {
                        ok = true;
                        printf("[%d] %s %s %s %d\n", timeStamp, queryUser.second.username.s, queryUser.second.name.s, queryUser.second.mailAddr.s, queryUser.second.privilege);
                    }
                }
            }
            if (!ok) {
                printf("[%d] -1\n", timeStamp);
            }
#endif
        }
        else if (!strcmp(operationName, "modify_profile")) {
            Account newAcc;
            string_t curUsername;
            while (true) {
                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 'c': scanf("%s", curUsername.s); break;
                    case 'u': scanf("%s", newAcc.username.s); break;
                    case 'p': scanf("%s", newAcc.password.s); break;
                    case 'n': scanf("%s", newAcc.name.s); break;
                    case 'm': scanf("%s", newAcc.mailAddr.s); break;
                    case 'g': scanf("%d", &newAcc.privilege); break;
                }
            }
            bool isLogin = loginList.find(curUsername).first;
            auto curUser = userList.find(curUsername);
            auto modifyUser = userList.find(newAcc.username);
            if (isLogin && modifyUser.first && (curUser.second.privilege > modifyUser.second.privilege
                                                || curUsername == modifyUser.second.username) && newAcc.privilege < curUser.second.privilege) {
                if (!newAcc.password.empty()) {
                    modifyUser.second.password = newAcc.password;
                }
                if (!newAcc.name.empty()) {
                    modifyUser.second.name = newAcc.name;
                }
                if (!newAcc.mailAddr.empty()) {
                    modifyUser.second.mailAddr = newAcc.mailAddr;
                }
                if (newAcc.privilege != -1) {
                    modifyUser.second.privilege = newAcc.privilege;
                }

                userList.modify(modifyUser.second.username, modifyUser.second);
                printf("[%d] %s %s %s %d\n", timeStamp, modifyUser.second.username.s, modifyUser.second.name.s, modifyUser.second.mailAddr.s, modifyUser.second.privilege);
            } else {
                printf("[%d] -1\n", timeStamp);
            }

        }
        else if (!strcmp(operationName, "add_train")) {

            Train newTrain;
            while (true) {
                //std::string tmp;
                int last;

                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 'i': scanf("%s", newTrain.trainID.s); break;
                    case 'n': scanf("%d", &newTrain.stationNum); break;
                    case 'm': scanf("%d", &newTrain.seatNum); break;
                    case 's':
                        scanf("%s", tmpStr);
                        last = 0;
                        newTrain.stationNum = 0;
                        for (int i = 0, I = strlen(tmpStr), j = 0; i <= I; ++i) {
                            if (tmpStr[i] == '|' || tmpStr[i] == 0) {
                                //newTrain.stations[newTrain.stationNum++].assign(tmp.substr(last, i - last));
                                newTrain.stations[newTrain.stationNum++].s[j] = 0;
                                j = 0;
                            } else {
                                newTrain.stations[newTrain.stationNum].s[j] = tmpStr[i];
                                ++j;
                            }
                        }
                        //newTrain.stations[newTrain.stationNum++].assign(tmp.substr(last));
                        //newTrain.stations[newTrain.stationNum++].s[j] = 0;
                        break;

                    case 'p':
                        scanf("%s", tmpStr);
                        last = 0;
                        newTrain.stationNum = 0;
                        for (int i = 0, I = strlen(tmpStr); i < I; ++i) {
                            if (tmpStr[i] == '|') {
                                newTrain.prices[newTrain.stationNum++] = last;
                                last = 0;
                            } else {
                                last = last * 10 + tmpStr[i] - '0';
                            }
                        }
                        newTrain.prices[newTrain.stationNum++] = last;
                        ++newTrain.stationNum;
                        break;

                    case 'x':
                        scanf("%s", tmpStr);
                        newTrain.startTime = calcMinute((tmpStr[0] - '0') * 10 + (tmpStr[1] - '0'), (tmpStr[3] - '0') * 10 + (tmpStr[4] - '0'));
                        break;
                    case 't':
                        scanf("%s", tmpStr);
                        last = 0;
                        newTrain.stationNum = 0;
                        for (int i = 0, I = strlen(tmpStr); i < I; ++i) {
                            if (tmpStr[i] == '|') {
                                newTrain.travelTimes[newTrain.stationNum++] = last;
                                last = 0;
                            } else {
                                last = last * 10 + tmpStr[i] - '0';
                            }
                        }
                        newTrain.travelTimes[newTrain.stationNum++] = last;
                        ++newTrain.stationNum;
                        break;
                    case 'o':
                        scanf("%s", tmpStr);
                        if (tmpStr[0] != '_') {
                            newTrain.stationNum = 1;
                            last = 0;
                            for (int i = 0, I = strlen(tmpStr); i < I; ++i) {
                                if (tmpStr[i] == '|') {
                                    newTrain.stopoverTimes[newTrain.stationNum++] = last;
                                    last = 0;
                                } else {
                                    last = last * 10 + tmpStr[i] - '0';
                                }
                            }
                            newTrain.stopoverTimes[newTrain.stationNum++] = last;
                            ++newTrain.stationNum;
                        } else {
                            newTrain.stationNum = 2;
                        }
                        break;
                    case 'd':
                        scanf("%s", tmpStr);
                        newTrain.saleDateBegin = calcDay((tmpStr[0] - '0') * 10 + (tmpStr[1] - '0'), (tmpStr[3] - '0') * 10 + (tmpStr[4] - '0'));
                        newTrain.saleDateEnd = calcDay((tmpStr[6] - '0') * 10 + (tmpStr[7] - '0'), (tmpStr[9] - '0') * 10 + (tmpStr[10] - '0'));
                        break;
                    case 'y': scanf("%s", tmpStr); newTrain.type = tmpStr[0]; break;
                }

            }
            if (!trainList.find(newTrain.trainID).first) {
                trainList.insert(newTrain.trainID, newTrain);
                printf("[%d] 0\n", timeStamp);
            } else {
                printf("[%d] -1\n", timeStamp);
            }
        }
        else if (!strcmp(operationName, "delete_train")) {
            string_t trainID;
            while (true) {
                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 'i': scanf("%s", trainID.s); break;
                }
            }
            if (trainList.find(trainID).first && !releaseList.find(trainID).first) {
                trainList.erase(trainID);
                printf("[%d] 0\n", timeStamp);
            } else {
                printf("[%d] -1\n", timeStamp);
            }
        }
        else if (!strcmp(operationName, "release_train")) {
            string_t trainID;
            while (true) {
                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 'i': scanf("%s", trainID.s); break;
                }
            }
            auto curTrain = trainList.find(trainID);
            if (curTrain.first && !releaseList.find(trainID).first) {
                releaseList.insert(trainID, true);

                Trip curTrip(curTrain.second.stationNum, curTrain.second.seatNum);
                for (int i = curTrain.second.saleDateBegin; i <= curTrain.second.saleDateEnd; ++i) {
                    tripList.insert(pair<string_t, int>(trainID, i), curTrip);
                }

                //int preTime = 0, prePrice = 0;
                trainOnStation nodeInfo;
                nodeInfo.trainID = curTrain.second.trainID;
                nodeInfo.startTime = curTrain.second.startTime;
                nodeInfo.saleDateBegin = curTrain.second.saleDateBegin;
                nodeInfo.saleDateEnd = curTrain.second.saleDateEnd;
                nodeInfo.arriveTime = nodeInfo.arrivePrice = nodeInfo.leaveTime = 0;

                for (int i = 0; i < curTrain.second.stationNum; ++i) {
                    nodeInfo.nextAddr = -1;
                    nodeInfo.ordinal = i;
                    if (i > 0) {
                        //    nodeInfo.arriveTime += curTrain.second.travelTimes[i-1];
                        //    nodeInfo.arriveTime += curTrain.second.prices[i-1];
                        nodeInfo.arrivePrice += curTrain.second.prices[i-1];
                        nodeInfo.arriveTime = nodeInfo.leaveTime + curTrain.second.travelTimes[i-1];
                        if (i < curTrain.second.stationNum - 1) nodeInfo.leaveTime = nodeInfo.arriveTime + curTrain.second.stopoverTimes[i];
                    }
                    stationToTrain.insert(pair<string_t, string_t>(curTrain.second.stations[i], nodeInfo.trainID), nodeInfo);

                    /*
                    string_t stationID(curTrain.second.stations[i]);
                    auto curStation = stationList.find(stationID);
                    int head = curStation.second.head;

                    if (head == -1) {
                        head = ioStation.newPos();
                        ioStation.write(nodeInfo, head);
                    } else {
                        trainOnStation tmp;
                        ioStation.read(tmp, head);
                        if (nodeInfo < tmp) {
                            nodeInfo.nextAddr = head;
                            head = ioStation.newPos();
                            ioStation.write(nodeInfo, head);
                        }
                        else {
                            int last = head;
                            while (true) {
                                if (tmp.nextAddr == -1) {
                                    tmp.nextAddr = ioStation.newPos();
                                    ioStation.write(nodeInfo, tmp.nextAddr);
                                    ioStation.write(tmp, last);
                                    break;
                                }
                                trainOnStation next;
                                ioStation.read(next, tmp.nextAddr);
                                if (nodeInfo < next) {
                                    nodeInfo.nextAddr = tmp.nextAddr;
                                    tmp.nextAddr = ioStation.newPos();
                                    ioStation.write(nodeInfo, tmp.nextAddr);
                                    ioStation.write(tmp, last);
                                    break;
                                } else {
                                    last = tmp.nextAddr;
                                    tmp = next;
                                }
                            }
                        }
                    }
                    if (head != curStation.second.head) {
                        curStation.second.head = head;
                        if(curStation.first) stationList.modify(stationID, curStation.second);
                        else stationList.insert(stationID, curStation.second);
                    }
                    */
                }
                printf("[%d] 0\n", timeStamp);
            } else {
                printf("[%d] -1\n", timeStamp);
            }
        }
        else if (!strcmp(operationName, "query_train")) {

            int date;
            string_t trainID;
            char dateString[6];
            while (true) {
                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 'i': scanf("%s", trainID.s); break;
                    case 'd':
                        scanf("%s", dateString);
                        date = calcDay((dateString[0] - '0') * 10 + (dateString[1] - '0'), (dateString[3] - '0') * 10 + (dateString[4] - '0'));
                        break;
                }
            }

#ifndef DISABLE_query_train
            auto curTrain = trainList.find(trainID);
            if (curTrain.first && date >= curTrain.second.saleDateBegin && date <= curTrain.second.saleDateEnd) {
                //std::cout << timeStamp << ' ' << trainID << ' ' << curTrain.second.type << std::endl;
                printf("[%d] %s %c\n", timeStamp, trainID.s, curTrain.second.type);
                int stationNum = curTrain.second.stationNum;
                bool isRelease = releaseList.find(trainID).first;
                Trip curTrip(stationNum, curTrain.second.seatNum);
                if (isRelease) {
                    curTrip = tripList.find(pair<string_t, int>(trainID, date)).second;
                }
                //std::cout << curTrain.second.stations[0] << " xx-xx xx:xx -> ";
                printf("%s xx-xx xx:xx -> ", curTrain.second.stations[0].s);
                printDate(date);
                putchar(' ');
                printMinute(curTrain.second.startTime);
                //std::cout << ' ' << 0 << ' ' << curTrip.restSeat[0] << std::endl;
                printf(" 0 %d\n", curTrip.restSeat[0]);
                int curTime = (date - 1) * 1440 + curTrain.second.startTime, curPrice = 0;

                for (int i = 1; i < stationNum - 1; ++i) {
                    //std::cout << curTrain.second.stations[i] << ' ';
                    printf("%s ", curTrain.second.stations[i].s);
                    curTime += curTrain.second.travelTimes[i-1];
                    printDateAndMinute(curTime);
                    printf(" -> ");
                    curTime += curTrain.second.stopoverTimes[i];
                    printDateAndMinute(curTime);
                    curPrice += curTrain.second.prices[i-1];
                    //std::cout << ' ' << curPrice << ' ' << curTrip.restSeat[i] << std::endl;
                    printf(" %d %d\n", curPrice, curTrip.restSeat[i]);
                }

                //std::cout << curTrain.second.stations[stationNum - 1] << ' ';
                printf("%s ", curTrain.second.stations[stationNum - 1].s);
                curTime += curTrain.second.travelTimes[stationNum - 2];
                curPrice += curTrain.second.prices[stationNum - 2];
                printDateAndMinute(curTime);
                //std::cout << " -> xx-xx xx:xx " << curPrice << " x" << std::endl;
                printf(" -> xx-xx xx:xx %d x\n", curPrice);
            } else {
                //std::cout << timeStamp << ' ' << -1 << std::endl;
                printf("[%d] -1\n", timeStamp);
            }
#endif
        }
        else if (!strcmp(operationName, "query_ticket")) {

            string_t startStationID, toStationID;
            char dateString[6];
            int date;
            int sortPattern = 0;
            while (true) {
                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 's': scanf("%s", startStationID.s); break;
                    case 't': scanf("%s", toStationID.s); break;
                    case 'd':
                        scanf("%s", dateString);
                        date = calcDay((dateString[0] - '0') * 10 + (dateString[1] - '0'), (dateString[3] - '0') * 10 + (dateString[4] - '0'));
                        break;
                    case 'p':
                        scanf("%s", tmpStr);
                        if (!strcmp(tmpStr, "time")) sortPattern = 0;
                        else sortPattern = 1;
                }
            }
#ifndef DISABLE_query_ticket
            //Station startStation = stationList.find(startStationID).second;
            //Station toStation = stationList.find(toStationID).second;

            //int ps = startStation.head, pt = toStation.head;

            BPT<pair<string_t, string_t>, trainOnStation>::iterator itS, itT;
            queryTicketNum = 0;
            if (stationToTrain.lower_bound(itS, pair<string_t, string_t>(startStationID, MIN_STRING_T))
                && stationToTrain.lower_bound(itT, pair<string_t, string_t>(toStationID, MIN_STRING_T))) {
                trainOnStation ss, tt;
                pair<string_t, string_t> tmp;
                bool flag = true;
                itS.getFirst(tmp);
                if (tmp.first != startStationID) flag = false;
                itS.getSecond(ss);

                itT.getFirst(tmp);
                if (tmp.first != toStationID) flag = false;
                itT.getSecond(tt);
            if (flag)
                while (true) {
                    if (ss.trainID == tt.trainID) {
                        if (ss.ordinal < tt.ordinal && ss.checkLeaveDate(date)) {
                            ++queryTicketNum;
                            FUCK[queryTicketNum].trainID = ss.trainID;
                            FUCK[queryTicketNum].takeTime = tt.arriveTime - ss.leaveTime;
                            FUCK[queryTicketNum].price = tt.arrivePrice - ss.arrivePrice;
                            FUCK[queryTicketNum].leaveTime = (ss.leaveTime + ss.startTime) % 1440;

                            int startDate = getDateByMinute((date - 1) * 1440 + FUCK[queryTicketNum].leaveTime - ss.leaveTime);

                            Trip curTrip = tripList.find(pair<string_t, int>(ss.trainID, startDate)).second;
                            FUCK[queryTicketNum].seat = curTrip.restSeat[ss.ordinal];
                            for (int i = ss.ordinal + 1; i < tt.ordinal; ++i) {
                                FUCK[queryTicketNum].seat = std::min(FUCK[queryTicketNum].seat, curTrip.restSeat[i]);
                            }
                        }
                        if (!itS.plusplus()) break;
                        itS.getFirst(tmp);
                        if (tmp.first != startStationID) break;
                        itS.getSecond(ss);

                        if (!itT.plusplus()) break;
                        itT.getFirst(tmp);
                        if (tmp.first != toStationID) break;
                        itT.getSecond(tt);
                    } else if (ss.trainID < tt.trainID) {
                        if (!itS.plusplus()) break;
                        itS.getFirst(tmp);
                        if (tmp.first != startStationID) break;
                        itS.getSecond(ss);
                    } else {
                        if (!itT.plusplus()) break;
                        itT.getFirst(tmp);
                        if (tmp.first != toStationID) break;
                        itT.getSecond(tt);
                    }
                }
            }
            /*
            if (ps != -1 && pt != -1) {
                trainOnStation ss, tt;
                ioStation.read(ss, ps);
                ioStation.read(tt, pt);
                while (ps != -1 && pt != -1) {
                    if (ss.trainID == tt.trainID) {
                        if (ss.ordinal < tt.ordinal && ss.checkLeaveDate(date)) {
                            ++queryTicketNum;
                            FUCK[queryTicketNum].trainID = ss.trainID;
                            FUCK[queryTicketNum].takeTime = tt.arriveTime - ss.leaveTime;
                            FUCK[queryTicketNum].price = tt.arrivePrice - ss.arrivePrice;
                            FUCK[queryTicketNum].leaveTime = (ss.leaveTime + ss.startTime) % 1440;

                            int startDate = getDateByMinute((date - 1) * 1440 + FUCK[queryTicketNum].leaveTime - ss.leaveTime);

                            Trip curTrip = tripList.find(pair<string_t, int>(ss.trainID, startDate)).second;
                            FUCK[queryTicketNum].seat = curTrip.restSeat[ss.ordinal];
                            for (int i = ss.ordinal + 1; i < tt.ordinal; ++i) {
                                FUCK[queryTicketNum].seat = std::min(FUCK[queryTicketNum].seat, curTrip.restSeat[i]);
                            }
                        }
                        ps = ss.nextAddr, pt = tt.nextAddr;
                        if (ps != -1 && pt != -1) {
                            ioStation.read(ss, ps);
                            ioStation.read(tt, pt);
                        }
                    } else if (ss.trainID < tt.trainID) {
                        ps = ss.nextAddr;
                        if (ps != -1) {
                            ioStation.read(ss, ps);
                        }
                    } else {
                        pt = tt.nextAddr;
                        if (pt != -1) {
                            ioStation.read(tt, pt);
                        }
                    }
                }
            }
            */
            if (queryTicketNum > 0) {
                sort(1, queryTicketNum, sortPattern);
            }
            printf("[%d] %d\n", timeStamp, queryTicketNum);
            for (int i = 1; i <= queryTicketNum; ++i) {
                //std::cout << FUCK[i].trainID << ' ' << startStationID << ' ' << dateString << ' ';
                printf("%s %s %s ", FUCK[i].trainID.s, startStationID.s, dateString);
                printMinute(FUCK[i].leaveTime);
                //std::cout << " -> " << toStationID << ' ';
                printf(" -> %s ", toStationID.s);
                printDateAndMinute((date - 1) * 1440 + FUCK[i].leaveTime + FUCK[i].takeTime);
                //std::cout << ' ' << FUCK[i].price << ' ' << FUCK[i].seat << std::endl;
                printf(" %d %d\n", FUCK[i].price, FUCK[i].seat);
            }
#endif
        }
        else if (!strcmp(operationName, "query_transfer")) {
            string_t startStationID, toStationID;
            char dateString[6];
            int date;
            int sortPattern = 0;
            while (true) {
                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 's': scanf("%s", startStationID.s); break;
                    case 't': scanf("%s", toStationID.s); break;
                    case 'd':
                        //std::cin >> dateString;
                        scanf("%s", dateString);
                        date = calcDay((dateString[0] - '0') * 10 + (dateString[1] - '0'), (dateString[3] - '0') * 10 + (dateString[4] - '0'));
                        break;
                    case 'p':
                        //std::cin >> tmp;
                        scanf("%s", tmpStr);
                        //if (tmp == "time") sortPattern = 0;
                        if (!strcmp(tmpStr, "time")) sortPattern = 0;
                        else sortPattern = 1;
                }
            }
#ifndef DISABLE_query_transfer
            BPT<pair<string_t, string_t>, trainOnStation>::iterator itS, itT;
            bool ok = 0;
            queryTransferResult res;
            if (stationToTrain.lower_bound(itS, pair<string_t, string_t>(startStationID, MIN_STRING_T))
                && stationToTrain.lower_bound(itT, pair<string_t, string_t>(toStationID, MIN_STRING_T))) {
                ioTrip.clear();
                stationTfList.clear();
                trainOnStation ss, tt;
                pair<string_t, string_t> tmp;

                bool flag = true;
                itS.getFirst(tmp);
                if (tmp.first != startStationID) flag = false;
                itS.getSecond(ss);

                itT.getFirst(tmp);
                if (tmp.first != toStationID) flag = false;
                itT.getSecond(tt);
                if (flag) {
                    while (true) {

                        if (ss.checkLeaveDate(date)) {

                            Train curTrain = trainList.find(ss.trainID).second;
                            int startDate = getDateByMinute(
                                    (date - 1) * 1440 + (ss.startTime + ss.leaveTime) % 1440 - ss.leaveTime);
                            Trip curTrip = tripList.find(pair<string_t, int>(ss.trainID, startDate)).second;
                            //int maxSeat = curTrip.restSeat[ss.ordinal], price = 0, taketime = 0;
                            tripOnStation tripInfo;
                            tripInfo.trainID = ss.trainID;
                            tripInfo.startTime = (ss.startTime + ss.leaveTime) % 1440;
                            tripInfo.arriveTime = (date - 1) * 1440 + tripInfo.startTime;
                            tripInfo.takeTime = 0;
                            tripInfo.seat = curTrip.restSeat[ss.ordinal];
                            tripInfo.price = 0;
                            for (int i = ss.ordinal + 1; i < curTrain.stationNum; ++i) {
                                tripInfo.seat = std::min(tripInfo.seat, curTrip.restSeat[i - 1]);
                                tripInfo.price += curTrain.prices[i - 1];
                                tripInfo.takeTime += curTrain.travelTimes[i - 1];
                                tripInfo.arriveTime += curTrain.travelTimes[i - 1];
                                tripInfo.nextAddr = -1;
                                Station curStation = stationTfList.find(curTrain.stations[i]).second;

                                if (curStation.head == -1) {
                                    curStation.head = ioTrip.newPos();
                                    ioTrip.write(tripInfo, curStation.head);
                                    stationTfList.insert(curTrain.stations[i], curStation);
                                } else {
                                    tripOnStation ttmp;
                                    ioTrip.read(ttmp, curStation.head);
                                    tripInfo.nextAddr = ttmp.nextAddr;
                                    ttmp.nextAddr = ioTrip.newPos();
                                    ioTrip.write(ttmp, curStation.head);
                                    ioTrip.write(tripInfo, ttmp.nextAddr);

                                }

                                if (i < curTrain.stationNum - 1) {
                                    tripInfo.takeTime += curTrain.stopoverTimes[i];
                                    tripInfo.arriveTime += curTrain.stopoverTimes[i];
                                }
                            }
                        }

                        if (!itS.plusplus()) break;
                        itS.getFirst(tmp);
                        if (tmp.first != startStationID) break;
                        itS.getSecond(ss);
                    }
                    while (true) {

                        Train curTrain = trainList.find(tt.trainID).second;

                        for (int i = curTrain.saleDateBegin; i <= curTrain.saleDateEnd; ++i) {
                            tmpTrip[i - curTrain.saleDateBegin].stationNum = -1;
                        }
                        int price = 0, totalTime = 0, tttt;
                        for (int i = 0; i < tt.ordinal; ++i) {
                            totalTime += curTrain.stopoverTimes[i];
                            totalTime += curTrain.travelTimes[i];
                        }
                        tttt = totalTime;
                        //if (tt.ordinal < curTrain.stationNum - 1) totalTime += curTrain.stopoverTimes[tt.ordinal];
                        for (int i = tt.ordinal - 1; i >= 0; --i) {
                            //    takeTime += curTrain.travelTimes[i];
                            totalTime -= curTrain.travelTimes[i];
                            price += curTrain.prices[i];
                            Station curStation = stationTfList.find(curTrain.stations[i]).second;
                            if (curStation.head != -1) {
                                tripOnStation ttmp;
                                ioTrip.read(ttmp, curStation.head);
                                while (true) {
                                    //todo: whether "<=" or "<"
                                    if (ttmp.trainID != curTrain.trainID && ttmp.arriveTime <=
                                                                            (curTrain.saleDateEnd - 1) * 1440 +
                                                                            curTrain.startTime + totalTime) {
                                        int startDay = curTrain.saleDateEnd - std::min(
                                                ((curTrain.saleDateEnd - 1) * 1440 + curTrain.startTime + totalTime -
                                                 ttmp.arriveTime) / 1440,
                                                curTrain.saleDateEnd - curTrain.saleDateBegin);

                                        if (tmpTrip[startDay - curTrain.saleDateBegin].stationNum == -1) {

                                            tmpTrip[startDay - curTrain.saleDateBegin] =
                                                    tripList.find(
                                                            pair<string_t, int>(curTrain.trainID, startDay)).second;
                                        }
                                        auto &curTrip = tmpTrip[startDay - curTrain.saleDateBegin];
                                        queryTransferResult curRes;
                                        curRes.trainID1 = ttmp.trainID;
                                        curRes.trainID2 = curTrain.trainID;
                                        curRes.time = ((startDay - 1) * 1440 + curTrain.startTime + tttt) -
                                                      (ttmp.startTime + (date - 1) * 1440);
                                        curRes.startTime1 = ttmp.startTime;
                                        curRes.arriveTime1 = ttmp.arriveTime;
                                        curRes.startTime2 = (startDay - 1) * 1440 + curTrain.startTime + totalTime;
                                        curRes.arriveTime2 = (startDay - 1) * 1440 + curTrain.startTime + tttt;
                                        curRes.price = ttmp.price + price;
                                        curRes.price1 = ttmp.price;
                                        curRes.price2 = price;
                                        curRes.transferStation = curTrain.stations[i];
                                        curRes.seat1 = ttmp.seat;
                                        curRes.seat2 = curTrip.restSeat[i];
                                        for (int j = i; j < tt.ordinal; ++j)
                                            curRes.seat2 = std::min(curRes.seat2, curTrip.restSeat[j]);

                                        if (!ok || cmpQueryTransferResult(curRes, res, sortPattern)) {
                                            ok = true;
                                            res = curRes;
                                        }
                                    }

                                    int next = ttmp.nextAddr;
                                    if (next == -1) break;
                                    ioTrip.read(ttmp, next);
                                }
                            }

                            totalTime -= curTrain.stopoverTimes[i];
                            //if (i > 0) takeTime += curTrain.stopoverTimes[i];
                        }

                        if (!itT.plusplus()) break;
                        itT.getFirst(tmp);
                        if (tmp.first != toStationID) break;
                        itT.getSecond(tt);
                    }
                }
            }
            //FUCK_IF_2:;
            /*
            Station startStation = stationList.find(startStationID).second;
            Station toStation = stationList.find(toStationID).second;
            int ps = startStation.head, pt = toStation.head;
            bool ok = false;
            queryTransferResult res;
            if (ps != -1 && pt != -1) {
                trainOnStation ss, tt;
                ioTrip.clear();
                stationTfList.clear();
                while (ps != -1) {
                    ioStation.read(ss, ps);
                    if (ss.checkLeaveDate(date)) {

                        Train curTrain = trainList.find(ss.trainID).second;
                        int startDate = getDateByMinute((date - 1) * 1440 + (ss.startTime + ss.leaveTime) % 1440 - ss.leaveTime);
                        Trip curTrip = tripList.find(pair<string_t, int>(ss.trainID, startDate)).second;
                        //int maxSeat = curTrip.restSeat[ss.ordinal], price = 0, taketime = 0;
                        tripOnStation tripInfo;
                        tripInfo.trainID = ss.trainID;
                        tripInfo.startTime = (ss.startTime + ss.leaveTime) % 1440;
                        tripInfo.arriveTime = (date - 1) * 1440 + tripInfo.startTime;
                        tripInfo.takeTime = 0;
                        tripInfo.seat = curTrip.restSeat[ss.ordinal];
                        tripInfo.price = 0;
                        for (int i = ss.ordinal + 1; i < curTrain.stationNum; ++i) {
                            tripInfo.seat = std::min(tripInfo.seat, curTrip.restSeat[i-1]);
                            tripInfo.price += curTrain.prices[i-1];
                            tripInfo.takeTime += curTrain.travelTimes[i-1];
                            tripInfo.arriveTime += curTrain.travelTimes[i-1];
                            tripInfo.nextAddr = -1;
                            Station curStation = stationTfList.find(curTrain.stations[i]).second;

                            if (curStation.head == -1) {
                                curStation.head = ioTrip.newPos();
                                ioTrip.write(tripInfo, curStation.head);
                                stationTfList.insert(curTrain.stations[i], curStation);
                            } else {
                                tripOnStation ttmp;
                                ioTrip.read(ttmp, curStation.head);
                                tripInfo.nextAddr = ttmp.nextAddr;
                                ttmp.nextAddr = ioTrip.newPos();
                                ioTrip.write(ttmp, curStation.head);
                                ioTrip.write(tripInfo, ttmp.nextAddr);

                            }

                            if (i < curTrain.stationNum - 1) {
                                tripInfo.takeTime += curTrain.stopoverTimes[i];
                                tripInfo.arriveTime += curTrain.stopoverTimes[i];
                            }
                        }
                    }
                    ps = ss.nextAddr;
                }

                while (pt != -1) {
                    ioStation.read(tt, pt);
                    Train curTrain = trainList.find(tt.trainID).second;

                    for (int i = curTrain.saleDateBegin; i <= curTrain.saleDateEnd; ++i) {
                        tmpTrip[i - curTrain.saleDateBegin].stationNum = -1;
                    }
                    int price = 0, totalTime = 0, tttt;
                    for (int i = 0; i < tt.ordinal; ++i) {
                        totalTime += curTrain.stopoverTimes[i];
                        totalTime += curTrain.travelTimes[i];
                    }
                    tttt = totalTime;
                    //if (tt.ordinal < curTrain.stationNum - 1) totalTime += curTrain.stopoverTimes[tt.ordinal];
                    for (int i = tt.ordinal - 1; i >= 0; --i) {
                        //    takeTime += curTrain.travelTimes[i];
                        totalTime -= curTrain.travelTimes[i];
                        price += curTrain.prices[i];
                        Station curStation = stationTfList.find(curTrain.stations[i]).second;
                        if (curStation.head != -1) {
                            tripOnStation ttmp;
                            ioTrip.read(ttmp, curStation.head);
                            while (true) {
                                //todo: whether "<=" or "<"
                                if (ttmp.trainID != curTrain.trainID && ttmp.arriveTime <= (curTrain.saleDateEnd - 1) * 1440 + curTrain.startTime + totalTime) {
                                    int startDay = curTrain.saleDateEnd - std::min(((curTrain.saleDateEnd - 1) * 1440 + curTrain.startTime + totalTime - ttmp.arriveTime) / 1440, curTrain.saleDateEnd - curTrain.saleDateBegin);

                                    if (tmpTrip[startDay-curTrain.saleDateBegin].stationNum == -1) {

                                        tmpTrip[startDay-curTrain.saleDateBegin] =
                                                tripList.find(pair<string_t, int>(curTrain.trainID, startDay)).second;
                                    }
                                    auto &curTrip = tmpTrip[startDay-curTrain.saleDateBegin];
                                    queryTransferResult curRes;
                                    curRes.trainID1 = ttmp.trainID;
                                    curRes.trainID2 = curTrain.trainID;
                                    curRes.time = ((startDay - 1) * 1440 + curTrain.startTime + tttt) - (ttmp.startTime + (date - 1) * 1440);
                                    curRes.startTime1 = ttmp.startTime;
                                    curRes.arriveTime1 = ttmp.arriveTime;
                                    curRes.startTime2 = (startDay - 1) * 1440 + curTrain.startTime + totalTime;
                                    curRes.arriveTime2 = (startDay - 1) * 1440 + curTrain.startTime + tttt;
                                    curRes.price = ttmp.price + price;
                                    curRes.price1 = ttmp.price;
                                    curRes.price2 = price;
                                    curRes.transferStation = curTrain.stations[i];
                                    curRes.seat1 = ttmp.seat;
                                    curRes.seat2 = curTrip.restSeat[i];
                                    for (int j = i; j < tt.ordinal; ++j) curRes.seat2 = std::min(curRes.seat2, curTrip.restSeat[j]);

                                    if (!ok || cmpQueryTransferResult(curRes, res, sortPattern)) {
                                        ok = true;
                                        res = curRes;
                                    }
                                }

                                int next = ttmp.nextAddr;
                                if (next == -1) break;
                                ioTrip.read(ttmp, next);
                            }
                        }

                        totalTime -= curTrain.stopoverTimes[i];
                        //if (i > 0) takeTime += curTrain.stopoverTimes[i];
                    }
                    pt = tt.nextAddr;

                }
            }
            */

            if (ok) {
                //std::cout << timeStamp << ' ';
                //std::cout << res.trainID1 << ' ' << startStationID << ' ';
                printf("[%d] %s %s ", timeStamp, res.trainID1.s, startStationID.s);
                printDate(date);
                putchar(' ');
                printMinute(res.startTime1);
                //std::cout << " -> " << res.transferStation << ' ';
                printf(" -> %s ", res.transferStation.s);
                printDateAndMinute(res.arriveTime1);
                //std::cout << ' ' << res.price1 << ' ' << res.seat1 << std::endl;
                printf(" %d %d\n", res.price1, res.seat1);
                //std::cout << res.trainID2 << ' ' << res.transferStation << ' ';
                printf("%s %s ", res.trainID2.s, res.transferStation.s);

                printDateAndMinute(res.startTime2);
                //std::cout << " -> " << toStationID << ' ';
                printf(" -> %s ", toStationID.s);
                printDateAndMinute(res.arriveTime2);
                //std::cout << ' ' << res.price2 << ' ' << res.seat2 << std::endl;
                printf(" %d %d\n", res.price2, res.seat2);
            } else {
                //std::cout << timeStamp << ' ' << 0 << std::endl;
                printf("[%d] 0\n", timeStamp);
            }
#endif
        }
        else if (!strcmp(operationName, "buy_ticket")) {

            string_t curUsername;
            //std::string dateString;
            char dateString[6];
            int date;
            bool isCandidate = false;
            orderOnAccount orderA;
            orderA.timeStamp = timeStamp;
            while (true) {
                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 'u': scanf("%s", curUsername.s); break;
                    case 'i': scanf("%s", orderA.trainID.s); break;
                    case 'd':
                        scanf("%s", dateString);
                        date = calcDay((dateString[0] - '0') * 10 + (dateString[1] - '0'), (dateString[3] - '0') * 10 + (dateString[4] - '0'));
                        break;
                    case 'n': scanf("%d", &orderA.seat); break;
                    case 'f': scanf("%s", orderA.fromStationID.s); break;

                    case 't': scanf("%s", orderA.toStationID.s); break;
                    case 'q':
                        //std::cin >> tmp;
                        scanf("%s", tmpStr);
                        if (!strcmp(tmpStr, "true")) isCandidate = true;
                        break;
                }
            }
#ifndef DISABLE_buy_ticket
            bool ok = false;
            if (loginList.find(curUsername).first && releaseList.find(orderA.trainID).first) {
                Account curUser = userList.find(curUsername).second;
                Train curTrain = trainList.find(orderA.trainID).second;
                if (curTrain.seatNum >= orderA.seat) {
                    int sOrdinal = -1, tOrdinal = -1;
                    int sumTime[100] = {0}, sumPrice[100] = {0};
                    for (int i = 0; i < curTrain.stationNum; ++i) {
                        if (sOrdinal == -1 && curTrain.stations[i] == orderA.fromStationID) sOrdinal = i;
                        if (tOrdinal == -1 && curTrain.stations[i] == orderA.toStationID) tOrdinal = i;
                        if (i > 0)
                            sumTime[i] = sumTime[i - 1] + curTrain.travelTimes[i - 1], sumPrice[i] = sumPrice[i - 1] + curTrain.prices[i - 1];
                        if (i < curTrain.stationNum - 1 && i > 0) sumTime[i] += curTrain.stopoverTimes[i];
                    }


                    //warning: avoid meaningless order.
                    if (sOrdinal != -1 && tOrdinal != -1 && sOrdinal < tOrdinal &&
                        (curTrain.saleDateBegin - 1) * 1440 + curTrain.startTime + sumTime[sOrdinal] < date * 1440 &&
                        (curTrain.saleDateEnd - 1) * 1440 + curTrain.startTime + sumTime[sOrdinal] >=
                        (date - 1) * 1440) {


                        int startDate = curTrain.saleDateBegin +
                                        (((date - 1) * 1440 + (curTrain.startTime + sumTime[sOrdinal]) % 1440) -
                                         ((curTrain.saleDateBegin - 1) * 1440 + curTrain.startTime +
                                          sumTime[sOrdinal])) / 1440;
                        Trip curTrip = tripList.find(pair<string_t, int>(orderA.trainID, startDate)).second;

                        int maxSeat = curTrip.restSeat[sOrdinal];
                        for (int i = sOrdinal; i < tOrdinal; ++i) {
                            maxSeat = std::min(maxSeat, curTrip.restSeat[i]);
                        }

                        orderA.price = sumPrice[tOrdinal] - sumPrice[sOrdinal];
                        orderA.leaveTime = (startDate - 1) * 1440 + curTrain.startTime + sumTime[sOrdinal];
                        orderA.arriveTime = (startDate - 1) * 1440 + curTrain.startTime + sumTime[tOrdinal];
                        if (tOrdinal < curTrain.stationNum - 1) orderA.arriveTime -= curTrain.stopoverTimes[tOrdinal];
                        orderA.sOrdinal = sOrdinal;
                        orderA.tOrdinal = tOrdinal;
                        orderA.startDate = startDate;

                        if (maxSeat >= orderA.seat) {
                            orderA.status = orderOnAccount::success;
                            for (int i = sOrdinal; i < tOrdinal; ++i) {
                                curTrip.restSeat[i] -= orderA.seat;
                            }
                            tripList.modify(pair<string_t, int>(orderA.trainID, startDate), curTrip);

                            /*
                            orderA.nextAddr = curUser.head;
                            curUser.head = ioAccount.newPos();
                            ioAccount.write(orderA, curUser.head);
                            userList.modify(curUsername, curUser);*/
                            userToOrder.insert(pair<string_t, int>(curUsername, timeStamp), orderA);
                            curUser.orderNum++;
                            userList.modify(curUsername, curUser);

                            ok = true;
                            printf("[%d] %lld\n", timeStamp, (long long)orderA.seat * orderA.price);
                        } else if (isCandidate) {
                            orderA.status = orderOnAccount::pending;
/*
                            orderA.nextAddr = curUser.head;
                            curUser.head = ioAccount.newPos();
                            ioAccount.write(orderA, curUser.head);
                            userList.modify(curUsername, curUser);
                            checkYourCode.insert(pair<string_t, int>(curUsername, orderA.timeStamp), curUser.head);
                            orderOnTrip orderT;
                            orderT.timestamp = orderA.timeStamp;
                            orderT.username = curUsername;
                            orderT.need = orderA.seat;
                            orderT.sOrdinal = sOrdinal, orderT.tOrdinal = tOrdinal;
                            orderT.nextAddr = curTrip.head;

                            curTrip.head = ioOrder.newPos();
                            ioOrder.write(orderT, curTrip.head);
                            tripList.modify(pair<string_t, int>(orderA.trainID, startDate), curTrip);
*/
                            userToOrder.insert(pair<string_t, int>(curUsername, timeStamp), orderA);
                            curUser.orderNum++;
                            userList.modify(curUsername, curUser);
                            orderOnTrip orderT;
                            orderT.timestamp = orderA.timeStamp;
                            orderT.username = curUsername;
                            orderT.need = orderA.seat;
                            orderT.sOrdinal = sOrdinal, orderT.tOrdinal = tOrdinal;
                            tripToOrder.insert(pair<pair<string_t, int>, int>(pair<string_t, int>(orderA.trainID, startDate), timeStamp), orderT);

                            ok = true;
                            printf("[%d] queue\n", timeStamp);
                        }
                    }
                }
            }
            if (!ok) {
                printf("[%d] -1\n", timeStamp);
            }
#endif
        }
        else if (!strcmp(operationName, "query_order")) {
            string_t curUsername;
            while (true) {
                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 'u': scanf("%s", curUsername.s); break;
                }
            }
#ifndef DISABLE_query_order
            if (loginList.find(curUsername).first) {
                Account curUser = userList.find(curUsername).second;
                printf("[%d] %d\n", timeStamp, curUser.orderNum);
                if (curUser.orderNum > 0) {
                    BPT<pair<string_t, int>, orderOnAccount>::iterator it;
                    userToOrder.lower_bound(it, pair<string_t, int>(curUsername, 0));
                    query_order(it, curUsername);
                }
            } else {
                printf("[%d] -1\n", timeStamp);
            }
#endif
        }
        else if (!strcmp(operationName, "refund_ticket")) {
            string_t curUsername;
            int num = 1;
            while (true) {
                scanf("%s", _key);
                if (_key[0] == '[') break;
                switch (_key[1]) {
                    case 'u': scanf("%s", curUsername.s); break;
                    case 'n': scanf("%d", &num); break;
                }
            }

            bool ok = false;
            if (loginList.find(curUsername).first) {
                Account curUser = userList.find(curUsername).second;
                if (curUser.orderNum >= num) {
                    num = curUser.orderNum - num + 1;
                    BPT<pair<string_t, int>, orderOnAccount>::iterator itA;
                    userToOrder.lower_bound(itA, pair<string_t, int>(curUsername, 0));
                    while (--num) itA.plusplus();
                    orderOnAccount orderA;
                    itA.getSecond(orderA);
                    if (orderA.status == orderOnAccount::success) {
                        orderA.status = orderOnAccount::refunded;
                        itA.modify(orderA);
                        Trip curTrip = tripList.find(pair<string_t, int>(orderA.trainID, orderA.startDate)).second;
                        for (int i = orderA.sOrdinal; i < orderA.tOrdinal; ++i)
                            curTrip.restSeat[i] += orderA.seat;
                        BPT<pair<pair<string_t, int>, int>, orderOnTrip>::iterator itT;
                        if(tripToOrder.lower_bound(itT, pair<pair<string_t, int>, int>(pair<string_t, int>(orderA.trainID, orderA.startDate), 0))) {
                            orderOnTrip orderT;
                            pair<pair<string_t, int>, int> tmp;
                            while (true) {
                                itT.getFirst(tmp);
                                if (tmp.first.first != orderA.trainID || tmp.first.second != orderA.startDate) break;
                                itT.getSecond(orderT);
                                int maxSeat = curTrip.restSeat[orderT.sOrdinal];
                                for (int i = orderT.sOrdinal; i < orderT.tOrdinal; ++i) {
                                    maxSeat = std::min(maxSeat, curTrip.restSeat[i]);
                                }
                                if (maxSeat >= orderT.need) {
                                    for (int i = orderT.sOrdinal; i < orderT.tOrdinal; ++i) {
                                        curTrip.restSeat[i] -= orderT.need;
                                    }
                                    satisfiyOrder(orderT.username, tmp.second);
                                    tripToOrder.erase(tmp);
                                    if (!tripToOrder.lower_bound(itT, tmp)) break;
                                } else {
                                    if (!itT.plusplus()) break;
                                    //it.getFirst(tmp);
                                    //if (tmp.first != pair<string_t, int>(orderA.trainID, orderA.startDate))
                                    //if (tmp.first.first != orderA.trainID || tmp.first.second != orderA.startDate) break;
                                }
                            }
                        }
                        tripList.modify(pair<string_t, int>(orderA.trainID, orderA.startDate), curTrip);
                        ok = true;
                        printf("[%d] 0\n", timeStamp);

                    } else if (orderA.status == orderOnAccount::pending) {
                        orderA.status = orderOnAccount::refunded;
                        itA.modify(orderA);
                        BPT<pair<pair<string_t, int>, int>, orderOnTrip>::iterator itT;
                        tripToOrder.lower_bound(itT, pair<pair<string_t, int>, int>(pair<string_t, int>(orderA.trainID, orderA.startDate), 0));
                        pair<pair<string_t, int>, int> tmp;
                        while (true) {
                            itT.getFirst(tmp);
                            if (tmp.second == orderA.timeStamp) {
                                tripToOrder.erase(tmp);
                                break;
                            }
                            itT.plusplus();
                        }
                        ok = true;
                        printf("[%d] 0\n", timeStamp);
                    }
                }
            }
            if (!ok) {
                printf("[%d] -1\n", timeStamp);
            }
        }
        else if (!strcmp(operationName, "clean")) {
            userList.clear();
            loginList.clear();
            trainList.clear();
            releaseList.clear();
            //stationList.clear();
            //ioStation.clear();
            tripList.clear();
            //ioOrder.clear();
            //ioTrip.clear();
            //stationTfList.clear();
            //ioAccount.clear();
            checkYourCode.clear();

            userToOrder.clear();
            tripToOrder.clear();
            stationToTrain.clear();
            printf("[%d] 0\n", timeStamp);
            scanf("%s", _key);
        }
        else if (!strcmp(operationName, "exit")) {
            loginList.clear();
            printf("[%d] bye\n", timeStamp);
            return 0;
        }
    }
}