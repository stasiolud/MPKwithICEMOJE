#include "Depo.h"
#include <iostream>

using namespace std;
using namespace SIP;

DepoI::DepoI(string name) : name(name) {}

void DepoI::TramOnline(shared_ptr <TramPrx> tram, const Ice::Current &current) {
    if (tram) {
        tram->setStatus(TramStatus::ONLINE, Ice::Context());
        cout << "Tramwaj " << tram->getStockNumber() << " wyjechal z zajezdni" << endl;
    } else {
        cout << "Dany Tramwaj nie istnieje" << endl;
    }
}

void DepoI::TramOffline(shared_ptr <TramPrx> tram, const Ice::Current &current) {
    if (tram == nullptr) {
        cout << "Nie znaleziono tramwaju." << endl;
        return;
    }

    tram->setStatus(TramStatus::OFFLINE, current.ctx);
    cout << "Tramwaj " << tram->getStockNumber() << " został wyłączony i wrócił do zajezdni." << endl;

    for (auto it = all_trams.begin(); it != all_trams.end(); ++it) {
        if (it->tram->ice_getIdentity() == tram->ice_getIdentity()) {
            all_trams.erase(it);
            break;
        }
    }
}

string DepoI::getName(const Ice::Current &current) {
    return name;
}

void DepoI::registerTram(shared_ptr <TramPrx> tram, const Ice::Current &current) {
    TramInfo tramInfo;
    tramInfo.tram = tram;
    tramInfo.tram->setStatus(TramStatus::WAITONLINE, Ice::Context());
    all_trams.push_back(tramInfo);
    cout << "Zajezdnia zarejestrowala tramwaj o numerze: " << tram->getStockNumber() << endl;
}

void DepoI::unregisterTram(shared_ptr <TramPrx> tram, const Ice::Current &current) {
    TramInfo tramInfo;
    tramInfo.tram = tram;
    tramInfo.tram->setStatus(TramStatus::WAITOFFLINE, Ice::Context());
}

TramList DepoI::getTrams(const Ice::Current &current) {
    return all_trams;
}
