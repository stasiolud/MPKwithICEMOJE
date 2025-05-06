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
    std::shared_ptr <TramPrx> selfPrx;

public:
    TramI(string stockNumber) {
        this->stockNumber = stockNumber;
        this->status = SIP::TramStatus::OFFLINE;
    };

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
        for (int index = 0; index < passengers.size(); index++) {
            if (passengers.at(index)->ice_getIdentity() == passenger->ice_getIdentity()) {
                cout << "Uzytkownik zakonczyl subskrypcje" << endl;
                passengers.erase(passengers.begin() + index);
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

int main(int argc, char *argv[]) {
    string address = "";
    string port = "";
    string name = "";
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <tramPort ex. 10010>" << endl;
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
        cerr << "Unable to open configfile.txt" << endl;
        return 1;
    }

    if (address.empty() || port.empty() || name.empty()) {
        cerr << "Missing required configuration parameters in configfile.txt" << endl;
        cerr << "Required parameters: address, port, name" << endl;
        return 1;
    }

    Ice::CommunicatorPtr ic;
    try {
        // uzyskuje dostep do obiektu sip
        ic = Ice::initialize(argc, argv);
        auto base = ic->stringToProxy(name + ":default -h " + address + " -p " + port + " -t 8000");
        auto mpk = Ice::checkedCast<MPKPrx>(base);
        if (!mpk) {
            throw "Invalid proxy";
        }

        //pobieram dostepne linie
        LineList lines = mpk->getLines();

        //wyswietlam info o dostepnych liniach
        cout << "Dostepne linie: " << endl << endl;
        for (int index = 0; index < lines.size(); ++index) {
            cout << "Linia nr: " << lines.at(index)->getName() << endl << "\tPrzystanki: " << endl;
            StopList tramStops = lines.at(index)->getStops();
            for (int stopIndex = 0; stopIndex < tramStops.size(); stopIndex++) {
                cout << "\t\t" << tramStops.at(stopIndex).stop->getName() << endl;
            }
            cout << endl << endl;
        }

        //tworze obiekt ice
        Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("TramAdapter", "default -p " + tramPort);

        //pobieram numer tramwaju
        string tramStockNumber;
        cout << "Wprowadź swój numer tramwaju: ";
        cin >> tramStockNumber;
        std::cin.clear();
        cout << endl;

        //tworze servant tramwaju
        auto tram = make_shared<TramI>(tramStockNumber);
        auto tramPrx = Ice::uncheckedCast<TramPrx>(adapter->addWithUUID(tram));
        tram->setProxy(tramPrx);
        adapter->add(tram, Ice::stringToIdentity("tram" + tramStockNumber));

        //wybieram do ktorej lini dolaczam
        string line_name;
        cout << "Wybierz linie, wpisujac nazwe: ";
        cin >> line_name;

        while (!checkName(line_name, lines)) {
            cout << "Niewlasciwa linia, wybierz ponownie: " << endl;
            cin >> line_name;
        }

        std::cin.clear();

        //ustawienie czasu dotarcia na przystanki
        time_t currentTime;
        time(&currentTime);
        tm *timeNow = localtime(&currentTime);

        int ID = getIdLine(lines, line_name);

        shared_ptr <LinePrx> linePrx = lines.at(ID);
        tram->setLine(linePrx, Ice::Current());

        StopList tramStops = linePrx->getStops();
        int hour = timeNow->tm_hour;
        int minute = timeNow->tm_min;
        int interval = 5;

        for (int index = 0; index < tramStops.size(); index++) {
            Time timeOfDay;
            timeOfDay.hour = hour;
            timeOfDay.minute = minute;


            //tramStops.at(index).time = timeOfDay;
            StopInfo stopInfo;
            stopInfo.time = timeOfDay;
            shared_ptr <TramStopPrx> tramStopPrx = tramStops.at(index).stop;
            stopInfo.stop = tramStopPrx;

            tram->addStop(stopInfo);
            tramStops.at(index).stop->UpdateTramInfo(tramPrx, timeOfDay);

            minute += interval;
            if (minute >= 60) {
                hour++;
                minute = minute - 60;
            }
        }

        //tram->setNextStop();

        //dolaczanie do linii
        adapter->activate();
        linePrx->registerTram(tramPrx);
        mpk->getDepo("Zajezdnia1")->registerTram(tramPrx);
        cout << "Waiting for tram to be online..." << endl;
        while (tram->getStatus(Ice::Current()) != SIP::TramStatus::ONLINE) {
            cout << "Czekam na online tramwaju..." << endl;
            sleep(1);
        }
        char sign;
        cout << "Znak 'q' konczy program. Znak 'n' oznacza dotarcie do kolejnego przystanku" << endl;
        while (true) {
            cin >> sign;
            if (sign == 'q') {
                //tram->unregisterAllUser(tramPrx);
                break;
            }
            if (sign == 'n') {
                StopList stops = linePrx->getStops();
                auto currentStopName = tramPrx->getLocation()->getName();
                auto lastStopName = stops.at(stops.size() - 1).stop->getName();
                tram->setNextStop();
                cout << "Dotarłeś do kolejnego przystanku: " << tramPrx->getLocation()->getName() << endl;
//                tram->informAllUser(tramPrx);
            }
        }
        linePrx->unregisterTram(tramPrx);
        mpk->getDepo("Zajezdnia1")->unregisterTram(tramPrx);
        cout << "Jestes w zajezdni, czekam na offline tramwaju..." << endl;
        while (tram->getStatus(Ice::Current()) != SIP::TramStatus::OFFLINE) {
            cout << "Czekam na offline tramwaju..." << endl;
            sleep(1);
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

    cout << "Koniec programu tramwaj" << endl;
}
