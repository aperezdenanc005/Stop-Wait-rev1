
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2003 Ahmet Sekercioglu
// Copyright (C) 2003-2015 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//


import ned.DatarateChannel;

//
// Here we make the model look a bit prettier in the GUI. We assign the
// "block/routing" icon to the simple module. All submodules of type
// Txc2 will use this icon by default
//
simple senderGBN
{
    parameters:
        @display("i=block/routing"); // add a default icon
    gates:
        input inPacket;
        input in;
        output out;
}

//
// Make the two module look a bit different with colorization effect.
// Use cyan for `tic', and yellow for `toc'.
//
simple receiver
{
    parameters:
        int capacity;

    gates:
        input in;
        output out;
}
simple Source
{
    parameters:
        @display("i=block/source");
    gates:
        output out;
}

network Tictoc2
{
    @display("bgb=379.75,155.82");
    types:
        channel C extends DatarateChannel
        {
            datarate=1bps;
            per=0.01;
        }
    submodules:
        source: Source {
            @display("p=14.7,14.7");
        }
        receiver: receiver {
            @display("p=267.05,64.68");
        }
        senderGBN: senderGBN {
            @display("p=120.05,59.78");
        }
    connections:
        source.out --> senderGBN.inPacket;
        receiver.out --> C --> senderGBN.in;
        senderGBN.out --> C --> receiver.in;
}

