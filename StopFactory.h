#pragma once

#include <Ice/Ice.h>
#include "MPK.h"
#include "TramStop.h"
#include <memory>
#include <string>

using namespace std;
using namespace SIP;

class StopFactoryI : public SIP::StopFactory {
private:
    int stopsCreated;
    Ice::ObjectAdapterPtr adapter;

public:
    StopFactoryI(Ice::ObjectAdapterPtr adapter);

    shared_ptr <TramStopPrx> createStop(string name, const Ice::Current &current) override;

    double getLoad(const Ice::Current &current = Ice::Current()) override;
};
