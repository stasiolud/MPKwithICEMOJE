#pragma once

#include <Ice/Ice.h>
#include "MPK.h"
#include <string>
#include <memory>
#include <vector>

using namespace std;
using namespace SIP;

class DepoI : public SIP::Depo {
private:
    string name;
    TramList all_trams;

public:
    DepoI(string name);

    void TramOnline(shared_ptr <TramPrx> tram, const Ice::Current &current) override;

    void TramOffline(shared_ptr <TramPrx> tram, const Ice::Current &current) override;

    string getName(const Ice::Current &current) override;

    void registerTram(shared_ptr <TramPrx> tram, const Ice::Current &current) override;

    void unregisterTram(shared_ptr <TramPrx> tram, const Ice::Current &current) override;

    TramList getTrams(const Ice::Current &current) override;
};
