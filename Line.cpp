#include "Line.h"
#include <iostream>

using namespace std;
using namespace SIP;

LineI::LineI(string name) : name(name) {}

TramList LineI::getTrams(const Ice::Current &current) {
    return all_trams;
}

StopList LineI::getStops(const Ice::Current &current) {
    return all_stops;
}

string LineI::getName(const Ice::Current &current) {
    return name;
}

void LineI::registerTram(shared_ptr <TramPrx> tram, const Ice::Current &current) {
    TramInfo tramInfo;
    tramInfo.tram = tram;
    all_trams.push_back(tramInfo);
    cout << "Nowy tramwaj o numerze: " << tram->getStockNumber() << " zostal dodany" << endl;
}

void LineI::unregisterTram(shared_ptr <TramPrx> tram, const Ice::Current &current) {
    for (int i = 0; i < all_trams.size(); ++i) {
        if (all_trams.at(i).tram->getStockNumber() == tram->getStockNumber()) {
            all_trams.erase(all_trams.begin() + i);
            break;
        }
    }
}

void LineI::setStops(StopList sl, const Ice::Current &current) {
    all_stops = sl;
}
