#include "StopFactory.h"
#include <iostream>

using namespace SIP;
using namespace std;

StopFactoryI::StopFactoryI(Ice::ObjectAdapterPtr adapter) : adapter(adapter), stopsCreated(0) {}

shared_ptr <TramStopPrx> StopFactoryI::createStop(std::string name, const Ice::Current &current) {
    auto newStop = make_shared<TramStopI>(name);
    stopsCreated++;
    auto stopPrx = Ice::uncheckedCast<TramStopPrx>(adapter->addWithUUID(newStop));
    return stopPrx;
}

double StopFactoryI::getLoad(const Ice::Current &current) {
    return static_cast<double>(stopsCreated);
}
