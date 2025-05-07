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


int main(int argc, char *argv[]) {
    Ice::CommunicatorPtr ic;
    try {

        //tworze instancje obiektu ice
        ic = Ice::initialize(argc, argv);
        Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("MPKAdapter", "tcp -h 192.168.0.11 -p 10000"); //moje IP trzeba wstawic

        //tworze servant mpk
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


