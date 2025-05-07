#include <Ice/Ice.h>
#include "MPK.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <string>

using namespace std;
using namespace SIP;

class PassengerI : public SIP::Passenger {
private:
    string stopName = "";
public:
    void setTramStopName(string name) {
        stopName = name;
    }

    void updateTramInfo(shared_ptr <TramPrx> tram, StopList stops, const Ice::Current &current) override {
        cout << "Aktualizacje tramwaju: " << tram->getStockNumber() << endl;
        cout << "Następne przystanki:" << endl;

        //Lista przystankow
        for (const auto &stop: stops) {
            cout << "- " << stop.stop->getName() << " o godzinie "
                 << stop.time.hour << ":" << stop.time.minute << endl;
        }
    }

    void updateStopInfo(shared_ptr <TramStopPrx> tramStop, TramList tramList, const Ice::Current &current) override {
        for (int i = 0; i < tramList.size(); ++i) {
            TramInfo tramInfo = tramList.at(i);
            cout << "\t\t Tramwaj nr: " << tramInfo.tram->getStockNumber()
                 << "\t Czas przybycia: " << tramInfo.time.hour << ":" << tramInfo.time.minute << endl;
        }
    };

    void notifyPassenger(string info, const Ice::Current &current) override {
        cout << info << endl;
    }

};

string loadAddressFromFile(const string &filename) {
    ifstream file(filename);
    string address;
    if (file.is_open()) {
        getline(file, address);
        file.close();
    } else {
        throw runtime_error("Nie można otworzyć pliku: " + filename);
    }
    return address;
}


int main(int argc, char *argv[]) {
    string address = "";
    string port = "";
    string name = "";
    string tramPort = argv[1];

    ifstream configFile("configfile.txt");
    if (configFile.is_open()) {
        string line;
        while (getline(configFile, line)) {
            istringstream iss(line);
            string key, value;

            if (getline(iss, key, '=') && getline(iss, value)) {
                key.erase(remove_if(key.begin(), key.end(), ::isspace), key.end());
                value.erase(remove_if(value.begin(), value.end(), ::isspace), value.end());

                if (key == "address") {
                    address = value;
                } else if (key == "port") {
                    port = value;
                } else if (key == "name") {
                    name = value;
                }
            }
        }
        configFile.close();
    } else {
        cout << "Problem z plikiem configfile.txt" << endl;
        return 1;
    }

    if (address.empty() || port.empty() || name.empty()) {
        cout << "Brakuje danych w configfile.txt" << endl;
        return 1;
    }

    cout << "Podaj swoje imię: ";
    string nameUser;
    cin >> nameUser;
    cin.clear();

    Ice::CommunicatorPtr ic;

    try {
        ic = Ice::initialize(argc, argv);

        auto base = ic->stringToProxy(name + ":default -h " + address + " -p " + port + " -t 8000");
        auto mpk = Ice::checkedCast<MPKPrx>(base);
        if (!mpk) {
            throw "Invalid proxy";
        }

        ifstream userFile("configuserfile.txt");
        string userAddress;
        if (userFile.is_open()) {
            getline(userFile, userAddress);
            userFile.close();
        } else {
            throw "Nie można otworzyć configuserfile.txt";
        }

        string userEndpoint = "tcp -h " + userAddress + " -p " + tramPort;
        Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("PassengerAdapter", userEndpoint);

        auto passenger = make_shared<PassengerI>();
        auto passengerPrx = Ice::uncheckedCast<PassengerPrx>(adapter->addWithUUID(passenger));
        adapter->add(passenger, Ice::stringToIdentity(nameUser));

        LineList lines = mpk->getLines();
        StopList allStops;

        cout << "Dostępne linie:\n" << endl;
        for (int i = 0; i < lines.size(); ++i) {
            cout << "Linia nr: " << lines[i]->getName() << "\n\tPrzystanki:" << endl;
            StopList tramStops = lines[i]->getStops();

            for (const auto &stopInfo: tramStops) {
                auto tramStop = stopInfo.stop;
                cout << "\t\t" << tramStop->getName() << endl;

                bool found = false;
                for (const auto &s: allStops) {
                    if (s.stop->getName() == tramStop->getName()) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    StopInfo stopInfo;
                    stopInfo.stop = tramStop;
                    allStops.push_back(stopInfo);
                }
            }

            TramList trams = lines[i]->getTrams();
            cout << "\tTramwaje nr: ";
            for (const auto &tramInfo: trams) {
                cout << tramInfo.tram->getStockNumber() << " ";
            }
            cout << "\n" << endl;
        }

        cout << "\nDostępne przystanki: " << endl;
        for (const auto &stopInfo: allStops) {
            auto tramStop = stopInfo.stop;
            cout << "\t" << tramStop->getName() << endl;

            TramList fullTramList;
            int batchSize = 5;
            int totalFetched = 0;

            while (true) {
                TramList batch = tramStop->getNextTrams(totalFetched + batchSize);
                if (batch.size() == fullTramList.size()) {
                    break;
                }
                fullTramList = batch;
                totalFetched += batchSize;
            }

            passenger->updateStopInfo(tramStop, fullTramList, Ice::Current());
        }

        adapter->activate();
        shared_ptr <TramPrx> currentTram = nullptr;
        bool running = true;

        while (running) {
            cout << "\n===== MENU PASAŻERA =====" << endl;
            cout << "1. Subskrybuj przystanek" << endl;
            cout << "2. Subskrybuj tramwaj" << endl;
            cout << "3. Pokaż kolejne przystanki tramwaju" << endl;
            cout << "4. Wyświetl dostępne linie i przystanki" << endl;
            cout << "5. Zakończ program" << endl;
            cout << "Wybierz opcję: " << endl;

            int option;
            cin >> option;

            switch (option) {
                case 1: {
                    cout << "Podaj nazwę przystanku: ";
                    string stopName;
                    cin >> stopName;
                    try {
                        auto tramStop = mpk->getTramStop(stopName);
                        tramStop->RegisterPassenger(passengerPrx);
                        passenger->setTramStopName(stopName);
                        cout << "Zasubskrybowano przystanek: " << stopName << endl;
                    } catch (...) {
                        cout << "Nie znaleziono przystanku: " << stopName << endl;
                    }
                    break;
                }
                case 2: {
                    cout << "Podaj numer tramwaju: ";
                    string tramNumber;
                    cin >> tramNumber;

                    shared_ptr <TramPrx> tram = nullptr;
                    for (const auto &line: lines) {
                        for (const auto &tramInfo: line->getTrams()) {
                            if (tramInfo.tram->getStockNumber() == tramNumber) {
                                tram = tramInfo.tram;
                                break;
                            }
                        }
                        if (tram) break;
                    }

                    if (tram) {
                        tram->RegisterPassenger(passengerPrx);
                        currentTram = tram;
                        cout << "Zasubskrybowano tramwaj: " << tramNumber << endl;
                    } else {
                        cout << "Nie znaleziono tramwaju o numerze: " << tramNumber << endl;
                    }
                    break;
                }
                case 3: {
                    if (!currentTram) {
                        cout << "Najpierw zasubskrybuj tramwaj." << endl;
                        break;
                    }

                    cout << "Ile kolejnych przystanków chcesz zobaczyć? ";
                    int numberOfStops;
                    cin >> numberOfStops;

                    StopList nextStops = currentTram->getNextStops(numberOfStops);
                    passenger->updateTramInfo(currentTram, nextStops, Ice::Current());
                    break;
                }
                case 4: {
                    cout << "Dostępne linie:\n" << endl;
                    allStops.clear();

                    for (int i = 0; i < lines.size(); ++i) {
                        cout << "Linia nr: " << lines[i]->getName() << "\n\tPrzystanki:" << endl;
                        StopList tramStops = lines[i]->getStops();

                        for (const auto &stopInfo: tramStops) {
                            auto tramStop = stopInfo.stop;
                            cout << "\t\t" << tramStop->getName() << endl;

                            bool found = false;
                            for (const auto &s: allStops) {
                                if (s.stop->getName() == tramStop->getName()) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                StopInfo stopInfo;
                                stopInfo.stop = tramStop;
                                allStops.push_back(stopInfo);
                            }
                        }

                        TramList trams = lines[i]->getTrams();
                        cout << "\tTramwaje nr: ";
                        for (const auto &tramInfo: trams) {
                            cout << tramInfo.tram->getStockNumber() << " ";
                        }
                        cout << "\n" << endl;
                    }

                    cout << "\nDostępne przystanki: " << endl;
                    for (const auto &stopInfo: allStops) {
                        auto tramStop = stopInfo.stop;
                        cout << "\t" << tramStop->getName() << endl;

                        TramList fullTramList;
                        int batchSize = 5;
                        int totalFetched = 0;

                        while (true) {
                            TramList batch = tramStop->getNextTrams(totalFetched + batchSize);
                            if (batch.size() == fullTramList.size()) {
                                break;
                            }
                            fullTramList = batch;
                            totalFetched += batchSize;
                        }

                        passenger->updateStopInfo(tramStop, fullTramList, Ice::Current());
                    }
                    break;
                }
                case 5: {
                    running = false;
                    break;
                }
                default:
                    cout << "Nieprawidłowa opcja. Spróbuj ponownie." << endl;
                    break;
            }
        }

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

    cout << "Koniec programu użytkownika" << endl;
    return 0;
}
