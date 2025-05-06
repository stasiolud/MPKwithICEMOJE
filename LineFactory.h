#pragma once

#include <Ice/Ice.h>
#include "MPK.h"
#include "Line.h"
#include <memory>

using namespace std;
using namespace SIP;

class LineFactoryI : public SIP::LineFactory {
private:
    int linesCreated;
    Ice::ObjectAdapterPtr adapter;

public:
    LineFactoryI(Ice::ObjectAdapterPtr adapter);

    shared_ptr <LinePrx> createLine(string name, const Ice::Current &current) override;

    double getLoad(const Ice::Current &current = Ice::Current()) override;
};
