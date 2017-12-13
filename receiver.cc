#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "paquete_m.h"

using namespace omnetpp;
const short mensajeACK = 1;
const short mensajeNACK = 2;

class receiver : public cSimpleModule{
private:
    int numSeq;
public:
    receiver();
protected:
    virtual void handleMessage(cMessage *msg);
    virtual void enviaNack(int nSeq);
    virtual void enviaAck(int nSeq);
};

Define_Module(receiver);

receiver::receiver(){
    numSeq = 0;
}

void receiver::handleMessage(cMessage *msg){
    paquete * pqt = check_and_cast<paquete *>(msg);//Hacemos el cast
    //Sacamos la seq del paquete
    int receivedSeq = pqt->getSeq();
    if(pqt->hasBitError()){
        enviaNack(receivedSeq);
    }
    else
    {
        enviaAck(receivedSeq);
    }

}
void receiver::enviaAck(int nSeq){
    //EV << "Enviando ack...";
    paquete * pqt = new paquete("ACK",1);
    pqt -> setBitLength(1);
    pqt -> setType(mensajeACK);
    send(pqt, "out");
}
void receiver::enviaNack(int nSeq){
    //EV << "Enviando nack...";
    paquete * pqt = new paquete("NACK",1);
    pqt -> setBitLength(1);
    pqt -> setType(mensajeNACK);
    send(pqt, "out");
}
