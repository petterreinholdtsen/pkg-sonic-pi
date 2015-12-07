#ifndef OSCHANDLER_H
#define OSCHANDLER_H

#include "oscpkt.hh"
#include "sonicpitheme.h"
#include "mainwindow.h"

class OscHandler
{

public:
    OscHandler(MainWindow *parent = 0, QTextEdit *out = 0, QTextEdit *error = 0, SonicPiTheme *theme = 0);
    void oscMessage(std::vector<char> buffer);
    bool signal_server_stop;
    bool server_started;

private:
    SonicPiTheme *theme;
    MainWindow *window;
    QTextEdit  *out;
    QTextEdit  *error;

    oscpkt::PacketReader pr;
    oscpkt::PacketWriter pw;
};

#endif // OSCHANDLER_H
