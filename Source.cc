//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2006-2015 OpenSim Ltd.
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//
#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "paquete_m.h"
using namespace omnetpp;

class Source: public cSimpleModule
{
    private:
        paquete * nuevoPqt;
        simtime_t startTime;
        int seq;
    public:
        virtual ~Source();
    protected:

        virtual void handleMessage(cMessage *msg) override;
        virtual paquete * generaPaquete();
        virtual void initialize() override;

};

Define_Module(Source);

Source::~Source(){
    cancelAndDelete(nuevoPqt);
}
void Source::initialize(){
    startTime=20;
    nuevoPqt = new paquete();
    scheduleAt(startTime, nuevoPqt);
    seq=0;
}
void Source::handleMessage(cMessage * msg){
    paquete *pqt = generaPaquete();
    send(pqt,"out");
    scheduleAt(simTime()+exponential(startTime),nuevoPqt);
}

paquete * Source::generaPaquete(){
    char nombrePaquete[15];
    sprintf(nombrePaquete,"msg-%d",seq++);
    paquete * msg = new paquete(nombrePaquete,0);
    msg -> setBitLength(1024);
    return msg;
}

