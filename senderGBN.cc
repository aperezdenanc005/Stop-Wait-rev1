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
const short sending_in = 1; //En este estado compruebo la cola original
const short sending_rep = 2; //En este estado compruebo la cola de paquetes ya enviados

//Defino los tipos de mensaje
const short mensajeACK = 1;
const short mensajeNACK = 2;


class senderGBN : public cSimpleModule{
private:
    paquete *message;  // message that has to be re-sent on error
    cMessage *sent; //Paquete que se reenvía
    cChannel * txChannel;
    cQueue *txQueue;
    cQueue *nackQueue;
    int sendSeqNumber;
    int pqtToRepeat; //Paquetes en la cola de retx
    int totalRepeated; //Paquetes que han sido retx
    //int rpt_num;  //paquetes que se han recibido mal
    /*Maquina de estados y estado*/
    short state_machine;

    simtime_t txFinishTime;
    /****CASO 0 *******/
    double_t numSentOk; // ++ Cuando recibe ack
    double_t numSentNOK; // ++ Cuando recibe nack
    double_t numTotal;
    double_t tasaNeta;
    double_t tasaBruta;
    double_t tasaFallos;
    //long tasaTraspaso; //Contador que se modifica cuando se recibe un ack de la ventana y hay que eliminar paquetes enviados
public:
    senderGBN();
    virtual ~senderGBN();

protected:
    virtual void sendCopyOf(paquete *msg);
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    /** CASO 0 **/
    virtual void refreshDisplay() const;
};

Define_Module(senderGBN);

senderGBN::senderGBN() {
    /*constructor*/
    state_machine = idle;
    sendSeqNumber = 0;
    pqtToRepeat = 0;
    totalRepeated = 0;
    sent = NULL;
    message = NULL;
    txQueue = NULL;
    nackQueue = NULL;
    txChannel = NULL;
    txFinishTime = NULL;
    /*****CASO 0 ****/

    numSentNOK = 0;
    numSentOk = 0;
    numTotal = 0;
    tasaNeta = 0;
    tasaBruta = 0;
    tasaFallos = 0;

}

senderGBN::~senderGBN() {
    /*Destructor*/
    cancelAndDelete(sent);
    txQueue->~cQueue(); //Liberamos la cola
    nackQueue->~cQueue();
    delete message; //Se elimina el paquete message
}

void senderGBN::initialize(){
    txChannel = gate("out")->getTransmissionChannel(); //Inicializamos el canal. Enganchamos al canal.
    sent = new cMessage("sent");
    txQueue = new cQueue("txQueue");
    nackQueue = new cQueue("nackQueue");
    WATCH(state_machine);
}

void senderGBN::handleMessage(cMessage *msg){
    EV << "-Message-\n";
        if(msg == sent){ //Si recibo el pqte que hemos enviado nosotros.
            /*el mensaje ya ha sido enviado*/
                switch(state_machine)
                {
                    case sending_in:
                            /*Estado: paquete enviado, hay paquetes en cola*/
                    {
                        EV << "SENDING IN\n";
                            if(txQueue->isEmpty()){
                                state_machine = idle;
                            }else{
                                /*hay mensajes que enviar*/
                                message = (paquete *)txQueue -> pop();
                                paquete *copy = (paquete *) message->dup();
                                nackQueue -> insert(copy);
                                sendCopyOf(message);
                                state_machine = sending_in;
                            }
                            break;
                    }
                    case sending_rep:
                    {
                        if(pqtToRepeat == totalRepeated){ //Si ya se han enviado todos los que habían fallado
                            /*se ha enviado toda la repetición*/
                            /*Comprobar cola de trasmisión*/
                            EV << "SENDING REP\n";
                            if(txQueue->isEmpty()){
                                /*vuelve a estado idle*/
                                state_machine = idle;
                            }else{
                                /*hay mensajes que enviar*/
                                message = (paquete *)txQueue->pop();
                                paquete *copy = (paquete *) message->dup();
                                nackQueue -> insert(copy);
                                sendCopyOf(message);
                                state_machine = sending_in;
                            }
                        }
                        else{//No se han enviado todos los que habían fallado
                            message = (paquete *)nackQueue->get(pqtToRepeat++); //Coges el paquete y despues sumas el indice
                            sendCopyOf(message);
                            state_machine = sending_rep;
                        }
                        break;
                    }
                    default:
                        //delete(message);
                        return;
                        break;
                }
        }
        else //Si recibimos un paquete de fuera
        {
            //Tenemos que tratar lo recibido como un paquete
            paquete * pqt = check_and_cast<paquete *>(msg);
            if(msg->arrivedOn("inPacket")){
                EV << "handleMessage\n";
                //Si lo que llega es un paquete tengo que reenviarlo si no hay un paquete
                switch (state_machine) {
                    case idle: //Si estoy esperando, reenvio el paquete que recibo.
                        //Creo paquete message que almacena el paquete. Elimino el mssge antiguo.
                    {
                        EV << "handleMessage: 1\n";
                        //delete(message);
                        EV << "handleMessage: 2\n";
                        message = pqt;
                        paquete *copy = (paquete *) message->dup();
                        nackQueue -> insert(copy);
                        sendCopyOf(message);
                        state_machine = sending_in;
                        break;
                    }
                    case sending_in://Si estamos haciendo algo, solo podemos almacenar el paquete.
                    {
                        EV << "handleMessage: 3\n";
                        txQueue-> insert(pqt);
                        break;
                    }
                    case sending_rep:
                    {
                        txQueue-> insert(pqt);
                        break;
                    }
                    default:
                        break;
                }

            }
            else
            {
                switch (pqt -> getType()) {
                    case mensajeACK:
                    {
                        EV << "mensaje ACK\n";
                        //Si llega un ACK se libera el paquete de la cola nack y ajustamos variables enteros que controlan las posiciones en la cola en la que estoy
                        paquete * paq = (paquete*)nackQueue->pop(); //No haria falta guardarlo pero da problemas, asi que lo elimino
                        delete(paq);
                        -- pqtToRepeat; //Esto se disminuye para ajustar los punteros de las posiciones de colas
                        -- totalRepeated;
                        numSentOk ++;
                        tasaBruta = numSentOk / txFinishTime.dbl();
                        tasaNeta ++;
                        break;
                    }
                    case mensajeNACK:
                    {
                        EV << "mensaje NACK\n\n";
                        //Si recibo el nack tengo que enviar las repeticiones
                        totalRepeated = nackQueue->getLength();

                        pqtToRepeat = 0;
                        //Ahora se comprueba la maquina de estados para ver si podemos enviar ya o espero a recibir un sent.
                        //Si estoy en idle puedo enviar ya, si estoy en sending tengo que esperar.
                        if(state_machine == idle){
                            paquete * paq = (paquete *)nackQueue ->get(pqtToRepeat++);
                            sendCopyOf(paq);
                        }
                        numSentNOK ++;
                        tasaFallos = numSentNOK / txFinishTime.dbl();
                        state_machine = sending_rep;

                        break;
                    }
                    default:
                        break;
                }
            }
    }
}

void senderGBN::sendCopyOf(paquete *msg)
{
    /*Duplicar el mensaje y mandar una copia*/
    paquete *copy = (paquete *) msg->dup();
    send(copy, "out");
    txFinishTime = txChannel->getTransmissionFinishTime(); //Pregunto al canal cuando va a terminar de enviar el mensaje
    scheduleAt(txFinishTime+1,sent);
    numTotal ++;
    tasaBruta = numTotal/ txFinishTime.dbl();
}

void senderGBN::refreshDisplay() const
{
    char buf[60];
    sprintf(buf, "PacketRate: %lf RealPacketRate: %lf FailedPacketRate %lf", tasaBruta, tasaNeta, tasaFallos);
    getDisplayString().setTagArg("t", 0, buf);
}



