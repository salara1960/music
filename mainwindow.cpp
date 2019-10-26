#include "mainwindow.h"
#include "ui_mainwindow.h"

//--------------------------------------------------------------------------------
const int max_st = 2048;

const QString main_pic    = "png/main.png";

const QString hide_pic    = "png/hide.png";
const QString show_pic    = "png/show.png";
const QString close_pic   = "png/close.png";
//--------------------------------------------------------------------------------
void mkstr(int n, char *s)
{
    if (!s) return;

    const size_t sz = 4;
    char *sta = static_cast<char *>(calloc(1, sz << 2));// *4
    if (sta) {
        memset(s, 0, strlen(s));
        unsigned char mas[sz];
        memcpy(mas, &n, sz);
        for (size_t i = 0; i < sz; i++)
            if (mas[i]) sprintf(sta+strlen(sta), "%x.", mas[i]);
        memcpy(s, sta, strlen(sta));
        free(sta);
    }
}
//--------------------------------------------------------------------------------
#ifdef TRACE
static QByteArray sps="";
//                          class TLog
TrLog::TrLog(const char *fileName, const char *funcName, int lineNumber)
{
    _fileName = fileName;
    _funcName = funcName;
    cerr << sps.data() << "Entering " << _funcName << "() - (" << _fileName << ":" << lineNumber << ")\n";
    sps.append("  ");

}
TrLog::~TrLog()
{
    sps.resize(sps.length() -2);
    cerr << sps.data() << "Leaving  " << _funcName << "() - (" << _fileName << ")\n";
}
#endif
//--------------------------------------------------------------------------------
ContMenu::ContMenu(QWidget *parent, QStringList *lst) : QTextEdit(parent)
{
    if (!lst) return;

    lt = lst;
    setReadOnly(true);
    cmenu = new QMenu(parent);
    QList<QString>::iterator it(lt->begin());
    while (it != lt->end()) cmenu->addAction(*it++);
    connect(cmenu, SIGNAL(triggered(QAction*)), SLOT(slotAct(QAction*)));
    connect(this, SIGNAL(contmenu(int)), parent, SLOT(select_cont_menu(int)));
    QRect *rr_src = new QRect (parent->geometry());
    QRect *rr_dst = new QRect (cmenu->geometry());
    cmenu->setGeometry(rr_src->left() + rr_src->width() - 40 ,
                       rr_src->bottom() - 40,
                       rr_dst->width(),
                       rr_dst->height());
    delete rr_dst;
    delete rr_src;

    cmenu->exec();
}
//--------------------------------------------------------------------------------
ContMenu::~ContMenu()
{
    delete cmenu;
}
//--------------------------------------------------------------------------------
void ContMenu::slotAct(QAction *pAction)
{
    if (!lt) return;

    QString dsptype = pAction->text();
    int sel = -1;
    for (int i = 0; i < lt->length(); i++)
        if (dsptype == lt->at(i)) {
            sel = i;
            break;
        }
    if (sel != -1) emit contmenu(sel);

}
//--------------------------------------------------------------------------------
//                         class MWindow
MWindow::MWindow(QWidget *parent, bool pm, bool dp) : QMainWindow(parent), ui(new Ui::MWindow)
{
#ifdef TRACE
    TRACER
#endif

    ui->setupUi(this);
//    this->setFixedSize(this->size());

    this->setTrayIconActions();
    this->showTrayIcon();

    MyError = 0;      //Code error for catch block
    st = nullptr;     //pointer to temp char_string
    layout = nullptr;
    sizeGrip = nullptr;

    filename.clear();
    list.clear();
    list_add = pause = mute = repeat = false;
    isStream = pm;
    audio = nullptr;
    sound = nullptr;
    channel = nullptr;
    fft = nullptr;
    fft_ind = 0;
    dsp_list = nullptr;
    dsp_name = nullptr;
    dsp = dp;
    if (dsp) {
        dsp_list = new QList<FMOD_DSP_TYPE>;
        *dsp_list << FMOD_DSP_TYPE_NORMALIZE
                  << FMOD_DSP_TYPE_MIXER
                  << FMOD_DSP_TYPE_ECHO
                  << FMOD_DSP_TYPE_HIGHPASS
                  << FMOD_DSP_TYPE_LOWPASS
                  << FMOD_DSP_TYPE_CHORUS
                  << FMOD_DSP_TYPE_COMPRESSOR
                  << FMOD_DSP_TYPE_DISTORTION
                  << FMOD_DSP_TYPE_PARAMEQ
                  << FMOD_DSP_TYPE_FLANGE
                  << FMOD_DSP_TYPE_PITCHSHIFT
                  << FMOD_DSP_TYPE_SFXREVERB
                  << FMOD_DSP_TYPE_TREMOLO;
        dsp_name = new QStringList();
        *dsp_name << "Normalize"
                  << "Mixer"
                  << "Echo"
                  << "HighPass"
                  << "LowPass"
                  << "Chorus"
                  << "Compressor"
                  << "Distortion"
                  << "Parameq"
                  << "Flange"
                  << "PitchShift"
                  << "SfxReverb"
                  << "Tremolo";
        dsp_ind = 0;
    }
    ptr_item.clear();

    tbl = nullptr;

    list_ind = 0;
    chan_pos = 0;
    end_pos = chan_pos -1;
    exinfo = nullptr;
    gain = 0.5;
    sz22x22 = new QSize (19, 19);
    sz16x16 = new QSize (16, 16);

    ui->setupUi(this);

    st = static_cast<char *>(calloc(1, max_st));//get memory for temp char_string and fill zero
    if (!st) {
        MyError |= 1;//memory error
        throw TheError(MyError);
    }

    tmr = startTimer(time_interval);
    if (tmr <= 0) {
        MyError |= 2;//timer error
        throw TheError(MyError);
    }

    ui->pushButtonRepeat->setIconSize(*sz16x16);
    ui->pushButtonPlay->setIconSize(*sz16x16);
    ui->pushButtonStop->setIconSize(*sz16x16);
    ui->pushButtonPause->setIconSize(*sz16x16);
    ui->pushButtonMute->setIconSize(*sz16x16);
    ui->pushButtonNext->setIconSize(*sz16x16);
    ui->pushButtonPrev->setIconSize(*sz16x16);
    ui->pushButtonAdd->setIconSize(*sz22x22);

    if (!dsp) ui->pushButtonDSP->hide();
    else {
        ui->pushButtonDSP->setIconSize(*sz16x16);
        ui->pushButtonDSP->setToolTip(ttip_head + dsp_name->at(dsp_ind) + ttip_tail);
    }

    //Menu
    connect(ui->actionVersion, SIGNAL(triggered(bool)), this, SLOT(About()));
    connect(ui->actionOpenSrc, SIGNAL(triggered(bool)), this, SLOT(list_media()));
    connect(ui->actionClearSrc, SIGNAL(triggered(bool)), this, SLOT(clear_media()));
    connect(ui->actionSavePlt, SIGNAL(triggered(bool)), this, SLOT(save_list()));
    connect(ui->actionOpenPlt, SIGNAL(triggered(bool)), this, SLOT(open_list()));
    //Media
    connect(ui->pushButtonPlay, SIGNAL(clicked(bool)) , this, SLOT(play_media()));
    connect(ui->pushButtonStop, SIGNAL(clicked(bool)), this, SLOT(stop_media()));
    connect(ui->pushButtonPause, SIGNAL(clicked(bool)), this, SLOT(pause_media()));
    connect(ui->pushButtonMute, SIGNAL(clicked(bool)), this, SLOT(mute_media()));
    connect(this, SIGNAL(cstoped()), this, SLOT(next()));
    connect(ui->pushButtonNext, SIGNAL(clicked(bool)), this, SLOT(next_media()));
    connect(ui->pushButtonPrev, SIGNAL(clicked(bool)), this, SLOT(prev_media()));
    connect(ui->pushButtonRepeat, SIGNAL(clicked(bool)), this, SLOT(list_repeat()));
    connect(ui->pushButtonAdd, SIGNAL(clicked(bool)), this, SLOT(list_dir()/*add_list_media()*/));
    connect(ui->pushButtonDSP, SIGNAL(clicked(bool)), this, SLOT(set_dsp_type()));

    //FMOD :
    f_result = FMOD::System_Create(&audio);
    if (f_result != FMOD_OK) {
        sprintf(st, "Error %d -> %s\n", f_result, FMOD_ErrorString(f_result));
        MyError |= 0x20;//FMOD error
        cerr << st;
        throw TheError(MyError, f_result);
    } else {
        unsigned int vr;
        f_result = audio->getVersion(&vr);
        CheckFMOD(f_result);
        fmod_ver = static_cast<int>(vr);
        if (vr < FMOD_VERSION) {
            MyError |= 0x40;//FMOD error
            throw TheError(MyError, static_cast<FMOD_RESULT>(vr));
        }

        f_result = audio->init(2, FMOD_INIT_NORMAL, nullptr);//FMOD_INIT_SOFTWARE_HRTF//FMOD_INIT_NORMAL
        CheckFMOD(f_result);

        sprintf(st, "Ready :");
        ui->MStatus->setText(trUtf8(st));
    }

    ui->vSlider->setRange(0, time_interval * 10);
    ui->vSlider->setValue(time_interval * 5);
    connect(ui->vSlider, SIGNAL(valueChanged(int)), this, SLOT(SetGain(int)));

    setWindowTitle("Music player");

}
//--------------------------------------------------------------------------------
MWindow::~MWindow()
{
    if (fft) fft->release();
    if (audio) audio->release();
    if (dsp_list) delete dsp_list;
    if (dsp_name) delete dsp_name;

    DelItems();

    delete sz22x22;
    delete sz16x16;
    delete tbl;
    delete ui;

    if (layout) delete layout;
    if (sizeGrip) delete sizeGrip;

    if (tmr > 0) killTimer(tmr);

    if (st) free(st);
}
//--------------------------------------------------------------------------------
void MWindow::resizeEvent(QResizeEvent *e)
{
    if (tbl) {
        QRect *rr = new QRect(this->ui->wida->geometry());

        int maxw = rr->width()-10;
        rr->setWidth(maxw);
        rr->setHeight(rr->height()-10);
        rr->setX(rr->x() - 8);
        rr->setY(rr->y() - 8);

        tbl->setGeometry(*rr);

        delete rr;

        if (tbl->columnWidth(0) < maxw) tbl->setColumnWidth(0, maxw);
    }
}
//--------------------------------------------------------------------------------
void MWindow::set_dsp_type()
{
    if (!dsp) return;
    contm = new ContMenu(this, dsp_name);
}
//--------------------------------------------------------------------------------
void MWindow::select_cont_menu(int sind)
{
    dsp_ind = sind;
    if (channel) {
        bool pl = false;
        f_result = channel->isPlaying(&pl);
        CheckFMOD(f_result);
        if (pl) {
            channel->setPaused(true);
            if (fft) {
                fft->release();
                fft = nullptr;
            }
            f_result = audio->createDSPByType(dsp_list->at(dsp_ind), &fft);
            CheckFMOD(f_result);
            if (dsp_list->at(dsp_ind) == FMOD_DSP_TYPE_ECHO) {
                f_result = fft->setParameter(FMOD_DSP_ECHO_DELAY, echo_delay);//250ms
                CheckFMOD(f_result);
            }
            f_result = channel->addDSP(fft, nullptr);
            CheckFMOD(f_result);
            fft->setActive(true);
            channel->setPaused(pause);
            audio->update();
        }
    }
    ui->pushButtonDSP->setToolTip(ttip_head + dsp_name->at(dsp_ind) + ttip_tail);
}
//--------------------------------------------------------------------------------
void MWindow::DelItems()
{
    if (!tbl) return;

    for (int i = 0; i < ptr_item.size(); i++) {
        if (ptr_item.at(i)) delete ptr_item.at(i);
    }
    ptr_item.clear();
}
//--------------------------------------------------------------------------------
FMOD_RESULT MWindow::CheckFMOD(FMOD_RESULT res)
{
    if (res != FMOD_OK) {
        switch (res) {
            case FMOD_ERR_FORMAT :
            case FMOD_ERR_FILE_NOTFOUND :
                QMessageBox::information(this, "FMOD Error", FMOD_ErrorString(res));
            break;
                default :
                    MyError |= 0x20;
                    throw TheError(MyError, res);
        }
    }
    return res;
}
//--------------------------------------------------------------------------------
void MWindow::About()
{
    sprintf(st, "\nMusic player version %s\n", ver);


    sprintf(st+strlen(st), "    Mode play ");
    if (isStream) sprintf(st+strlen(st), "'Stream'");
            else  sprintf(st+strlen(st), "'Sound'");

    if (dsp) {
        QByteArray *darr = new QByteArray(dsp_name->at(dsp_ind).toLocal8Bit());
        sprintf(st+strlen(st), "\n    DSP '%s'\n", darr->data());
        delete darr;
    }

    sprintf(st+strlen(st), "used : Qt v.%s + FMODex API v.", QT_VERSION_STR);
    uint32_t v = htonl(static_cast<uint32_t>(fmod_ver));
    const size_t dl = sizeof(int);
    unsigned char mas[dl], fst = 1;
    memcpy(&mas[0], &v, dl);
    for (size_t i = 0; i < dl; i++) {
        if (mas[i]) {
            fst = 0;
            sprintf(st+strlen(st), "%x.", mas[i]);
        } else {
            if (!fst) sprintf(st+strlen(st), "%x.", mas[i]);
        }
    }
    sprintf(st+strlen(st), "\n");

    QMessageBox::information(this, "About this", trUtf8(st));


}
//--------------------------------------------------------------------------------
MWindow::TheError::TheError(int err)
{
    code = err;
    fmod_code = FMOD_OK;
}
//--------------------------------------------------------------------------------
MWindow::TheError::TheError(int err, FMOD_RESULT ferr)
{
    code = err;
    fmod_code = ferr;
}
//--------------------------------------------------------------------------------
void MWindow::timerEvent(QTimerEvent *event)
{
    if (tmr == event->timerId()) {
        channel->getPosition(&chan_pos, FMOD_TIMEUNIT_MS);
        bool pl; channel->isPlaying(&pl);
        if ((pl) && (!pause)) {
            uint32_t min = (end_pos/static_cast<uint32_t>(time_interval) ) - start_play_tick;
            uint32_t chas = min/60;
            min %= 60;
            sprintf(st, "%02d:%02d", chas, min);
            ui->label->setText(tr(st));

            start_play_tick++;
            ui->hSlider->setValue( static_cast<int>(start_play_tick) );
        }
        if (chan_pos>=end_pos) {
            stop_media();
            if (list_ind < list.size()) emit cstoped();
        }
    }
}
//--------------------------------------------------------------------------------
void MWindow::clear_media()
{
    if (!list.size()) return;

    bool pl; channel->isPlaying(&pl);
    if (pl) stop_media();
    if (tbl) {
        DelItems();
        tbl->hide();
        delete tbl;
        tbl = nullptr;
    }
    list.clear();
    list_ind = 0;
    filename = "";
    UpdateStatus();
}
//--------------------------------------------------------------------------------
void MWindow::save_list()
{
    if (!list.size()) return;

    QString fileNamePlayList = QFileDialog::getSaveFileName(this,
                                tr("Save File"),
                                "pl.plt",
                                tr("PlayList (*.plt)"));
    if (!fileNamePlayList.size()) return;

    QFile fil(fileNamePlayList);
    bool flag = fil.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    if (flag) {
        QByteArray *mas = new QByteArray();
        QStringListIterator j_list(list);
        while (j_list.hasNext()) mas->append(j_list.next().toLocal8Bit() + "\n");
        fil.write(mas->data());
        fil.close();
        delete mas;
    }
}
//--------------------------------------------------------------------------------
void MWindow::open_list()
{
    QString *nm = new QString();
    *nm = QFileDialog::getOpenFileName(this,
                                       tr("Open PlayList"),
                                       nullptr,
                                       tr("PlayList (*.plt)"));
    if (!nm->size()) {
        ui->MStatus->setText(tr("No playlist file selected"));
        delete nm;
        return;
    }

    QFile fil(*nm);
    QStringList *ls = new QStringList();
    char *str = static_cast<char *>(calloc(1, 256));
    qint64 dl;
    bool flag = fil.open(QIODevice::ReadOnly | QIODevice::Text);
    if (flag) {
        while (!fil.atEnd()) {
            memset(str, 0, 256);
            dl = fil.readLine(str, 255);
            if ((dl > 0) && (dl < 256)) {
                if (strchr(str,'\n')) *(str + dl - 1) = 0;
                ls->append(str);
            }
        }
        fil.close();
    } else ui->MStatus->setText(tr("Error open playlist file ") + *nm);
    free(str);

    flag = false;
    if (list.size() > 0) list += *ls;
    else {
        list = *ls;
        flag = true;
    }
    delete ls;
    delete nm;
    MkList();
    if (flag) {
        list_ind = 0;
        filename = list.at(list_ind);
        UpdateStatus();
        play_media();
    } else if (tbl) tbl->selectRow(list_ind);

}
//--------------------------------------------------------------------------------
void MWindow::UpdateStatus()
{
    if (filename.size()) {
        QString *tmp = new QString;
        tmp->sprintf("%d: ", list_ind + 1);
        ui->MStatus->setText((*tmp) + filename);
        delete tmp;
    } else ui->MStatus->clear();
}
//--------------------------------------------------------------------------------
void MWindow::list_repeat()
{
    QSize *sz;
    repeat = !repeat;
    if (repeat) {
        sz = sz22x22;
        ui->pushButtonRepeat->setIcon(QIcon(QPixmap(":png/player_repeat2.png")));
    } else {
        sz = sz16x16;
        ui->pushButtonRepeat->setIcon(QIcon(QPixmap(":png/player_repeat1.png")));
    }
    ui->pushButtonRepeat->setIconSize(*sz);

}
//--------------------------------------------------------------------------------
void MWindow::MkList()
{
    if (tbl) {
        DelItems();
        delete tbl;
        tbl = nullptr;
    }
    tbl = new QTableWidget(this->ui->wida);

    QPalette *pal = new QPalette(this->ui->MStatus->palette());
    tbl->setPalette(*pal);
    delete pal;

    QFont *ft = new QFont(this->ui->MStatus->font());
    tbl->setFont(*ft);
    delete ft;

    QRect *rr = new QRect(this->ui->wida->geometry());
    int max_width = rr->width()-10;
    rr->setWidth(max_width);
    rr->setHeight(rr->height()-10);
    rr->setX(rr->x() - 8);
    rr->setY(rr->y() - 8);
    tbl->setGeometry(*rr);
    delete rr;

    tbl->setRowCount(list.size());
    tbl->setColumnCount(1);
    //for ver.5.11.2
    tbl->horizontalHeader()->setVisible(false);
    tbl->verticalHeader()->setVisible(false);
    //for ver.5.5.1
    //tbl->horizontalHeader()->hide();
    //tbl->verticalHeader()->hide();
    QString *tmp = new QString();
    QStringList *lt = new QStringList();
    tbl->resizeRowsToContents();
    tbl->setGridStyle(Qt::NoPen);
    for (int row = 0; row < tbl->rowCount(); row++) {
        QTableWidgetItem *item = new QTableWidgetItem();
        ptr_item << item;
        *tmp = list.at(row);
        lt->clear();
        *lt = tmp->split("/");
        tmp->sprintf("%d: ", row + 1);
        tmp->append(lt->at(lt->length() - 1));
        item->setText(*tmp);
        item->setToolTip(*tmp);
        tbl->setItem(row, 0, item);
        tbl->resizeColumnToContents(0);
        if (tbl->columnWidth(0) < max_width) tbl->setColumnWidth(0, max_width);
    }
    delete lt;
    delete tmp;
/**/
    tbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//    tbl->setWindowFlags(Qt::SubWindow);
//    //QSizeGrip *
//            sizeGrip = new QSizeGrip(tbl);
//    //QGridLayout *
//            layout = new QGridLayout(tbl);
//    layout->addWidget(sizeGrip, 0,0,2,2,Qt::AlignBottom | Qt::AlignRight);
/**/
    connect(tbl, SIGNAL(cellClicked(int,int)), this, SLOT(RowNum(int, int)));
    tbl->show();
    tbl->update();
}
//--------------------------------------------------------------------------------
void MWindow::list_media()
{
    QStringList *ls = new QStringList();
    *ls = QFileDialog::getOpenFileNames(this,
                                        tr("Open Files"),
                                        nullptr,
                                        tr("Music (*.mp3 *.MP3 *.mpga *.wav *.WAV *.flac *.FLAC *.wma *.WMA *.ogg *.OGG)"));
    if (!ls->size()) {
        ui->MStatus->setText(tr("No file selected"));
        delete ls;
        return;
    }
    bool play = false;
    if (!list.size()) {
        play = true;
        list = *ls;
    } else list += *ls;
    delete ls;

    MkList();

    if (play) {
        list_ind = 0;
        filename = list.at(list_ind);
        UpdateStatus();
        play_media();
    } else tbl->selectRow(list_ind);

}
//--------------------------------------------------------------------------------
void MWindow::list_dir()
{

    directory = QFileDialog::getExistingDirectory(this, tr("Select folder with media files"));
    if (directory.isEmpty()) {
        ui->MStatus->setText(tr("Folder ") + directory + tr(" is empty"));
        return;
    }

    QStringList *ls = new QStringList;
    QString dirname(directory + "/");

    QDir *dir = new QDir(directory);
    *ls = dir->entryList(QStringList()  << "*.mp3"
                                        << "*.MP3"
                                        << "*.flac"
                                        << "*.FLAC"
                                        <<"*.wav"
                                        << "*.WAV"
                                        << "*.wma"
                                        << "*.WMA"
                                        << "*.ogg"
                                        << "*.OGG",
                         QDir::Files);
    delete dir;

    if (!ls->size()) {
        ui->MStatus->setText(tr("No media files in folder ") + dirname);
        delete ls;
        return;
    }
    QList<QString>::iterator it(ls->begin());
    while (it != ls->end()) {
        *it = dirname + *it;
        it++;
    }

    bool play=false;
    if (!list.size()) {
        play = true;
        list = *ls;
    } else list += *ls;
    delete ls;

    MkList();

    if (play) {
        list_ind = 0;
        filename = list.at(list_ind);
        UpdateStatus();
        play_media();
    } else tbl->selectRow(list_ind);

}
//--------------------------------------------------------------------------------
void MWindow::RowNum(int r, int c)
{
    if (c) return;
    bool up = false;
    if ((r >= 0) && (r < list.size()) && (r != list_ind)) {
        up = true;
        list_ind = r;
        filename = list.at(list_ind);
        stop_media();
        play_media();
    } else if (r == list_ind) {
        up = true;
        list_ind = r;
        filename = list.at(list_ind);
    }
    if (up) UpdateStatus();
}
//--------------------------------------------------------------------------------
void MWindow::next()
{
bool plt = true;

    list_ind++;
    if (list_ind >= list.size()) {
        if (!repeat) plt = false; else list_ind = 0;
    }
    if (plt) {
        filename = list.at(list_ind);
        UpdateStatus();
        play_media();
    } else list_ind = 0;
}
//--------------------------------------------------------------------------------
void MWindow::stop_play()
{
    stop_media();
    filename = list.at(list_ind);
    UpdateStatus();
    play_media();
}
//--------------------------------------------------------------------------------
void MWindow::next_media()
{
    if (list.size()) {
        list_ind++;
        if (list_ind >= list.size()) list_ind = 0;
        stop_play();
    }
}
//--------------------------------------------------------------------------------
void MWindow::prev_media()
{
    if (list.size()) {
        list_ind--;
        if (list_ind < 0) list_ind = list.size() - 1;
        stop_play();
    }
}
//--------------------------------------------------------------------------------
void MWindow::GotoPos(int pos)
{
    start_play_tick = static_cast<uint32_t>(pos);
    channel->setPosition(start_pos + ( static_cast<uint32_t>(pos) * static_cast<uint32_t>(time_interval)), FMOD_TIMEUNIT_MS);
}
//--------------------------------------------------------------------------------
void MWindow::SetGain(int aten)
{
    gain = aten;
    gain /= (time_interval * 10);
    channel->setVolume(gain);
}
//--------------------------------------------------------------------------------
void MWindow::play_media()
{
    if (filename != "") {
        UpdateStatus();
        if (audio) {
            tbl->selectRow(list_ind);

            QByteArray namef(filename.toLocal8Bit());
            char *uk = namef.data();
            if (!isStream)
                f_result = audio->createSound(static_cast<const char *>(uk), FMOD_SOFTWARE, exinfo, &sound);
            else
                f_result = audio->createStream(static_cast<const char *>(uk),FMOD_SOFTWARE, exinfo, &sound);
            if (CheckFMOD(f_result) == FMOD_OK) {

                f_result = audio->playSound( static_cast<FMOD_CHANNELINDEX>(1), sound, true, &channel);
                CheckFMOD(f_result);

                if (dsp) {
                    if (fft) {
                        fft->release();
                        fft = nullptr;
                    }
                    f_result = audio->createDSPByType(dsp_list->at(dsp_ind), &fft);
                    CheckFMOD(f_result);
                    if (dsp_list->at(dsp_ind) == FMOD_DSP_TYPE_ECHO) {
                        f_result = fft->setParameter(FMOD_DSP_ECHO_DELAY, echo_delay);//250ms
                        CheckFMOD(f_result);
                    }
                    f_result = channel->addDSP(fft, nullptr);
                    CheckFMOD(f_result);
                    fft->setActive(true);
                    ui->pushButtonDSP->setToolTip(ttip_head + dsp_name->at(dsp_ind) + ttip_tail);
                }
                channel->setPaused(pause);

                channel->getPosition(&start_pos, FMOD_TIMEUNIT_MS);
                channel->setVolume(gain);
                channel->setPan(0);
                chan_pos = start_pos;
                sound->getLength(&end_pos, FMOD_TIMEUNIT_MS);
                ui->pushButtonPlay->setIconSize(*sz22x22);
                ui->pushButtonStop->setIconSize(*sz16x16);
                ui->hSlider->setRange(0, (static_cast<int>(end_pos))/time_interval);
                start_play_tick = 0;

                connect(ui->hSlider, SIGNAL(valueChanged(int)), this, SLOT(GotoPos(int)));

                f_result = audio->update();
                CheckFMOD(f_result);
            } else stop_media();
        } else ui->MStatus->setText(trUtf8("Audio_system not init"));
    } else {
        ui->MStatus->setText(trUtf8("No file selected"));
#ifdef DBG
        qDebug() << "No file selected";
    }
    qDebug() << "play: mute=" << mute << " pause=" << pause;
#else
    }
#endif
}
//--------------------------------------------------------------------------------
void MWindow::stop_media()
{
    if (dsp) {
        if (fft) {
            fft->release();
            fft = nullptr;
        }
    }
    channel->stop();
    channel = nullptr;
    if (sound) {
        sound->release();
        sound = nullptr;
    }
    pause = mute = false;

    ui->pushButtonStop->setIconSize(*sz22x22);
    ui->pushButtonPlay->setIconSize(*sz16x16);
    start_pos = chan_pos = 0;
    end_pos = chan_pos -1;

    disconnect(ui->hSlider, SIGNAL(valueChanged(int)), this, SLOT(GotoPos(int)));
    start_play_tick = 0;
    ui->hSlider->setValue(0);
    ui->label->clear();
#ifdef DBG
    qDebug() << "stop: mute=" << mute << " pause=" << pause;
#endif
}
//--------------------------------------------------------------------------------
void MWindow::pause_media()
{
    pause = !pause;
    channel->setPaused(pause);
    QSize *sz;
    if (pause) sz = sz22x22; else sz = sz16x16;
    ui->pushButtonPause->setIconSize(*sz);
#ifdef DBG
    qDebug() << "pause=" << pause;
#endif
}
//--------------------------------------------------------------------------------
void MWindow::mute_media()
{
    mute = !mute;
    channel->setMute(mute);
    QSize *sz;
    if (mute) {
        sz = sz22x22;
        ui->pushButtonMute->setIcon(QIcon(QPixmap(":png/mute_on.png")));
    } else {
        sz = sz16x16;
        ui->pushButtonMute->setIcon(QIcon(QPixmap(":png/mute_off.png")));
    }
    ui->pushButtonMute->setIconSize(*sz);
#ifdef DBG
    qDebug() << "mute=" << mute;
#endif
}
//--------------------------------------------------------------------------------
//**************************************************************************************
//                            Tray
//**************************************************************************************
void MWindow::showTrayIcon()
{
    trayIcon = new QSystemTrayIcon(this);
    QIcon trayImage(main_pic);
    trayIcon->setIcon(trayImage);
    trayIcon->setContextMenu(trayIconMenu);

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

    trayIcon->show();
}
//-------------------------------------------------------------------------------------
void MWindow::trayActionExecute()
{
    //QMessageBox::information(this, "TrayIcon", "Тестовое сообщение. Замените вызов этого сообщения своим кодом.");
}
//-------------------------------------------------------------------------------------
void MWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (static_cast<int>(reason)) {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            this->showNormal();
        break;
    }
}
//-------------------------------------------------------------------------------------
void MWindow::setTrayIconActions()
{
    minA  = new QAction(QIcon(hide_pic), "Hide", this);
    maxA  = new QAction(QIcon(show_pic), "Show", this);
    quitA = new QAction(QIcon(close_pic),"Quit", this);


    connect(minA, SIGNAL(triggered()),  this, SLOT(hide()));
    connect(maxA, SIGNAL(triggered()),  this, SLOT(showNormal()));
    connect(quitA, SIGNAL(triggered()), qApp, SLOT(quit()));

    trayIconMenu = new QMenu(this);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(minA);
    trayIconMenu->addAction(maxA);
    trayIconMenu->addAction(quitA);
    //trayIconMenu->setStyleSheet(QString::fromUtf8("background-color: rgb(100, 100, 100);"));
}
//-------------------------------------------------------------------------------------
void MWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) this->hide();
    }
}
//-------------------------------------------------------------------------------------
void MWindow::closeEvent(QCloseEvent *evt)
{
    if (trayIcon->isVisible()) {
        this->hide();
        evt->ignore();
    }
}
//**************************************************************************************
