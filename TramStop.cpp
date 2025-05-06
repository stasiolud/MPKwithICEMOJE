#include "TramStop.h"
#include <iostream>


using namespace std;
using namespace SIP;

TramStopI::TramStopI(string name) {
    this->name = name;
}

void TramStopI::addLine(shared_ptr <LinePrx> line) {
    lines.push_back(line);
}

string TramStopI::getName(const Ice::Current &current) {
    return name;
}

TramList TramStopI::getNextTrams(int howMany, const Ice::Current &current) {
    TramList nextTrams;
    for (int i = 0; i < howMany; ++i) {
        if (i < coming_trams.size()) {
            nextTrams.push_back(coming_trams.at(i));
        }
    }
    return nextTrams;
}

void TramStopI::RegisterPassenger(shared_ptr <PassengerPrx> passenger, const Ice::Current &current) {
    passengers.push_back(passenger);
    cout << "Pasazer zasubskrybowal przystanek: " << name << endl;
    cout << "Przystanek: " << this->name << endl;
    cout << "Liczba zasubskrybowanych pasażerów: " << passengers.size() << endl;
}

void TramStopI::UnregisterPassenger(shared_ptr <PassengerPrx> passenger, const Ice::Current &current) {
    for (int index = 0; index < passengers.size(); index++) {
        if (passengers.at(index)->ice_getIdentity() == passenger->ice_getIdentity()) {
            passengers.erase(passengers.begin() + index);
            cout << "Pasazer odsubskrybowal przystanek: " << name << endl;
            break;
        }
    }
}

void TramStopI::UpdateTramInfo(shared_ptr <TramPrx> tram, Time time, const Ice::Current &current) {
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
}

void TramStopI::addCurrentTram(shared_ptr <TramPrx> tram, const Ice::Current &current) {
    TramInfo tramInfo;
    tramInfo.tram = tram;
    currentTrams.push_back(tramInfo);
    string header = "Tramwaje na przystanku " + name;
    cout << header << endl;
    cout << "Liczba zasubskrybowanych pasażerów: " << passengers.size() << endl;
    for (const auto &passenger: passengers) {
        cout << "pass1" << endl;
        passenger->notifyPassenger(header, Ice::Context());
    }
    for (auto it = currentTrams.begin(); it != currentTrams.end(); ++it) {
        string info = "Tramwaj: " + it->tram->getStockNumber();
        cout << info << endl;
        for (const auto &passenger: passengers) {
            cout << "pass1" << endl;
            passenger->notifyPassenger(info, Ice::Context());
        }

    }
}

void TramStopI::removeCurrentTram(shared_ptr <TramPrx> tram, const Ice::Current &current) {
    for (auto it = currentTrams.begin(); it != currentTrams.end(); ++it) {
        if (it->tram->ice_getIdentity() == tram->ice_getIdentity()) {
            currentTrams.erase(it);
            break;
        }
    }
}
