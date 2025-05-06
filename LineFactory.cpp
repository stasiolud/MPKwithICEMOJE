#include "LineFactory.h"
#include <iostream>

using namespace SIP;
using namespace std;

LineFactoryI::LineFactoryI(Ice::ObjectAdapterPtr adapter) : adapter(adapter), linesCreated(0) {}

shared_ptr <LinePrx> LineFactoryI::createLine(string name, const Ice::Current &current) {
    auto newLine = make_shared<LineI>(name);
    linesCreated++;
    auto linePrx = Ice::uncheckedCast<LinePrx>(adapter->addWithUUID(newLine));
    return linePrx;
}

double LineFactoryI::getLoad(const Ice::Current &current) {
    return static_cast<double>(linesCreated);
}
