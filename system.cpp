#include <Ice/Ice.h>
#include "MPK.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <string>

using namespace std;
using namespace SIP;

class MPK_I : public SIP::MPK {
private:
    LineList all_lines;
    StopList all_stops;
    DepoList all_depos;
    vector <std::shared_ptr<LineFactoryPrx>> lineFactories;
    vector <std::shared_ptr<StopFactoryPrx>> stopFactories;
public:
    LineList getLines(const Ice::Current &current) override {
        return all_lines;
    };

    void addStop(shared_ptr <TramStopPrx> tramStop) {
        StopInfo stopInfo;
        stopInfo.stop = tramStop;
        all_stops.push_back(stopInfo);
    }

    void addLine(shared_ptr <LinePrx> line, const Ice::Current &current) override {
        all_lines.push_back(line);
    }

    void registerDepo(::std::shared_ptr <DepoPrx> depo, const Ice::Current &current) override {
        cout << "Nowa zajezdnia o nazwie: " << depo->getName() << endl;
        DepoInfo depoInfo;
        depoInfo.stop = depo;
        depoInfo.name = depo->getName();
        all_depos.push_back(depoInfo);
    }

    void unregisterDepo(::std::shared_ptr <DepoPrx> depo, const Ice::Current &current) override {
        for (int index = 0; index < all_depos.size(); index++) {
            if (all_depos.at(index).stop->getName() == depo->getName()) {
                cout << "Usuwam zajezdnie o nazwie: " << depo->getName() << endl;
                all_depos.erase(all_depos.begin() + index);
                break;
            }
        }
    };

    shared_ptr <TramStopPrx> getTramStop(string name, const Ice::Current &current) override {
        for (int i = 0; i < all_stops.size(); ++i) {
            if (all_stops.at(i).stop->getName() == name) {
                return all_stops.at(i).stop;
            }
        }
        return NULL;
    }

    shared_ptr <DepoPrx> getDepo(string name, const Ice::Current &current) override {
        for (int i = 0; i < all_depos.size(); ++i) {
            if (all_depos.at(i).stop->getName() == name) {
                return all_depos.at(i).stop;
            }
        }
        return NULL;
    }

    DepoList getDepos(const Ice::Current &current) override {
        return all_depos;
    }

    void registerLineFactory(std::shared_ptr <SIP::LineFactoryPrx> lf, const Ice::Current &current) override {
        // Sprawdzenie, czy fabryka już jest zarejestrowana
        if (std::find(lineFactories.begin(), lineFactories.end(), lf) == lineFactories.end()) {
            lineFactories.push_back(lf);
            std::cout << "Fabryka linii zarejestrowana." << std::endl;
        }
    }

    void unregisterLineFactory(std::shared_ptr <SIP::LineFactoryPrx> lf, const Ice::Current &current) override {
        auto it = std::find(lineFactories.begin(), lineFactories.end(), lf);
        if (it != lineFactories.end()) {
            lineFactories.erase(it);
            std::cout << "LineFactory unregistered." << std::endl;
        }
    }

    void registerStopFactory(std::shared_ptr <SIP::StopFactoryPrx> lf, const Ice::Current &current) override {
        // Sprawdzenie, czy fabryka już jest zarejestrowana
        if (std::find(stopFactories.begin(), stopFactories.end(), lf) == stopFactories.end()) {
            stopFactories.push_back(lf);
            std::cout << "StopFactory registered." << std::endl;
        }
    }

    void unregisterStopFactory(std::shared_ptr <SIP::StopFactoryPrx> lf, const Ice::Current &current) override {
        auto it = std::find(stopFactories.begin(), stopFactories.end(), lf);
        if (it != stopFactories.end()) {
            stopFactories.erase(it);
            std::cout << "StopFactory unregistered." << std::endl;
        }
    }

};

class TramStopI : public SIP::TramStop {
private:
    string name;
    LineList lines;
    vector <shared_ptr<PassengerPrx>> passengers;
    TramList coming_trams;
    TramList currentTrams;
public:
    TramStopI(string name) {
        this->name = name;
    }

    void addLine(::std::shared_ptr <LinePrx> line) {
        lines.push_back(line);
    }

    string getName(const Ice::Current &current) override {
        return name;
    };

    TramList getNextTrams(int howMany, const Ice::Current &current) override {
        TramList nextTrams;
        for (int i = 0; i < howMany; ++i) {
            if (i < coming_trams.size()) {
                nextTrams.push_back(coming_trams.at(i));
            }
        }
        return nextTrams;
//            TramList nextTrams;
//            for(int i = 0; i < lines.size(); ++i){
//
//                TramList tramList = lines.at(i)->getTrams();
//                int numberOfAllStops = lines.at(i)->getStops().size();
//                for(int j = 0; j < tramList.size(); ++j){
//
//                    for(int k = 1; k < numberOfAllStops; ++k){
//
//                        StopList stopList = tramList.at(j).tram->getNextStops(k);
//                        for(int l = 0; l < stopList.size(); ++l){
//
//                            if(stopList.at(l).stop->getName() == name){
//                                TramInfo tramInfo;
//                                tramInfo.tram = tramList.at(j).tram;
//                                nextTrams.push_back(tramInfo);
//                            }
//                        }
//                    }
//                }
//            }
//            TramList resultTramList;
//            for(int i = 0; i < howMany; i++){
//                resultTramList.push_back(nextTrams.at(i));
//            }
//            return resultTramList;
    };

    void RegisterPassenger(::std::shared_ptr <PassengerPrx> passenger, const Ice::Current &current) override {
        passengers.push_back(passenger);
        cout << "Pasazer zasubskrybowal przystanek: " << name << endl;
//            for(int lineIndex = 0; lineIndex < lines.size(); lineIndex++){
//                shared_ptr<LinePrx> line = lines.at(lineIndex);
//                TramList trams = line->getTrams();
//                for(int tramIndex = 0; tramIndex < trams.size(); tramIndex++){
//                    shared_ptr<TramPrx> tram = trams.at(tramIndex).tram;
//                    tram->RegisterPassenger(passenger);
//                }
//            }
    };

    void UnregisterPassenger(::std::shared_ptr <PassengerPrx> passenger, const Ice::Current &current) override {
        for (int index = 0; index < passengers.size(); index++) {
            if (passengers.at(index)->ice_getIdentity() == passenger->ice_getIdentity()) {
                passengers.erase(passengers.begin() + index);
                cout << "Pasazer odsubskrybowal przystanek: " << name << endl;
//                    for(int lineIndex = 0; lineIndex < lines.size(); lineIndex++){
//                        shared_ptr<LinePrx> line = lines.at(lineIndex);
//                        TramList trams = line->getTrams();
//                        for(int tramIndex = 0; tramIndex < trams.size(); tramIndex++){
//                            shared_ptr<TramPrx> tram = trams.at(tramIndex).tram;
//                            tram->UnregisterPassenger(passenger);
//                        }
//                    }
                break;
            }
        }
    };

    void UpdateTramInfo(std::shared_ptr <SIP::TramPrx> tram, SIP::Time time, const Ice::Current &current) override {
        TramInfo tramInfo;
        tramInfo.tram = tram;
        tramInfo.time = time;

        for (int i = 0; i < coming_trams.size(); ++i) {
            if (coming_trams.at(i).time.hour < time.hour) {
                coming_trams.insert(coming_trams.begin() + i, tramInfo);
                return;
            } else if (coming_trams.at(i).time.hour == time.hour) {
                if (coming_trams.at(i).time.minute < time.minute) {
                    coming_trams.insert(coming_trams.begin() + i, tramInfo);
                    return;
                }
            }
        }
        coming_trams.push_back(tramInfo);
    };

    void addCurrentTram(shared_ptr <SIP::TramPrx> tram, const Ice::Current &current) override {
        TramInfo tramInfo;
        tramInfo.tram = tram;
        currentTrams.push_back(tramInfo);
        string header = "Tramwaje na przystanku " + name;
        for (const auto &passenger: passengers) {
            passenger->notifyPassenger(header, Ice::Context());
        }
        for (auto it = currentTrams.begin(); it != currentTrams.end(); ++it) {
            string info = it->tram->getStockNumber();
            for (const auto &passenger: passengers) {
                passenger->notifyPassenger(info, Ice::Context());
            }

        }

    }

    void removeCurrentTram(shared_ptr <SIP::TramPrx> tram, const Ice::Current &current) override {
        for (auto it = currentTrams.begin(); it != currentTrams.end(); ++it) {
            if (it->tram->ice_getIdentity() == tram->ice_getIdentity()) {
                currentTrams.erase(it);
                break;
            }
        }
    }

};

class LineI : public SIP::Line {
private:
    TramList all_trams;
    StopList all_stops;
    string name;
public:
    LineI(string name) {
        this->name = name;
    }

    TramList getTrams(const Ice::Current &current) override {
        return all_trams;
    };

    SIP::StopList getStops(const Ice::Current &current) override {
        return all_stops;
    };

    string getName(const Ice::Current &current) override {
        return name;
    };

    void registerTram(shared_ptr <TramPrx> tram, const Ice::Current &current) override {
        TramInfo tramInfo;
        tramInfo.tram = tram;

        all_trams.push_back(tramInfo);

        cout << "Nowy tramwaj o numerze: " << tram->getStockNumber() << " zostal dodany"
             << endl;
    };

    void unregisterTram(::std::shared_ptr <TramPrx> tram, const Ice::Current &current) override {
        for (int i = 0; i < all_trams.size(); ++i) {
            if (all_trams.at(i).tram->getStockNumber() == tram->getStockNumber()) {
                cout << "Zjezdza z lini tramwaj o numerze: " << tram->getStockNumber() << endl;
                cout << "Oczekiwanie na offline" << tram->getStockNumber() << endl;
                all_trams.erase(all_trams.begin() + i);
                break;
            }
        }
    };

    void setStops(SIP::StopList sl, const Ice::Current &current) override {
        all_stops = sl;
    }

};

class DepoI : public SIP::Depo {
private:
    string name;
    TramList all_trams;
public:
    DepoI(string name) {
        this->name = name;
    }

    void TramOnline(::std::shared_ptr <TramPrx> tram, const Ice::Current &current) override {
        if (tram) {
            tram->setStatus(SIP::TramStatus::ONLINE, Ice::Context());
            cout << "Tramwaj " << tram->getStockNumber() << " wyjechal z zajezdni" << endl;
        } else {
            cout << "Dany Tramwaj nie istnieje" << endl;
        }
    }

    void TramOffline(::std::shared_ptr <TramPrx> tram, const Ice::Current &current) override {
        if (tram) {
            tram->setStatus(SIP::TramStatus::OFFLINE, Ice::Context());
            cout << "Tramwaj " << tram->getStockNumber() << " zjechal do zajezdni" << endl;
        } else {
            cout << "Dany Tramwaj nie istnieje" << endl;
        }
    }

    string getName(const Ice::Current &current) override {
        return name;
    }

    void registerTram(::std::shared_ptr <TramPrx> tram, const Ice::Current &current) override {
        TramInfo tramInfo;
        tramInfo.tram = tram;
        tramInfo.tram->setStatus(SIP::TramStatus::WAITONLINE, Ice::Context());
        all_trams.push_back(tramInfo);
        cout << "Zajezdnia zarejestrowala tramwaj o numerze: " << tram->getStockNumber() << endl;
    };

    void unregisterTram(::std::shared_ptr <TramPrx> tram, const Ice::Current &current) override {
        TramInfo tramInfo;
        tramInfo.tram = tram;
        tramInfo.tram->setStatus(SIP::TramStatus::WAITOFFLINE, Ice::Context());
    };

    TramList getTrams(const Ice::Current &current) override {
        return all_trams;
    };
};

class LineFactoryI : public SIP::LineFactory {
private:
    int linesCreated = 0;
    Ice::ObjectAdapterPtr adapter;
public:
    LineFactoryI(Ice::ObjectAdapterPtr adapter) : adapter(adapter) {}

    std::shared_ptr <SIP::LinePrx> createLine(string name, const Ice::Current &current) override {
        auto newLine = make_shared<LineI>(name);
        linesCreated++;

        auto linePrx = Ice::uncheckedCast<SIP::LinePrx>(adapter->addWithUUID(newLine));


        return linePrx;
    }

    double getLoad(const Ice::Current &current = Ice::Current()) override {
        return static_cast<double>(linesCreated);
    }
};

class StopFactoryI : public SIP::StopFactory {
private:
    int stopsCreated = 0;
    Ice::ObjectAdapterPtr adapter;
public:
    StopFactoryI(Ice::ObjectAdapterPtr adapter) : adapter(adapter) {}

    std::shared_ptr <SIP::TramStopPrx> createStop(string name, const Ice::Current &current) override {
        auto newStop = make_shared<TramStopI>(name);
        stopsCreated++;

        auto stopPrx = Ice::uncheckedCast<SIP::TramStopPrx>(adapter->addWithUUID(newStop));

        return stopPrx;
    }

    double getLoad(const Ice::Current &current = Ice::Current()) override {
        return static_cast<double>(stopsCreated);
    }
};


int main(int argc, char *argv[]) {
    Ice::CommunicatorPtr ic;
    try {

        //tworze instancje obiektu ice
        ic = Ice::initialize(argc, argv);
        Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("MPKAdapter", "default -p 10000");

        //tworze servant mpk
        auto mpk = make_shared<MPK_I>();
        adapter->add(mpk, Ice::stringToIdentity("mpk"));

        //Wczytuje zajezdnie
//        ifstream depos_file("depos.txt");
//        if (!depos_file.is_open()) {
//            cerr << "Nie można otworzyć pliku." << endl;
//            throw "File error";
//        }

        //string depo_name;

        //while (depos_file >> depo_name) {
        auto depo = make_shared<DepoI>("Zajezdnia1");
        auto depoPrx = Ice::uncheckedCast<DepoPrx>(adapter->addWithUUID(depo));
        mpk->registerDepo(depoPrx, Ice::Current());
        //}

        auto lineFactory = make_shared<LineFactoryI>(adapter);
        auto lineFactoryPrx = Ice::uncheckedCast<LineFactoryPrx>(adapter->addWithUUID(lineFactory));

        mpk->registerLineFactory(lineFactoryPrx, Ice::Current());

        auto stopFactory = make_shared<StopFactoryI>(adapter);
        auto stopFactoryPrx = Ice::uncheckedCast<StopFactoryPrx>(adapter->addWithUUID(stopFactory));

        mpk->registerStopFactory(stopFactoryPrx, Ice::Current());

        //wczytuje przystanki
        ifstream stops_file("stops.txt");

        if (!stops_file.is_open()) {
            cerr << "Nie można otworzyć pliku." << endl;
            throw "File error";
        }

        string stop_name;
        while (stops_file >> stop_name) {
            auto tramStopPrx = stopFactory->createStop(stop_name, Ice::Current());
            mpk->addStop(tramStopPrx);
        }
//
//        ifstream stops_file2("stops2.txt");
//
//        if (!stops_file2.is_open()) {
//            cerr << "Nie można otworzyć pliku." << endl;
//            throw "File error";
//        }
//
//        while (stops_file2 >> stop_name) {
//            if(!mpk->getTramStop(stop_name)){
//                auto tramStopPrx = stopFactory->createStop(stop_name, Ice::Current());
//                mpk->addStop(tramStopPrx);
//            }
//        }

        ifstream lines_file("lines.txt");
        if (!lines_file.is_open()) {
            cerr << "Nie można otworzyć pliku." << endl;
            throw "File error";
        }

        string file_line;


        cout << "Dostepne linie i przystanki: " << endl;
        while (getline(lines_file, file_line)) {

            //pozyskuje numer linii
            size_t separator_position = file_line.find(':');
            string line_number = file_line.substr(0, separator_position);
            cout << "Linia nr: " << line_number << endl;

            //tworze obiekt linii
            auto linePrx = lineFactory->createLine(line_number, Ice::Current());



            //pozyskuje id przystankow, szukam przystankow i dodaje do linii
            string tramStopsNames = file_line.substr(separator_position + 1);

            istringstream iss(tramStopsNames);
            string stop_name;


            time_t currentTime;
            time(&currentTime);
            tm *timeNow = localtime(&currentTime);

            StopList stopList;
            while (iss >> stop_name) {
                auto tramStopPrx = stopFactory->createStop(stop_name, Ice::Current());
                StopInfo stopInfo;
                stopInfo.time.hour = timeNow->tm_hour;
                stopInfo.time.minute = timeNow->tm_min;
                stopInfo.stop = tramStopPrx;
                stopList.push_back(stopInfo);
            }

            linePrx->setStops(stopList);

            if (linePrx != ICE_NULLPTR) {
                mpk->addLine(linePrx, Ice::Current());
            }

            istringstream ss(tramStopsNames);
            string name;

            cout << "\t przystanki: ";
            while (ss >> name) {
                shared_ptr <TramStopPrx> tramStopPrx = mpk->getTramStop(name, Ice::Current());
                if (!tramStopPrx) {
                    cout << "Brak przystankow";
                } else {
                    cout << tramStopPrx->getName() << " ";
                }
            }
            cout << endl;
        }
        //aktywuje nasluchiwanie
        adapter->activate();
        while (true) {
            cout << "Kliknij d - aby wyswietlic depo" << endl;
            char sign;
            cin >> sign;
            if (sign == 'd') {
                cout << "Zajezdnia: " << depoPrx->getName() << endl;
                DepoList depoList = mpk->getDepos(Ice::Current());
                for (int i = 0; i < depoList.size(); ++i) {
                    cout << "\t" << depoList.at(i).stop->getName() << endl;
                }
                cout << "Zarejestrowane tramwaje: " << endl;
                TramList tramList = depoPrx->getTrams(Ice::Context());
                for (int i = 0; i < tramList.size(); ++i) {
                    cout << i << ". " << tramList.at(i).tram->getStockNumber() << " - ";
                    if (tramList.at(i).tram->getStatus(Ice::Context()) == SIP::TramStatus::ONLINE) {
                        cout << "driving" << endl;
                    } else if (tramList.at(i).tram->getStatus(Ice::Context()) == SIP::TramStatus::OFFLINE) {
                        cout << "offline" << endl;
                    } else if (tramList.at(i).tram->getStatus(Ice::Context()) == SIP::TramStatus::WAITONLINE) {
                        cout << "waiting to online" << endl;
                    } else if (tramList.at(i).tram->getStatus(Ice::Context()) == SIP::TramStatus::WAITOFFLINE) {
                        cout << "waiting to offline" << endl;
                    } else {
                        cout << "unknown" << endl;
                    }
                }
                cout << "Wpisz '<numer> ONLINE' lub '<numer> OFFLINE', lub 'q' aby wyjsc z depo: " << endl;
                string command;
                cin.ignore(); // czyści bufor po wcześniejszym cin >> sign
                getline(cin, command);

                if (command == "q") {
                    cout << "Zajezdnia zamyka sie" << endl;
                    break;
                }

                istringstream iss(command);
                int number;
                string action;
                iss >> number >> action;

                if (number < 0 || number >= tramList.size()) {
                    cout << "Nieprawidlowy numer tramwaju." << endl;
                } else if (action == "ONLINE") {
                    shared_ptr <TramPrx> tram = tramList.at(number).tram;
                    if (tram->getStatus(Ice::Context()) == SIP::TramStatus::ONLINE) {
                        cout << "Tramwaj jest juz online" << endl;
                    } else {
                        depoPrx->TramOnline(tram, Ice::Context());
                        cout << "Tramwaj " << tram->getStockNumber() << " jest online" << endl;
                    }
                } else if (action == "OFFLINE") {
                    shared_ptr <TramPrx> tram = tramList.at(number).tram;
                    if (tram->getStatus(Ice::Context()) == SIP::TramStatus::OFFLINE) {
                        cout << "Tramwaj jest juz offline" << endl;
                    } else {
                        depoPrx->TramOffline(tram, Ice::Context());
                        cout << "Tramwaj " << tram->getStockNumber() << " jest offline" << endl;
                    }
                } else {
                    cout << "Nieznana komenda." << endl;
                }
            }
            sleep(1);
        }
        //zawieszam watek az do przerwania
        ic->waitForShutdown();

    }
    catch (const Ice::Exception &e) {
        cout << e << endl;
    } catch (const char *msg) {
        cout << msg << endl;
    }

    if (ic) {
        try {
            ic->destroy();
        } catch (const Ice::Exception &e) {
            cout << e << endl;
        }
    }

    cout << "Koniec pracy systemu" << endl;
}


