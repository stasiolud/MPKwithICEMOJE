#pragma once

#include <Ice/Ice.h>
#include "MPK.h"
#include <memory>
#include <vector>
#include <string>

using namespace std;
using namespace SIP;

class TramStopI : public SIP::TramStop {
private:
    string name;
    LineList lines;
    vector <shared_ptr<PassengerPrx>> passengers;
    TramList coming_trams;
    TramList currentTrams;

public:
    TramStopI(string name);

    void addLine(shared_ptr <LinePrx> line);

    string getName(const Ice::Current &current) override;

    TramList getNextTrams(int howMany, const Ice::Current &current) override;

    void RegisterPassenger(shared_ptr <PassengerPrx> passenger, const Ice::Current &current) override;

    void UnregisterPassenger(shared_ptr <PassengerPrx> passenger, const Ice::Current &current) override;

    void UpdateTramInfo(shared_ptr <TramPrx> tram, Time time, const Ice::Current &current) override;

    void addCurrentTram(shared_ptr <TramPrx> tram, const Ice::Current &current) override;

    void removeCurrentTram(shared_ptr <TramPrx> tram, const Ice::Current &current) override;

    void removeComingTram(shared_ptr <TramPrx> tram, const Ice::Current &current) override;

};
