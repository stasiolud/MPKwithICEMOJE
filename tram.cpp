#include <Ice/Ice.h>
#include "MPK.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <string>

using namespace std;
using namespace SIP;

class TramI : public SIP::Tram {
private:
    TramStatus status;
    string stockNumber;
    shared_ptr <TramStopPrx> currentStop;
    StopList stopList;
    vector <shared_ptr<PassengerPrx>> passengers;
    shared_ptr <LinePrx> line;
    shared_ptr <TramPrx> selfPrx;

public:
    TramI(string stockNumber) : stockNumber(stockNumber), status(TramStatus::OFFLINE) {};

    void addStop(const struct StopInfo stopInfo) {
        stopList.push_back(stopInfo);
    };

    void setProxy(std::shared_ptr <TramPrx> prx) {
        selfPrx = prx;
    }


    void setNextStop() {
        if (line) {
            for (int i = 0; i < line->getStops().size(); ++i) {
                if (this->currentStop->getName() == line->getStops().at(i).stop->getName()) {
                    if (i + 1 < line->getStops().size()) {
                        this->currentStop->removeCurrentTram(selfPrx);
                        this->currentStop = line->getStops().at(i + 1).stop;
                        this->currentStop->addCurrentTram(selfPrx);
                        for (auto &passenger: passengers) {
                            string info =
                                    "Tramwaj " + this->stockNumber + " dojechal do " + this->currentStop->getName();
                            passenger->notifyPassenger(info, Ice::Context());
                        }
                    } else {
                        i = 0;
                        this->currentStop->removeCurrentTram(selfPrx);
                        this->currentStop = line->getStops().at(i).stop;
                        this->currentStop->addCurrentTram(selfPrx);
                        for (auto &passenger: passengers) {
                            string info =
                                    "Tramwaj " + this->stockNumber + " dojechal do " + this->currentStop->getName();
                            passenger->notifyPassenger(info, Ice::Context());
                        }
                    }
                    return;
                }
            }
        }
    }

    void setLine(shared_ptr <LinePrx> line, const Ice::Current &current) override {
        this->line = line;
        this->currentStop = this->line->getStops().at(0).stop;
    }

    shared_ptr <LinePrx> getLine(const Ice::Current &current) override {
        return line;
    }

    shared_ptr <TramStopPrx> getLocation(const Ice::Current &current) override {
        return currentStop;
    };

    int getNextStopIndex(StopList allStops) {
        for (int i = 0; i < allStops.size(); i++) {
            if (allStops.at(i).stop->getName() == currentStop->getName()) {
                if (i + 1 < allStops.size()) {
                    return i + 1;
                } else {
                    return -1;
                }
            }
        }
        return -1;
    };

    StopList getNextStops(int howMany, const Ice::Current &current) override {
        StopList nextStops;

        StopList allStops = line->getStops();
        int stopIndex = getNextStopIndex(allStops);

        if (stopIndex != -1) {
            for (int i = stopIndex; i < stopIndex + howMany; ++i) {
                if (i < allStops.size()) {
                    nextStops.push_back(allStops.at(i));
                }
            }
            for (int i = stopIndex - 2; i >= 0; --i) {
                if (nextStops.size() < howMany) {
                    nextStops.push_back(allStops.at(i));
                }
            }
        } else {
            for (int i = 0; i < allStops.size() - 1; ++i) {
                if (nextStops.size() < howMany) {
                    nextStops.push_back(allStops.at(i));
                }
            }
        }

        return nextStops;
    }

    void informPassenger(shared_ptr <TramPrx> tram, StopList stops) {
        for (int i = 0; i < passengers.size(); ++i) {
            passengers.at(i)->updateTramInfo(tram, stops);
        }
    }

    void RegisterPassenger(shared_ptr <PassengerPrx> passenger, const Ice::Current &current) override {
        cout << "Uzytkownik subskrybuje" << endl;
        passengers.push_back(passenger);
    };

    void UnregisterPassenger(shared_ptr <PassengerPrx> passenger, const Ice::Current &current) override {
        for (int i = 0; i < passengers.size(); i++) {
            if (passengers.at(i)->ice_getIdentity() == passenger->ice_getIdentity()) {
                cout << "Uzytkownik zakonczyl subskrypcje" << endl;
                passengers.erase(passengers.begin() + i);
                break;
            }
        }
    };

    string getStockNumber(const Ice::Current &current) override {
        return stockNumber;
    }

    TramStatus getStatus(const Ice::Current &current) override {
        return status;
    }

    void setStatus(TramStatus status, const Ice::Current &current) override {
        this->status = status;
    }
};

int getIdLine(LineList lines, string name) {
    for (int i = 0; i < lines.size(); ++i) {
        if (lines.at(i)->getName() == name) {
            return i;
        }
    }
    return -1;
}

bool checkName(string line_name, LineList lines) {
    for (int i = 0; i < lines.size(); ++i) {
        if (line_name == lines.at(i)->getName()) {
            return true;
        }
    }
    return false;
}

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

    if (argc < 2) {
        cout << "Uzycie: " << argv[0] << " <tramPort>" << endl;
        return 1;
    }

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
        cout << "Problem z configfile.txt" << endl;
        return 1;
    }

    if (address.empty() || port.empty() || name.empty()) {
        cout << "Brakuje argumentow w configfile.txt" << endl;
        return 1;
    }

    Ice::CommunicatorPtr ic;
    try {
        ic = Ice::initialize(argc, argv);
        auto base = ic->stringToProxy(name + ":default -h " + address + " -p " + port + " -t 8000");
        auto mpk = Ice::checkedCast<MPKPrx>(base);
        if (!mpk) {
            throw "Invalid proxy";
        }

        LineList lines = mpk->getLines();

        cout << "DOSTĘPNE LINIE:\n" << endl;
        for (const auto &line: lines) {
            cout << "Linia: " << line->getName() << "\nPrzystanki:" << endl;
            for (const auto &stopInfo: line->getStops()) {
                cout << "  - " << stopInfo.stop->getName() << endl;
            }
            cout << endl;
        }

        string userAddress = loadAddressFromFile("configuserfile.txt");
        string userEndpoint = "tcp -h " + userAddress + " -p " + tramPort;

        Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("TramAdapter", userEndpoint);

        string tramStockNumber;
        cout << "Podaj numer tramwaju: ";
        cin >> tramStockNumber;
        cin.clear();
        cout << endl;

        auto tram = make_shared<TramI>(tramStockNumber);
        auto tramPrx = Ice::uncheckedCast<TramPrx>(adapter->addWithUUID(tram));
        tram->setProxy(tramPrx);
        adapter->add(tram, Ice::stringToIdentity("tram" + tramStockNumber));

        string line_name;
        cout << "Podaj nazwę linii: ";
        cin >> line_name;
        while (!checkName(line_name, lines)) {
            cout << "Nieprawidłowa linia. Podaj ponownie: ";
            cin >> line_name;
        }

        int ID = getIdLine(lines, line_name);
        shared_ptr <LinePrx> linePrx = lines.at(ID);
        tram->setLine(linePrx, Ice::Current());

        StopList tramStops = linePrx->getStops();

        time_t currentTime;
        time(&currentTime);
        tm *timeNow = localtime(&currentTime);

        int baseHour = timeNow->tm_hour;
        int baseMinute = timeNow->tm_min;
        int interval = 3;

        int currentMinutes = baseHour * 60 + baseMinute;

        for (const auto &stop : tramStops) {
            Time timeOfDay;
            timeOfDay.hour = currentMinutes / 60;
            timeOfDay.minute = currentMinutes % 60;

            StopInfo stopInfo;
            stopInfo.time = timeOfDay;
            stopInfo.stop = stop.stop;

            tram->addStop(stopInfo);
            stop.stop->UpdateTramInfo(tramPrx, timeOfDay);

            currentMinutes += interval;
        }


        adapter->activate();
        linePrx->registerTram(tramPrx);
        mpk->getDepo("Zajezdnia1")->registerTram(tramPrx);

        cout << endl;
        cout << "==============================" << endl;
        cout << "   OCZEKIWANIE NA STATUS ONLINE" << endl;
        cout << "==============================" << endl;

        while (tram->getStatus(Ice::Current()) != SIP::TramStatus::ONLINE) {
            cout << "Czekam na tramwaj w trybie ONLINE..." << endl;
        }

        cout << endl;
        cout << "==============================" << endl;
        cout << "     TRAMWAJ JEST ONLINE" << endl;
        cout << "Numer tramwaju: " << tramPrx->getStockNumber() << endl;
        cout << "==============================" << endl;

        cout << endl;
        cout << "INSTRUKCJA OBSŁUGI:" << endl;
        cout << "  n - przejazd do następnego przystanku" << endl;
        cout << "  q - zakończ działanie tramwaju" << endl;
        cout << "==============================" << endl;

        char sign;
        while (true) {
            cout << endl << "Wybierz opcję [n/q]: " << endl;
            cin >> sign;

            if (sign == 'q') {
                cout << "Kończenie pracy tramwaju..." << endl;
                break;
            }

            if (sign == 'n') {
                tram->setNextStop();
                string stopName = tramPrx->getLocation()->getName();
                cout << "--------------------------------" << endl;
                cout << "Dotarłeś do przystanku: " << stopName << endl;
                cout << "--------------------------------" << endl;
            } else {
                cout << "Nieznana komenda. Wpisz 'n' lub 'q'." << endl;
            }
        }


        mpk->getDepo("Zajezdnia1")->unregisterTram(tramPrx);

        cout << "Oczekiwanie na przejście tramwaju w tryb OFFLINE..." << endl;
        while (tram->getStatus(Ice::Current()) != SIP::TramStatus::OFFLINE) {
            cout << "Czekam na tryb OFFLINE..." << endl;
        }
        linePrx->unregisterTram(tramPrx);

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

    cout << "Koniec programu tramwaj" << endl;
    return 0;
}
