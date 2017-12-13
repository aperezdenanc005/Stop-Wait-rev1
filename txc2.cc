//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2003 Ahmet Sekercioglu
// Copyright (C) 2003-2015 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

/**
 * In this class we add some debug messages to Txc1. When you run the
 * simulation in the OMNeT++ GUI Tkenv, the output will appear in
 * the main text window, and you can also open separate output windows
 * for `tic' and `toc'.
 */
class Txc2 : public cSimpleModule
{
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(Txc2);

void Txc2::initialize()
{
    if (strcmp("tic", getName()) == 0) {
        // The `ev' object works like `cout' in C++.
        EV << "Sending initial message\n";
        cPacket *msg = new cPacket("tictocMsg",0,10240);
        send(msg, "out");
    }
}



void Txc2::handleMessage(cMessage *msg)
{
    // msg->getName() is name of the msg object, here it will be "tictocMsg".
    //
   //

    if(strcmp("toc",getName())==0 && ((cPacket*)msg)->hasBitError()==true) {
        cPacket *msgNACK = new cPacket("NACK",0,1024);
        send(msgNACK,"out");
    }

    else if(strcmp("toc",getName())==0 && ((cPacket*)msg)->hasBitError()==false) {
            cPacket *msgACK = new cPacket("ACK",0,1024);
            send(msgACK,"out");
    }
    else {
        cPacket *msgTIC = new cPacket("TIC",0,10240);
        send(msgTIC,"out");
    }
}

