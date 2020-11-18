#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//#define TRACE
//#define DBG

#define WITH_TRAY

#include <iostream>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include <fmodex/fmod.hpp>
#include <fmodex/fmod_errors.h>
#include <fmodex/fmod_dsp.h>
#include <fmodex/api/fmod_event.hpp>

#include <QApplication>
#include <QMouseEvent>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QIODevice>
#include <QTextCodec>
#include <QMutex>
#include <QProgressBar>
#include <QFileDialog>
#include <QWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QSizeGrip>
#include <QtWidgets/QGridLayout>
#ifdef WITH_TRAY
    #include <QSystemTrayIcon>
#endif

//********************************************************************************

//********************************************************************************
namespace Ui {
class MWindow;
}
using namespace std;

//********************************************************************************
void mkstr(int n, char *s);
//********************************************************************************
#ifdef TRACE

class TrLog
{
public:

    TrLog(const char* fileName, const char* funcName, int lineNumber);
    ~TrLog();

private:

    const char *_fileName;
    const char *_funcName;
};

#define TRACER TrLog TrLog(__FILE__ , __FUNCTION__ , __LINE__);

#endif
//--------------------------------------------------------------
class MWindow;
//--------------------------------------------------------------
class ContMenu : public QTextEdit
{
    Q_OBJECT

private:
    QMenu* cmenu;
    QStringList *lt;

public:
    ContMenu(QWidget *parent = nullptr, QStringList *lst = nullptr);
    ~ContMenu();

signals:
    void contmenu(int);

public slots:
    void slotAct(QAction* pAction);

    //friend class MWindow;
};
//--------------------------------------------------------------
class MWindow : public QMainWindow
{
    Q_OBJECT

public:

    class TheError {
        public :
            int code;
            FMOD_RESULT fmod_code;
            TheError(int);
            TheError(int,FMOD_RESULT);
    };

    explicit MWindow(QWidget *parent = nullptr, bool pm = true, bool dp = false);
    ~MWindow();

    void stop_play();
    void MkList();
    FMOD_RESULT CheckFMOD(FMOD_RESULT);
    void DelItems();

protected:
    virtual void timerEvent(QTimerEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;
    virtual void keyPressEvent(QKeyEvent *) override;
#ifdef WITH_TRAY
    virtual void changeEvent(QEvent *) override;
    virtual void closeEvent(QCloseEvent *) override;
#endif

signals:
    void cstoped();
    void contmenu(int);

public slots:
    void About();
    void list_media();
    void list_dir();
    void save_list();
    void open_list();
    void clear_media();
    void list_repeat();
    void play_media();
    void stop_media();
    void pause_media();
    void mute_media();
    void next();
    void UpdateStatus();
    void next_media();
    void prev_media();
    void GotoPos(int);
    void SetGain(int);
    void RowNum(int, int);
    void set_dsp_type();
    void select_cont_menu(int);

    //tray
#ifdef WITH_TRAY
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void trayActionExecute();
    void setTrayIconActions();
    void showTrayIcon();
#endif

private:
    //const char *ver = "2.3";
    //const char *ver = "2.3.1";//11.12.2018
    //const char *ver = "2.4.1";//30.01.2019 minor changes for new Qt version (5.12.0)
    //const char *ver = "2.4.2";//12.02.2019 minor changes for new Qt version (5.12.1)
    //const char *ver = "2.4.3";//18.03.2019 minor changes (remove temp. notes)
    //const char *ver = "2.5";//19.09.2019 minor changes : main window size update
    //const char *ver = "2.5.1";//20.09.2019 minor changes in ui
    //const char *ver = "2.6";//26.10.2019 major changes : add icon try mode
    const char *ver = "2.7";//18.11.2020 major changes : add keyPressed event support

    const char *ttip_head = "<html><head/><body><p><span style='font-size:8pt; font-style:italic; color:#0000ff';>";
    const char *ttip_tail = "</span></p></body></html";
    const float echo_delay = 250.0;//250ms
    const int time_interval = 1000;//1000ms = 1sec
    char *st;
    int tmr, MyError, list_ind, fmod_ver;
    bool pause, mute, play_flag, stop_flag, repeat, list_add, isStream;
    unsigned int start_pos, chan_pos, end_pos, start_play_tick;
    float gain;

    Ui::MWindow *ui;//interface form
    QString filename;
    QStringList list;
    QSize *sz22x22, *sz16x16;
    QList<QTableWidgetItem *> ptr_item;

    FMOD_RESULT f_result;
    FMOD::System *audio;
    FMOD::Sound *sound;
    FMOD::Channel *channel;
    FMOD_CREATESOUNDEXINFO *exinfo;
    FMOD::DSP *fft;
    bool dsp;
    int fft_ind, dsp_ind;
    QList<FMOD_DSP_TYPE> *dsp_list;
    QStringList *dsp_name;
    QString directory;

    QTableWidget *tbl;

    ContMenu *contm;

    QSizeGrip *sizeGrip;
    QGridLayout *layout;

    //tray
#ifdef WITH_TRAY
    QMenu *trayIconMenu;
    QAction *minA;
    QAction *maxA;
    QAction *quitA;
    QSystemTrayIcon *trayIcon;
#endif

};

#endif // MAINWINDOW_H
