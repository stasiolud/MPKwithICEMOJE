#include <Ice/Ice.h>
#include "MPK.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include <TramStop.h>
#include <Line.h>
#include <Depo.h>
#include <LineFactory.h>
#include <StopFactory.h>

using namespace std;
using namespace SIP;

class MPK_I : public SIP::MPK {
private:
    LineList all_lines;
    StopList all_stops;
    DepoList all_depos;
    vector <shared_ptr<LineFactoryPrx>> lineFactories;
    vector <shared_ptr<StopFactoryPrx>> stopFactories;
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

    void registerDepo(shared_ptr <DepoPrx> depo, const Ice::Current &current) override {
        cout << "Nowa zajezdnia o nazwie: " << depo->getName() << endl;
        DepoInfo depoInfo;
        depoInfo.stop = depo;
        depoInfo.name = depo->getName();
        all_depos.push_back(depoInfo);
    }

    void unregisterDepo(shared_ptr <DepoPrx> depo, const Ice::Current &current) override {
        for (int i = 0; i < all_depos.size(); i++) {
            if (all_depos.at(i).stop->getName() == depo->getName()) {
                cout << "Usuwam zajezdnie o nazwie: " << depo->getName() << endl;
                all_depos.erase(all_depos.begin() + i);
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

    void registerLineFactory(shared_ptr <LineFactoryPrx> lf, const Ice::Current &current) override {
        // Sprawdzenie, czy fabryka już jest zarejestrowana
        if (find(lineFactories.begin(), lineFactories.end(), lf) == lineFactories.end()) {
            lineFactories.push_back(lf);
            cout << "Fabryka linii zarejestrowana" << endl;
        }
    }

    void unregisterLineFactory(shared_ptr <LineFactoryPrx> lf, const Ice::Current &current) override {
        auto it = find(lineFactories.begin(), lineFactories.end(), lf);
        if (it != lineFactories.end()) {
            lineFactories.erase(it);
            cout << "Fabryka linii wyrejestrowana" << endl;
        }
    }

    void registerStopFactory(shared_ptr <StopFactoryPrx> lf, const Ice::Current &current) override {
        // Sprawdzenie, czy fabryka już jest zarejestrowana
        if (find(stopFactories.begin(), stopFactories.end(), lf) == stopFactories.end()) {
            stopFactories.push_back(lf);
            cout << "Fabryka przystanku zarejestrowana" << endl;
        }
    }

    void unregisterStopFactory(shared_ptr <StopFactoryPrx> lf, const Ice::Current &current) override {
        auto it = find(stopFactories.begin(), stopFactories.end(), lf);
        if (it != stopFactories.end()) {
            stopFactories.erase(it);
            cout << "Fabryka stopu wyrejestrowana" << endl;
        }
    }

};

map <string, string> loadConfig(const string &filename) {
    ifstream file(filename);
    map <string, string> config;
    string line;

    while (getline(file, line)) {
        istringstream is_line(line);
        string key;
        if (getline(is_line, key, '=')) {
            string value;
            if (getline(is_line, value)) {
                config[key] = value;
            }
        }
    }

    return config;
}


int main(int argc, char *argv[]) {
    Ice::CommunicatorPtr ic;
    try {
        auto config = loadConfig("configfile.txt");

        string address = config["address"];
        string port = config["port"];
        string endpoint = "tcp -h " + address + " -p " + port;

        Ice::CommunicatorHolder ich(argc, argv);
        auto adapter = ich->createObjectAdapterWithEndpoints("MPKAdapter", endpoint);

        // ========== REJESTRACJA OBIEKTÓW ==========
        cout << "===== REJESTRACJA OBIEKTÓW SYSTEMU =====" << endl;

        auto mpk = make_shared<MPK_I>();
        adapter->add(mpk, Ice::stringToIdentity("mpk"));

        auto depo = make_shared<DepoI>("Zajezdnia1");
        auto depoPrx = Ice::uncheckedCast<DepoPrx>(adapter->addWithUUID(depo));
        mpk->registerDepo(depoPrx, Ice::Current());

        auto lineFactory = make_shared<LineFactoryI>(adapter);
        auto lineFactoryPrx = Ice::uncheckedCast<LineFactoryPrx>(adapter->addWithUUID(lineFactory));
        mpk->registerLineFactory(lineFactoryPrx, Ice::Current());

        auto stopFactory = make_shared<StopFactoryI>(adapter);
        auto stopFactoryPrx = Ice::uncheckedCast<StopFactoryPrx>(adapter->addWithUUID(stopFactory));
        mpk->registerStopFactory(stopFactoryPrx, Ice::Current());

        cout << "\n===== WCZYTYWANIE PRZYSTANKÓW =====" << endl;
        ifstream stops_file("stops.txt");
        if (!stops_file.is_open()) {
            cerr << "Błąd: nie można otworzyć pliku stops.txt." << endl;
            throw "File error";
        }

        string stop_name;
        while (stops_file >> stop_name) {
            auto tramStopPrx = stopFactory->createStop(stop_name, Ice::Current());
            mpk->addStop(tramStopPrx);
        }

        cout << "\n===== WCZYTYWANIE LINII I PRZYSTANKÓW =====" << endl;
        ifstream lines_file("lines.txt");
        if (!lines_file.is_open()) {
            cerr << "Błąd: nie można otworzyć pliku lines.txt." << endl;
            throw "File error";
        }

        string file_line;
        while (getline(lines_file, file_line)) {
            size_t separator_position = file_line.find(':');
            string line_number = file_line.substr(0, separator_position);
            string tramStopsNames = file_line.substr(separator_position + 1);
            cout << "Linia: " << line_number << endl;

            auto linePrx = lineFactory->createLine(line_number, Ice::Current());

            istringstream iss(tramStopsNames);
            StopList stopList;
            time_t currentTime;
            time(&currentTime);
            tm *timeNow = localtime(&currentTime);

            while (iss >> stop_name) {
                auto tramStopPrx = mpk->getTramStop(stop_name, Ice::Current());
                if (!tramStopPrx) {
                    tramStopPrx = stopFactory->createStop(stop_name, Ice::Current());
                    mpk->addStop(tramStopPrx);
                }

                StopInfo stopInfo;
                stopInfo.time.hour = timeNow->tm_hour;
                stopInfo.time.minute = timeNow->tm_min;
                stopInfo.stop = tramStopPrx;
                stopList.push_back(stopInfo);
            }

            linePrx->setStops(stopList);
            mpk->addLine(linePrx, Ice::Current());

            // wypisanie przystanków
            istringstream ss(tramStopsNames);
            string name;
            cout << "\tPrzystanki: ";
            while (ss >> name) {
                auto tramStopPrx = mpk->getTramStop(name, Ice::Current());
                cout << tramStopPrx->getName() << " ";
            }
            cout << "\n" << endl;
        }

        adapter->activate();
        cout << "\n===== SYSTEM GOTOWY =====" << endl;


        while (true) {
            cout << "\nNaciśnij 'd' aby przejść do zajezdni: ";
            char sign;
            cin >> sign;

            if (sign == 'd') {
                TramList tramList = depoPrx->getTrams(Ice::Context());
                cout << "\n===== TRAMWAJE W ZAJEDZNI =====" << endl;
                for (int i = 0; i < tramList.size(); ++i) {
                    cout << i << ". " << tramList[i].tram->getStockNumber() << " - ";
                    switch (tramList[i].tram->getStatus(Ice::Context())) {
                        case TramStatus::ONLINE:
                            cout << "driving";
                            break;
                        case TramStatus::OFFLINE:
                            cout << "offline";
                            break;
                        case TramStatus::WAITONLINE:
                            cout << "waiting to online";
                            break;
                        case TramStatus::WAITOFFLINE:
                            cout << "waiting to offline";
                            break;
                        default:
                            cout << "unknown";
                    }
                    cout << endl;
                }

                cout << "\nWpisz '<numer> ONLINE' lub '<numer> OFFLINE', lub 'q' aby wyjść." << endl;
                string command;
                cin.ignore();
                getline(cin, command);

                if (command == "q") {
                    cout << "Opuszczanie..." << endl;
                    return 0;
                }

                istringstream iss(command);
                int number;
                string action;
                iss >> number >> action;

                if (number < 0 || number >= tramList.size()) {
                    cout << "Błąd: nieprawidłowy numer tramwaju." << endl;
                } else {
                    auto tram = tramList[number].tram;
                    if (action == "ONLINE") {
                        if (tram->getStatus(Ice::Context()) == TramStatus::ONLINE) {
                            cout << "Tramwaj jest już online." << endl;
                        } else {
                            depoPrx->TramOnline(tram, Ice::Context());
                            cout << "Tramwaj " << tram->getStockNumber() << " przełączony na ONLINE." << endl;
                        }
                    } else if (action == "OFFLINE") {
                        if (tram->getStatus(Ice::Context()) == TramStatus::OFFLINE) {
                            cout << "Tramwaj jest już offline." << endl;
                        } else {
                            depoPrx->TramOffline(tram, Ice::Context());
                            cout << "Tramwaj " << tram->getStockNumber() << " przełączony na OFFLINE." << endl;
                        }
                    } else {
                        cout << "Nieznana komenda." << endl;
                    }
                }
            }
        }

        ic->waitForShutdown();

    } catch (const Ice::Exception &e) {
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

    cout << "\n===== KONIEC PRACY SYSTEMU =====" << endl;
    return 0;
}
