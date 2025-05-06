#include <Ice/Ice.h>
#include "MPK.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <string>

using namespace std;
using namespace SIP;

class PassengerI:public SIP::Passenger {
    private:
        string stopName = "";
    public:
        void setTramStopName(string name) {
            stopName = name;
        }
        void updateTramInfo(shared_ptr<TramPrx> tram, StopList stops, const Ice::Current& current) override {
            cout << "Aktualizacje tramwaju: " << tram->getStockNumber() << endl;
            cout << "Następne przystanki:" << endl;

            // Wypisujemy listę przystanków, na które tramwaj ma dotrzeć
            for (const auto& stop : stops) {
                cout << "- " << stop.stop->getName() << " o godzinie "
                     << stop.time.hour << ":" << stop.time.minute << endl;
            }
        }
        void updateStopInfo(shared_ptr<TramStopPrx> tramStop, TramList tramList, const Ice::Current& current) override{
            for(int i = 0; i < tramList.size(); ++i){
                TramInfo tramInfo = tramList.at(i);
                cout << "\t\t Tramwaj nr: " << tramInfo.tram->getStockNumber()
                     << "\t Czas przybycia: " << tramInfo.time.hour << ":" << tramInfo.time.minute << endl;
            }
        };
};

//enum subscription_type {TRAM, STOP};

//bool checkName(string tramStopName, StopList allStops, enum subscription_type type){
//    if(type == TRAM){
//        for(int i = 0; i < allStops.size(); ++i){
//            if(tramStopName == allStops.at(i).stop->getName()){
//                return true;
//            }
//        }
//    }
//    else{
//        for(int i = 0; i < allStops.size(); ++i){
//            if(tramStopName == allStops.at(i).stop->getName()){
//                return true;
//            }
//        }
//    }
//    return false;
//}

int main (int argc, char *argv[])
{
    string address = "";
    string port = "";
    string name = "";

    ifstream configFile("configfile.txt");
    if (configFile.is_open()) {
        string line;
        while (getline(configFile, line)) {
            istringstream iss(line);
            string key, value;

            if (getline(iss, key, '=') && getline(iss, value)) {
                // Remove any whitespace
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

        //tworze obiekt ice
        Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints ("PassengerAdapter", "default -p 10004");

        //tworze servant użytkownika
        auto passenger = make_shared<PassengerI>();
        auto passengerPrx = Ice::uncheckedCast<PassengerPrx>(adapter->addWithUUID(passenger));
        adapter->add(passenger, Ice::stringToIdentity("passenger1"));
        //pobieram dostepne linie
        LineList lines = mpk->getLines();

        //wyswietlam informacje o dostepnych liniach i tramwajach

 //       StopList newStopList = lines.at(0)->getStops();

//        TramList tramList2 = newStopList.at(0).stop->getNextTrams(1);
//
//        for(int i = 0; i < tramList2.size(); ++i){
//            cout << tramList2.at(i).tram->getStockNumber() << endl;
//        }

        StopList allStops;
        cout << "Dostepne linie: " << endl << endl;
        for(int index = 0; index < lines.size(); ++index){
            cout << "Linia nr: " << lines.at(index)->getName() << endl << "\t Przystanki: " << endl;
            StopList tramStops = lines.at(index)->getStops();

            cout << "stopy ilosc: " << tramStops.size() << endl;

            for(int stopIndex = 0; stopIndex < tramStops.size(); stopIndex++){
                shared_ptr<TramStopPrx> tramStop = tramStops.at(stopIndex).stop;
                cout << "\t\t" << tramStop->getName() << endl;
                bool found = false;
                for(int indexAllStop = 0; indexAllStop < allStops.size(); indexAllStop++){
                    if(allStops.at(indexAllStop).stop->getName() == tramStop->getName()){
                        found = true;
                        break;
                    }
                }
                if(!found){
                    StopInfo stopInfo;
                    stopInfo.stop = tramStop;
                    allStops.push_back(stopInfo);
                }
            }

            cout << endl;
            TramList trams = lines.at(index)->getTrams();
            cout << "\t Tramwaje nr: ";
            for(int tramIndex = 0; tramIndex < trams.size(); tramIndex++){
                cout << trams.at(tramIndex).tram->getStockNumber() << " ";
            }
            cout << endl << endl;
        }

        //wyswietlam info o przystankach i tramwajach
        cout << endl;
        cout << "Dostępne przystanki: " << endl;
        for(int stopIndex = 0; stopIndex < allStops.size(); stopIndex++){
            shared_ptr<TramStopPrx> tramStop = allStops.at(stopIndex).stop;
            cout << "\t" << tramStop->getName() << endl;

            TramList tramList = tramStop->getNextTrams(1);
            passenger->updateStopInfo(tramStop, tramList, Ice::Current());
        }
        cout << endl << endl;

        //pobieram informacje uzytkownika co chce sledzic
        char choice;
        string name;
        cout << "Wybierz co chcesz zasubskrybowac: 'p' - przystanek, 't' - tramwaj" << endl;
        cin >> choice;
        if(choice == 'p') cout << "Podaj nazwe przystanku: " << endl;
        else if(choice == 't') cout << "Podaj numer tramwaju: " << endl;
        else throw "Niepoprawny wybor subskrypcji";

        cin >> name;
//        while(!checkName(name, allStops)){
//            cout << "Niepoprawna nazwa, wybierz ponownie" << endl;
//            cin >> name;
//        }
        std::cin.clear();

        //rejestruje uzytkownika
        adapter->activate();
        shared_ptr<TramPrx> tram = nullptr;
        shared_ptr<TramStopPrx> tramStop = nullptr;
        char sign;
        if(choice == 'p'){
            tramStop = mpk->getTramStop(name);
            if(tramStop){
                tramStop->RegisterPassenger(passengerPrx);
                passenger->setTramStopName(name);
            }else{
                throw "Nie znaleziono takiego przystanku";
            }
        }else{
            for(int index=0; index < lines.size(); index++){

                TramList trams = lines.at(index)->getTrams();
                for(int tramIndex = 0; tramIndex < trams.size(); tramIndex++){
                    if(trams.at(tramIndex).tram->getStockNumber() == name){
                        tram = trams.at(tramIndex).tram;
                        break;
                    }
                }
                if(tram) break;
            }
            if(tram) tram->RegisterPassenger(passengerPrx);
            else throw "Nie znalezionio tramwaju o podanym numerze";

            cout << "Klikniecie klawisza 'q' zakonczy program" << endl;
            cout << "Jesli chcesz poznac jakie sa kolejne przystanki, kliknij 'k'" << endl;
            while(true){
                cin >> sign;
                if(sign == 'k'){
                    cout << "Podaj ilosc przystankow jaka chcesz zobaczyc: " << endl;
                    int number_of_stops;
                    cin >> number_of_stops;
                    StopList nextStops = tram->getNextStops(number_of_stops);
                    passenger->updateTramInfo(tram, nextStops, Ice::Current());
                }
                if(sign == 'q') break;
            }
        }

        if(choice == 'p'){
            cout << "Wyrejestrowuje z przystanku" << endl;
            tramStop->UnregisterPassenger(passengerPrx);
        }
        else{
            cout << "Wyrejestrowuje z tramwaju" << endl;
            tram->UnregisterPassenger(passengerPrx);
        }

    }catch(const Ice::Exception & e){
        cout << e << endl;
    }catch(const char *msg){
        cout << msg << endl;
    }

    if(ic){
        try{
            ic->destroy();
        }catch(const Ice::Exception & e){
            cout << e << endl;
        }
    }

    cout << "Koniec programu uzytkownika" << endl;
}


