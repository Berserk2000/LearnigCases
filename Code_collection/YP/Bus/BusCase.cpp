#include <cassert>
#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <vector>

using namespace std;

enum class QueryType {
    NewBus,
    BusesForStop,
    StopsForBus,
    AllBuses,
};

struct Query {
    QueryType type;
    string bus;
    string stop;
    vector<string> stops;
};

istream& operator>>(istream& is, Query& q) {
    string type;
    is >> type;
    if(type == "NEW_BUS"s){
        q.type = QueryType::NewBus;
        is >> q.bus;
        int amount;
        is >> amount;
        q.stops.resize(amount);
        
        for(auto& i : q.stops){
            is >> i;
        }
    }else if(type == "BUSES_FOR_STOP"s){
        q.type = QueryType::BusesForStop;
        is >> q.stop;
    }else if(type == "STOPS_FOR_BUS"){
        q.type = QueryType::StopsForBus;
        is >> q.bus;
    }else if(type == "ALL_BUSES"){
        q.type = QueryType::AllBuses;
    }
    return is;
}

// Структура выдачи списка автобусов через остановку.
struct BusesForStopResponse {
    vector<string> buses;
};

ostream& operator<<(ostream& os, const BusesForStopResponse& r) {
    bool first=1;
    for(const auto& bus : r.buses){
        if(!first){
            os << ' ';
        }else{
            first = 0 ;
        }
        os << bus;
    }
    return os;
}

// Структура выдачи описания остановок для маршрута автобуса.
struct StopsForBusResponse {
    bool bus_exist = 1;
    vector<pair<string,vector<string>>> stops_interchanges;
};

ostream& operator<<(ostream& os, const StopsForBusResponse& r) {
    if(!r.bus_exist){
        os << "No bus"s;
        return os;
    }
    auto last = r.stops_interchanges.end();
    for(auto stop_iter = r.stops_interchanges.begin(); stop_iter != last;++stop_iter){
        os << "Stop " << stop_iter->first << ": ";
        bool first = 1;
        for(const auto& bus: stop_iter->second){
            if(!first){
                os << " ";
            }else{
                first = 0;
            }
            os << bus;
        }
        if(stop_iter != prev(last)){
              os << "\n";
        }
    }
    return os;
}

// Структура выдачи описания всех автобусов.
struct AllBusesResponse {
    bool bus_exist = 1;
    map<string,vector<string>> buses_to_stops;
};

ostream& operator<<(ostream& os, const AllBusesResponse& r) {
    if(!r.bus_exist){
        os << "No buses"s;
        return os;
    }
    auto last = r.buses_to_stops.end();
    for(auto buses_iter = r.buses_to_stops.begin(); buses_iter != last;++buses_iter){
        os << "Bus " << buses_iter->first << ": ";
        bool first = 1;
        for(const auto& stop: buses_iter->second){
            if(!first){
                os << " ";
            }else{
                first = 0;
            }
            os << stop;
        }
        if(buses_iter != prev(last)){
              os << "\n";
        }
      
    }
    return os;
}

class BusManager {
private:
map<string, vector<string>> buses_to_stops_;
map<string, vector<string>> stops_to_buses_;
public:
    void AddBus(const string& bus, const vector<string>& stops) {
        buses_to_stops_[bus] = stops;
        for(const auto& stop : stops){
            stops_to_buses_[stop].push_back(bus);
        }
    }

    BusesForStopResponse GetBusesForStop(const string& stop) const {
        BusesForStopResponse r;
        if(stops_to_buses_.count(stop)){
            r.buses = stops_to_buses_.at(stop);
        }else{
            r.buses.push_back("No stop"s);
        }
        return r;
    }

    StopsForBusResponse GetStopsForBus(const string& bus) const {
        StopsForBusResponse r;
        if(!buses_to_stops_.count(bus)){
            r.bus_exist = 0;
            return r;
        }
        int i = 0;
        r.stops_interchanges.resize(buses_to_stops_.at(bus).size());
        for (const string& stop : buses_to_stops_.at(bus)) {
            r.stops_interchanges[i].first = stop;
            if(stops_to_buses_.at(stop).size() == 1){
                r.stops_interchanges[i].second.push_back("no interchange"s);
            }else{
                for(const auto& bus_inter : stops_to_buses_.at(stop)){
                    if(bus_inter != bus){
                        r.stops_interchanges[i].second.push_back(bus_inter);
                    }
                }
            }
            ++i;
        }
        return r;
    }

    AllBusesResponse GetAllBuses() const {
        AllBusesResponse r;
        if (buses_to_stops_.empty()) {
            r.bus_exist = 0;
        } else {
            r.buses_to_stops = buses_to_stops_;
        }
        return r;
    }

    // const  map<string, vector<string>> GetBusMap() const {
    //     return buses_to_stops_;
    // }
    // const map<string, vector<string>> GetStopMap() const {
    //     return stops_to_buses_;
    // }
};


int main() {
   // TestBusManager();
    int query_count;
    Query q;

    cin >> query_count;

    BusManager bm;
    for (int i = 0; i < query_count; ++i) {
        cin >> q;
        switch (q.type) {
            case QueryType::NewBus:
                bm.AddBus(q.bus, q.stops);
                break;
            case QueryType::BusesForStop:
                cout << bm.GetBusesForStop(q.stop) << endl;
                break;
            case QueryType::StopsForBus:
                cout << bm.GetStopsForBus(q.bus) << endl;
                break;
            case QueryType::AllBuses:
                cout << bm.GetAllBuses() << endl;
                break;
        }
    }
}

// void TestQueryIn(){
//     Query q;
//     istringstream input1("NEW_BUS 32 3 Tolstopaltsevo Marushkino Vnukovo"s);
//     input1 >> q;
//     assert(q.type == QueryType::NewBus && q.bus == "32"s && (q.stops == vector<string>{"Tolstopaltsevo"s,"Marushkino"s,"Vnukovo"s}));

//     istringstream input2("ALL_BUSES"s);
//     input2 >> q;
//     assert(q.type == QueryType::AllBuses);

//     istringstream input3("BUSES_FOR_STOP Vnukovo"s);
//     input3 >> q;
//     assert(q.type == QueryType::BusesForStop && q.stop == "Vnukovo"s );

//     istringstream input4("STOPS_FOR_BUS 32K"s);
//     input4 >> q;
//     assert(q.type == QueryType::StopsForBus && q.bus == "32K"s);
// }
// void TestAddBus(){
//     BusManager bm;
//     Query q;

//     istringstream input1("NEW_BUS 32 3 Tolstopaltsevo Marushkino Vnukovo"s);
//     input1 >> q;
//     bm.AddBus(q.bus,q.stops);
//     assert(bm.GetStopMap().at("Tolstopaltsevo"s) == vector<string>{"32"s});
//     assert(bm.GetBusMap().at("32"s) == vector<string>({"Tolstopaltsevo"s,"Marushkino"s,"Vnukovo"s}));
//     assert(bm.GetBusMap().count("32K"s) == 0);

//     istringstream input2("NEW_BUS 32K 6 Tolstopaltsevo Marushkino Vnukovo Peredelkino Solntsevo Skolkovo"s);
//     input2 >> q;
//     bm.AddBus(q.bus,q.stops);
//     assert(bm.GetStopMap().at("Tolstopaltsevo"s) == vector<string>({"32"s,"32K"s}));
//     assert(bm.GetBusMap().at("32"s) == vector<string>({"Tolstopaltsevo"s,"Marushkino"s,"Vnukovo"s}));
//     assert(bm.GetBusMap().count("32K"s) == 1);

// }
// void TestBusesForStop(){
//     BusManager bm;
//     Query q;

//     istringstream input0("BUSES_FOR_STOP Tolstopaltsevo"s);
//     input0 >> q;
//     ostringstream os0;
//     os0 << bm.GetBusesForStop(q.stop);
//     assert(os0.str() == "No stop"s);

//     istringstream input1("NEW_BUS 32 3 Tolstopaltsevo Marushkino Vnukovo"s);
//     input1 >> q;
//     bm.AddBus(q.bus,q.stops);

//     istringstream input2("BUSES_FOR_STOP Tolstopaltsevo"s);
//     input2 >> q;
//     ostringstream os1;
//     os1 << bm.GetBusesForStop(q.stop);
//     assert(os1.str() == "32");

//     istringstream input3("NEW_BUS 32K 6 Tolstopaltsevo Marushkino Vnukovo Peredelkino Solntsevo Skolkovo"s);
//     input3 >> q;
//     bm.AddBus(q.bus,q.stops);

//     istringstream input4("BUSES_FOR_STOP Tolstopaltsevo"s);
//     input4 >> q;
//     ostringstream os2;
//     os2 << bm.GetBusesForStop(q.stop);
//     assert(os2.str() == "32 32K");

// }
// void TestStopsForBus(){
//     BusManager bm;
//     Query q;

//     istringstream input0("STOPS_FOR_BUS 32"s);
//     input0 >> q;
//     ostringstream os0;
//     os0 << bm.GetStopsForBus(q.bus);
//     assert(os0.str() == "No bus"s);

//     istringstream input1("NEW_BUS 32 3 Tolstopaltsevo Marushkino Vnukovo"s);
//     input1 >> q;
//     bm.AddBus(q.bus,q.stops);

//     istringstream input2("NEW_BUS 32K 6 Tolstopaltsevo Marushkino Vnukovo Peredelkino Solntsevo Skolkovo"s);
//     input2 >> q;
//     bm.AddBus(q.bus,q.stops);

//     stringstream input3("NEW_BUS 950 6 Kokoshkino Marushkino Vnukovo Peredelkino Solntsevo Troparyovo"s);
//     input3 >> q;
//     bm.AddBus(q.bus,q.stops);

//     istringstream input4("NEW_BUS 272 4 Vnukovo Moskovsky Rumyantsevo Troparyovo"s);
//     input4 >> q;
//     bm.AddBus(q.bus,q.stops);

//     istringstream input5("STOPS_FOR_BUS 272"s);
//     input5 >> q;
//     ostringstream os2;
//     ostringstream osCtrl;
//     osCtrl << "Stop Vnukovo: 32 32K 950\n"s
//            << "Stop Moskovsky: no interchange\n"s
//            << "Stop Rumyantsevo: no interchange\n"s
//            << "Stop Troparyovo: 950\n"s;
//     os2 << bm.GetStopsForBus(q.bus);
//     assert(os2.str() == osCtrl.str());
// }
// void TestAllBuses(){
//     BusManager bm;
//     Query q;

//     istringstream input0("ALL_BUSES"s);
//     input0 >> q;
//     ostringstream os0;
//     os0 << bm.GetAllBuses();
//     assert(os0.str() == "No buses"s);

//     istringstream input1("NEW_BUS 32 3 Tolstopaltsevo Marushkino Vnukovo"s);
//     input1 >> q;
//     bm.AddBus(q.bus,q.stops);

//     istringstream input2("NEW_BUS 32K 6 Tolstopaltsevo Marushkino Vnukovo Peredelkino Solntsevo Skolkovo"s);
//     input2 >> q;
//     bm.AddBus(q.bus,q.stops);

//     stringstream input3("NEW_BUS 950 6 Kokoshkino Marushkino Vnukovo Peredelkino Solntsevo Troparyovo"s);
//     input3 >> q;
//     bm.AddBus(q.bus,q.stops);

//     istringstream input4("NEW_BUS 272 4 Vnukovo Moskovsky Rumyantsevo Troparyovo"s);
//     input4 >> q;
//     bm.AddBus(q.bus,q.stops);

//     istringstream input5("ALL_BUSES"s);
//     input5 >> q;
//     ostringstream os2;
//     ostringstream osCtrl;
//     osCtrl << "Bus 272: Vnukovo Moskovsky Rumyantsevo Troparyovo\n"s
//            << "Bus 32: Tolstopaltsevo Marushkino Vnukovo\n"s
//            << "Bus 32K: Tolstopaltsevo Marushkino Vnukovo Peredelkino Solntsevo Skolkovo\n"s
//            << "Bus 950: Kokoshkino Marushkino Vnukovo Peredelkino Solntsevo Troparyovo\n"s;
//     os2 << bm.GetAllBuses();
//     assert(os2.str() == osCtrl.str());
// }
// void TestBusManager(){
//     TestQueryIn();
//     TestAddBus();
//     TestBusesForStop();
//     TestStopsForBus();
//     TestAllBuses();
// }

// #include <iostream>
// #include <map>
// #include <string>
// #include <vector>

// using namespace std;

// int main() {
//     int q;
//     cin >> q;

//     map<string, vector<string>> buses_to_stops, stops_to_buses;

//     for (int i = 0; i < q; ++i) {
//         string operation_code;
//         cin >> operation_code;

//         if (operation_code == "NEW_BUS"s) {
//             string bus;
//             cin >> bus;
//             int stop_count;
//             cin >> stop_count;
//             vector<string>& stops = buses_to_stops[bus];
//             stops.resize(stop_count);
//             for (string& stop : stops) {
//                 cin >> stop;
//                 stops_to_buses[stop].push_back(bus);
//             }

//         } else if (operation_code == "BUSES_FOR_STOP"s) {
//             string stop;
//             cin >> stop;
//             if (stops_to_buses.count(stop) == 0) {
//                 cout << "No stop"s << endl;
//             } else {
//                 bool is_first = true;
//                 for (const string& bus : stops_to_buses[stop]) {
//                     if (!is_first) {
//                         cout << " "s;
//                     }
//                     is_first = false;
//                     cout << bus;
//                 }
//                 cout << endl;
//             }

//         } else if (operation_code == "STOPS_FOR_BUS"s) {
//             string bus;
//             cin >> bus;
//             if (buses_to_stops.count(bus) == 0) {
//                 cout << "No bus"s << endl;
//             } else {
//                 for (const string& stop : buses_to_stops[bus]) {
//                     cout << "Stop "s << stop << ":"s;
//                     if (stops_to_buses[stop].size() == 1) {
//                         cout << " no interchange"s;
//                     } else {
//                         for (const string& other_bus : stops_to_buses[stop]) {
//                             if (bus != other_bus) {
//                                 cout << " "s << other_bus;
//                             }
//                         }
//                     }
//                     cout << endl;
//                 }
//             }

//         } else if (operation_code == "ALL_BUSES"s) {
//             if (buses_to_stops.empty()) {
//                 cout << "No buses"s << endl;
//             } else {
//                 for (const auto& bus_item : buses_to_stops) {
//                     cout << "Bus "s << bus_item.first << ":"s;
//                     for (const string& stop : bus_item.second) {
//                         cout << " "s << stop;
//                     }
//                     cout << endl;
//                 }
//             }
//         }
//     }
// }