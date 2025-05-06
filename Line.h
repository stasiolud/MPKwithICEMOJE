#pragma once

#include <Ice/Ice.h>
#include "MPK.h"
#include <string>
#include <memory>
#include <vector>

using namespace std;
using namespace SIP;

class LineI : public SIP::Line {
private:
    TramList all_trams;
    StopList all_stops;
    string name;

public:
    LineI(string name);

    TramList getTrams(const Ice::Current &current) override;

    StopList getStops(const Ice::Current &current) override;

    string getName(const Ice::Current &current) override;

    void registerTram(shared_ptr <TramPrx> tram, const Ice::Current &current) override;

    void unregisterTram(shared_ptr <TramPrx> tram, const Ice::Current &current) override;

    void setStops(StopList sl, const Ice::Current &current) override;
};
