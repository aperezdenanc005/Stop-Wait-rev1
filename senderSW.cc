//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//


#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "paquete_m.h" //Esto es lo que se va a intercambiar

using namespace omnetpp;

//Maquina de estados para el sistema S&W
const short idle = 0;
const short sending = 1;
const short waitAck = 2;

//Defino los tipos de mensaje
const short mensajeACK = 1;
const short mensajeNACK = 2;


class senderSW : public cSimpleModule{
private:
    paquete *message;  // message that has to be re-sent on error
    cMessage *sent; //Paquete que se reenvía
    cChannel * txChannel;
    cQueue *txQueue;
    cQueue *noConfirmedQueue;
    int sendSeqNumber;
    /*Maquina de estados y estado*/
    short state_machine;
public:
    senderSW();
    virtual ~senderSW();

protected:
    virtual void sendCopyOf(paquete *msg);
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(senderSW);

senderSW::senderSW() {
    /*constructor*/
    state_machine = idle;
    sendSeqNumber = 0;
    sent = NULL;
    message = NULL;
    txQueue = NULL;
    txChannel = NULL;
}

senderSW::~senderSW() {
    /*Destructor*/
    cancelAndDelete(sent);
    txQueue->~cQueue(); //Liberamos la cola
    noConfirmedQueue->~cQueue(); //Liberamos la cola de no confirmados
    delete message; //Se elimina el paquete message
}

void senderSW::initialize(){
    txChannel = gate("out")->getTransmissionChannel(); //Inicializamos el canal. Enganchamos al canal.
    sent = new cMessage("sent");
    txQueue = new cQueue("txQueue");
    WATCH(state_machine);
}

void senderSW::handleMessage(cMessage *msg){
    EV << "Message";
    if(msg == sent){ //Si recibo el pqte que hemos enviado nosotros.
        /*el mensaje ya ha sido enviado*/
        state_machine=waitAck;
    }
    else //Si recibimos un paquete de fuera
    {
        //Tenemos que tratar lo recibido como un paquete
        paquete * pqt = check_and_cast<paquete *>(msg);
        if(msg->arrivedOn("inPacket")){
            EV << "handleMessage";
            //Si lo que llega es un paquete tengo que reenviarlo si no hay un paquete
            switch (state_machine) {
                case idle: //Si estoy esperando, reenvio el paquete que recibo.
                    //Creo paquete message que almacena el paquete. Elimino el mssge antiguo.
                    EV << "handleMessage: 1";
                    //delete(message);
                    EV << "handleMessage: 2";
                    message = pqt;
                    sendCopyOf(message);
                    break;
                case sending://Si estamos haciendo algo, solo podemos almacenar el paquete.
                    EV << "handleMessage: 3";
                    txQueue-> insert(pqt);
                    break;
                case waitAck:
                    txQueue-> insert(pqt);
                    break;
                default:
                    break;
            }

        }else{
            switch (pqt -> getType()) {
                case mensajeACK:
                    //Si llega un ACK compruebo la cola. Si esta vacía no hago nada y paso a estado idle y si tiene paquetes
                    //recojo los paquetes.
                    if(txQueue-> isEmpty()){
                        state_machine = idle;
                    }else{
                        delete(message);
                        message = (paquete *)txQueue -> pop();
                        sendCopyOf(message);
                    }
                    break;
                case mensajeNACK:
                    sendCopyOf(message);//Envio copia del mensaje fallido.
                    break;
                default:
                    break;
            }
        }
        delete(pqt);
    }
}

void senderSW::sendCopyOf(paquete *msg)
{
    /*Duplicar el mensaje y mandar una copia*/
    paquete *copy = (paquete *) msg->dup();
    send(copy, "out");
    state_machine=sending;
    simtime_t txFinishTime = txChannel->getTransmissionFinishTime(); //Pregunto al canal cuando va a terminar de enviar el mensaje
    scheduleAt(txFinishTime,sent);
}

