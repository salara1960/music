#include "mainwindow.h"
#include <errno.h>

int main(int argc, char *argv[])
{
int c_err=0;
bool pmode=false;//sound mode
bool d_s_p=true;//with dsp effects

#ifdef Q_WS_X11
    bool useGUI = getenv("DISPLAY") != 0;
#else
    bool useGUI = true;
#endif

    if (!useGUI) {
        cout << "Console version not support, bye.\n";
        return 1;
    }


    setlocale(LC_ALL,"UTF8");

    if (argc>1) {
        if (!strcmp(argv[1], "sound")) pmode = false;
        if (argc>2) {
            if (!strcmp(argv[2], "dsp")) d_s_p = true;
        }
    }

    try {
        QApplication music(argc, argv);

        MWindow wnd(NULL, pmode, d_s_p);

        //wnd.setModePlay(pmode);

        wnd.show();

        music.exec();
    }
    // Error handler block:
    // - 0x01 - memory error
    // - 0x02 - timer error
    // - 0x04 - Qstring::toInt error
    // - 0x08 - bad index
    // - 0x10 - bad pointer
    // - 0x20 - FMOD error
    // - 0x40 - old version fmodex
    catch (MWindow::TheError (er)) {
        c_err = er.code;
        if ((c_err>0) && (c_err<=0x40)) {
            if (c_err & 1) {
                perror("Error calloc function\n");
                perror(strerror(errno));
            }
            if (c_err & 2) cerr << "Error startTimer function (" << c_err << ")\n";
            if (c_err & 4) cerr << "Error 'Qstring::toInt' function (" << c_err << ")\n";
            if (c_err & 8) cerr << "Error index of array (" << c_err << ")\n";
            if (c_err & 0x10) cerr << "Error pointer (NULL)  (" << c_err << ")\n";
            if (c_err & 0x20) cerr << "FMOD error ("
                                   << er.fmod_code << ") : "
                                   << FMOD_ErrorString(er.fmod_code)
                                   << endl;
            if (c_err & 0x40) {
                char tmps[32]={0};
                cerr << "FMODex: You have old version :";
                mkstr(htonl((int)er.fmod_code), tmps);
                cerr << tmps << ", need version : ";
                mkstr(htonl((int)FMOD_VERSION), tmps);
                cerr << tmps << endl;
            }
        } else cerr << "Unknown Error (" << c_err << ")\n";
        return c_err;
    }
    catch (bad_alloc) {
        perror("Error while alloc memory via call function new\n");
        perror(strerror(errno));
        return -1;
    }
    catch (bad_exception) {
        perror("Error exception\n");
        return -1;
    }

    return 0;
}
