//--
// This file is part of Sonic Pi: http://sonic-pi.net
// Full project source: https://github.com/samaaron/sonic-pi
// License: https://github.com/samaaron/sonic-pi/blob/master/LICENSE.md
//
// Copyright 2013, 2014, 2015 by Sam Aaron (http://sam.aaron.name).
// All rights reserved.
//
// Permission is granted for use, copying, modification, and
// distribution of modified versions of this work as long as this
// notice is included.
//++


// Standard stuff
#include <iostream>
#include <math.h>
#include <sstream>
#include <fstream>

// Qt stuff
#include <QDir>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QIcon>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QDockWidget>
#include <QPoint>
#include <QSettings>
#include <QSize>
#include <QStatusBar>
#include <QTextEdit>
#include <QTextBrowser>
#include <QToolBar>
#include <QProcess>
#include <QFont>
#include <QTabWidget>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QSplashScreen>
#include <QPixmap>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QGridLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QScrollArea>
#include <QShortcut>
#include <QToolButton>
#include <QSettings>
#include <QScrollBar>
#include <QSignalMapper>

// QScintilla stuff
#include <Qsci/qsciapis.h>
#include <Qsci/qsciscintilla.h>

#include "sonicpilexer.h"
#include "sonicpiapis.h"
#include "sonicpiscintilla.h"

#include "oschandler.h"
#include "sonicpiudpserver.h"
#include "sonicpitcpserver.h"

// OSC stuff
#include "oscpkt.hh"
#include "udp.hh"
using namespace oscpkt;

// OS specific stuff
#if defined(Q_OS_WIN)
  #include <QtConcurrent/QtConcurrentRun>
  void sleep(int x) { Sleep((x)*1000); }
#elif defined(Q_OS_MAC)
  #include <QtConcurrent/QtConcurrentRun>
#else
//assuming Raspberry Pi
  #include <cmath>
  #include <QtConcurrentRun>
#endif

#include "mainwindow.h"

#ifdef Q_OS_MAC
MainWindow::MainWindow(QApplication &app, QMainWindow* splash)
#else
MainWindow::MainWindow(QApplication &app, QSplashScreen* splash)
#endif
{
  this->protocol = UDP;
  this->splash = splash;

  if(protocol == TCP){
    clientSock = new QTcpSocket(this);
  }


  printAsciiArtLogo();
  // kill any zombie processes that may exist
  // better: test to see if UDP ports are in use, only kill/sleep if so
  // best: kill SCSynth directly if needed
  qDebug() << "[GUI] - shutting down any pre-existing audio servers...";
  Message msg("/exit");
  sendOSC(msg);
  sleep(2);

  this->setUnifiedTitleAndToolBarOnMac(true);
  this->setWindowIcon(QIcon(":images/icon-smaller.png"));

  currentLine = 0;
  currentIndex = 0;
  is_recording = false;
  show_rec_icon_a = false;

  rec_flash_timer = new QTimer(this);
  connect(rec_flash_timer, SIGNAL(timeout()), this, SLOT(toggleRecordingOnIcon()));

  // Setup output and error panes
  outputPane = new QTextEdit;
  errorPane = new QTextEdit;

  QThreadPool::globalInstance()->setMaxThreadCount(3);

  server_thread = QtConcurrent::run(this, &MainWindow::startServer);

  OscHandler* handler = new OscHandler(this, this->outputPane, this->errorPane);

  if(protocol == UDP){
    sonicPiServer = new SonicPiUDPServer(this, handler);
    osc_thread = QtConcurrent::run(sonicPiServer, &SonicPiServer::startServer);
  }
  else{
    sonicPiServer = new SonicPiTCPServer(this, handler);
    sonicPiServer->startServer();
  }

  // Window layout
  tabs = new QTabWidget();
  tabs->setTabsClosable(false);
  tabs->setMovable(false);
  tabs->setTabPosition(QTabWidget::South);

  // Syntax highlighting
  lexer = new SonicPiLexer;
  lexer->setAutoIndentStyle(SonicPiScintilla::AiMaintain);

  // create workspaces and add them to the tabs
  // workspace shortcuts
  signalMapper = new QSignalMapper (this) ;
  for(int ws = 0; ws < workspace_max; ws++) {
    std::string s;


    SonicPiScintilla *workspace = new SonicPiScintilla(lexer);

    //tab completion when in list
    QShortcut *indentLine = new QShortcut(QKeySequence("Tab"), workspace);
    connect (indentLine, SIGNAL(activated()), signalMapper, SLOT(map())) ;
    signalMapper -> setMapping (indentLine, (QObject*)workspace);

    //transpose chars
    QShortcut *transposeChars = new QShortcut(ctrlKey('t'), workspace);
    connect (transposeChars, SIGNAL(activated()), workspace, SLOT(transposeChars())) ;

    //move line or selection up and down
    QShortcut *moveLineUp = new QShortcut(ctrlMetaKey('p'), workspace);
    connect (moveLineUp, SIGNAL(activated()), workspace, SLOT(moveLineOrSelectionUp())) ;

    QShortcut *moveLineDown = new QShortcut(ctrlMetaKey('n'), workspace);
    connect (moveLineDown, SIGNAL(activated()), workspace, SLOT(moveLineOrSelectionDown())) ;

    // Windows-style shortcuts for copy and paste

    QShortcut *winCopy = new QShortcut(ctrlKey('c'), workspace);
    connect (winCopy, SIGNAL(activated()), workspace, SLOT(copy())) ;
    QShortcut *winPaste = new QShortcut(ctrlKey('v'), workspace);
    connect (winPaste, SIGNAL(activated()), workspace, SLOT(paste())) ;

    //set Mark
#ifdef Q_OS_MAC
    QShortcut *setMark = new QShortcut(QKeySequence("Meta+Space"), workspace);
#else
    QShortcut *setMark = new QShortcut(QKeySequence("Ctrl+Space"), workspace);
#endif
    connect (setMark, SIGNAL(activated()), workspace, SLOT(setMark())) ;

    //escape
    QShortcut *escape = new QShortcut(ctrlKey('g'), workspace);
    QShortcut *escape2 = new QShortcut(QKeySequence("Escape"), workspace);
    connect(escape, SIGNAL(activated()), workspace, SLOT(escapeAndCancelSelection()));
    connect(escape, SIGNAL(activated()), this, SLOT(resetErrorPane()));
    connect(escape2, SIGNAL(activated()), workspace, SLOT(escapeAndCancelSelection()));
    connect(escape2, SIGNAL(activated()), this, SLOT(resetErrorPane()));

    //quick nav by jumping up and down 10 lines at a time
    QShortcut *forwardTenLines = new QShortcut(shiftMetaKey('u'), workspace);
    connect(forwardTenLines, SIGNAL(activated()), workspace, SLOT(forwardTenLines()));
    QShortcut *backTenLines = new QShortcut(shiftMetaKey('d'), workspace);
    connect(backTenLines, SIGNAL(activated()), workspace, SLOT(backTenLines()));

    //cut to end of line
    QShortcut *cutToEndOfLine = new QShortcut(ctrlKey('k'), workspace);
    connect(cutToEndOfLine, SIGNAL(activated()), workspace, SLOT(cutLineFromPoint()));

    //Emacs live copy and cut
    QShortcut *copyToBuffer = new QShortcut(metaKey(']'), workspace);
    connect(copyToBuffer, SIGNAL(activated()), workspace, SLOT(copyClear()));
    QShortcut *cutToBuffer = new QShortcut(ctrlKey(']'), workspace);
    connect(cutToBuffer, SIGNAL(activated()), workspace, SLOT(cut()));

    //upcase next word
    QShortcut *upcaseWord= new QShortcut(metaKey('u'), workspace);
    connect(upcaseWord, SIGNAL(activated()), workspace, SLOT(upcaseWordOrSelection()));

    //downcase next word
    QShortcut *downcaseWord= new QShortcut(metaKey('l'), workspace);
    connect(downcaseWord, SIGNAL(activated()), workspace, SLOT(downcaseWordOrSelection()));

    //Goto nth Tab
    QShortcut *changeTab = new QShortcut(metaKey(int2char(ws)), this);
    connect(changeTab, SIGNAL(activated()), signalMapper, SLOT(map()));
    signalMapper -> setMapping(changeTab, ws);

    QString w = QString(tr("Workspace %1")).arg(QString::number(ws));
    workspaces[ws] = workspace;
    tabs->addTab(workspace, w);
  }

  connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(changeTab(int)));
  connect(signalMapper, SIGNAL(mapped(QObject*)), this, SLOT(completeListOrIndentLine(QObject*)));

  QFont font("Monospace");
  font.setStyleHint(QFont::Monospace);
  lexer->setDefaultFont(font);

  autocomplete = new SonicPiAPIs(lexer);
  autocomplete->loadSamples(sample_path);

  // adding universal shortcuts to outputpane seems to
  // steal events from doc system!?
  // addUniversalCopyShortcuts(outputPane);

  addUniversalCopyShortcuts(errorPane);
  outputPane->setReadOnly(true);
  errorPane->setReadOnly(true);
  outputPane->setLineWrapMode(QTextEdit::NoWrap);
#if defined(Q_OS_WIN)
  outputPane->setFontFamily("Courier New");
#elif defined(Q_OS_MAC)
  outputPane->setFontFamily("Menlo");
#else
  outputPane->setFontFamily("Bitstream Vera Sans Mono");
#endif

  outputPane->document()->setMaximumBlockCount(1000);
  errorPane->document()->setMaximumBlockCount(1000);

  outputPane->zoomIn(1);
  errorPane->zoomIn(1);
  errorPane->setMaximumHeight(130);
  errorPane->setMinimumHeight(130);

  // hudPane = new QTextBrowser;
  // hudPane->setMinimumHeight(130);
  // hudPane->setHtml("<center><img src=\":/images/logo.png\" height=\"113\" width=\"138\"></center>");
  // hudWidget = new QDockWidget(this);
  // hudWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
  // hudWidget->setAllowedAreas(Qt::RightDockWidgetArea);
  // hudWidget->setTitleBarWidget(new QWidget());
  // addDockWidget(Qt::RightDockWidgetArea, hudWidget);
  // hudWidget->setWidget(hudPane);
  // hudWidget->setObjectName("hud");


  prefsWidget = new QDockWidget(tr("Preferences"), this);
  prefsWidget->setFocusPolicy(Qt::NoFocus);
  prefsWidget->setAllowedAreas(Qt::RightDockWidgetArea);
  prefsWidget->setFeatures(QDockWidget::DockWidgetClosable);
  prefsCentral = new QWidget;
  prefsWidget->setWidget(prefsCentral);
  addDockWidget(Qt::RightDockWidgetArea, prefsWidget);
  prefsWidget->hide();
  prefsWidget->setObjectName("prefs");

  outputWidget = new QDockWidget(tr("Log"), this);
  outputWidget->setFocusPolicy(Qt::NoFocus);
  outputWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
  outputWidget->setAllowedAreas(Qt::RightDockWidgetArea);
  outputWidget->setWidget(outputPane);
  addDockWidget(Qt::RightDockWidgetArea, outputWidget);
  outputWidget->setObjectName("output");

  docsCentral = new QTabWidget;
  docsCentral->setFocusPolicy(Qt::NoFocus);
  docsCentral->setTabsClosable(false);
  docsCentral->setMovable(false);
  docsCentral->setTabPosition(QTabWidget::South);

  docPane = new QTextBrowser;
  docPane->setFocusPolicy(Qt::NoFocus);
  docPane->setMinimumHeight(200);
  docPane->setOpenExternalLinks(true);
  QString style = "QTextBrowser { selection-color: white; selection-background-color: deeppink; padding-left:10; padding-top:10; padding-bottom:10; padding-right:10 ; background:white;}";
  docPane->setStyleSheet(style);

  QShortcut *up = new QShortcut(ctrlKey('p'), docPane);
  up->setContext(Qt::WidgetShortcut);
  connect(up, SIGNAL(activated()), this, SLOT(docScrollUp()));
  QShortcut *down = new QShortcut(ctrlKey('n'), docPane);
  down->setContext(Qt::WidgetShortcut);
  connect(down, SIGNAL(activated()), this, SLOT(docScrollDown()));


#if defined(Q_OS_WIN)
  docPane->setHtml("<center><img src=\":/images/logo.png\" height=\"298\" width=\"365\"></center>");
#elif defined(Q_OS_MAC)
  docPane->setHtml("<center><img src=\":/images/logo.png\" height=\"298\" width=\"365\"></center>");
#else
  //assuming Raspberry Pi
  //use smaller logo
  docPane->setHtml("<center><img src=\":/images/logo-smaller.png\" height=\"161\" width=\"197\"></center>");
#endif


  addUniversalCopyShortcuts(docPane);

  QHBoxLayout *docLayout = new QHBoxLayout;
  docLayout->addWidget(docsCentral);
  docLayout->addWidget(docPane, 1);
  QWidget *docW = new QWidget();
  docW->setLayout(docLayout);

  docWidget = new QDockWidget(tr("Help"), this);
  docWidget->setFocusPolicy(Qt::NoFocus);
  docWidget->setAllowedAreas(Qt::BottomDockWidgetArea);
  docWidget->setWidget(docW);
  docWidget->setObjectName("help");

  addDockWidget(Qt::BottomDockWidgetArea, docWidget);
  docWidget->hide();

  // Currently causes a segfault when dragging doc pane out of main
  // window:
  // connect(docWidget, SIGNAL(visibilityChanged(bool)), this,
  // SLOT(helpClosed(bool)));

  QVBoxLayout *mainWidgetLayout = new QVBoxLayout;
  mainWidgetLayout->addWidget(tabs);
  mainWidgetLayout->addWidget(errorPane);
  QWidget *mainWidget = new QWidget;
  mainWidget->setFocusPolicy(Qt::NoFocus);
  errorPane->hide();
  mainWidget->setLayout(mainWidgetLayout);
  setCentralWidget(mainWidget);

  createShortcuts();
  createToolBar();
  createStatusBar();
  createInfoPane();

  readSettings();

  #ifndef Q_OS_MAC
  setWindowTitle(tr("Sonic Pi"));
  #endif

  connect(&app, SIGNAL( aboutToQuit() ), this, SLOT( onExitCleanup() ) );

  waitForServiceSync();

  initPrefsWindow();
  initDocsWindow();

  QSettings settings("uk.ac.cam.cl", "Sonic Pi");

  if(settings.value("first_time", 1).toInt() == 1) {
    QTextEdit* startupPane = new QTextEdit;
    startupPane->setReadOnly(true);
    startupPane->setFixedSize(600, 615);
    startupPane->setWindowIcon(QIcon(":images/icon-smaller.png"));
    startupPane->setWindowTitle(tr("Welcome to Sonic Pi"));
    addUniversalCopyShortcuts(startupPane);
    QString html;

    startupPane->setHtml(readFile(":/html/startup.html"));
    docWidget->show();
    startupPane->show();
  }
}

void MainWindow::changeTab(int id){
  tabs->setCurrentIndex(id);
}

void MainWindow::completeListOrIndentLine(QObject* ws){
  SonicPiScintilla *spws = ((SonicPiScintilla*)ws);
  if(spws->isListActive()) {
    spws->tabCompleteifList();
  }
  else {
    indentCurrentLineOrSelection(spws);
  }
}

void MainWindow::indentCurrentLineOrSelection(SonicPiScintilla* ws) {
  int start_line, finish_line, point_line, point_index;
  ws->getCursorPosition(&point_line, &point_index);
  if(ws->hasSelectedText()) {
    statusBar()->showMessage(tr("Indenting selection..."), 2000);
    int unused_a, unused_b;
    ws->getSelection(&start_line, &unused_a, &finish_line, &unused_b);
  } else {
    statusBar()->showMessage(tr("Indenting line..."), 2000);
    start_line = point_line;
    finish_line = point_line;
  }


  std::string code = ws->text().toStdString();

  Message msg("/indent-selection");
  std::string filename = workspaceFilename(ws);
  msg.pushStr(filename);
  msg.pushStr(code);
  msg.pushInt32(start_line);
  msg.pushInt32(finish_line);
  msg.pushInt32(point_line);
  msg.pushInt32(point_index);
  sendOSC(msg);
}

QString MainWindow::rootPath() {
  // diversity is the spice of life
#if defined(Q_OS_MAC)
  return QCoreApplication::applicationDirPath() + "/../..";
#elif defined(Q_OS_WIN)
  return QCoreApplication::applicationDirPath() + "/../../../..";
#else
  return QCoreApplication::applicationDirPath() + "/../../..";
#endif
}

void MainWindow::startServer(){
    serverProcess = new QProcess();

    QString root = rootPath();

  #if defined(Q_OS_WIN)
    QString prg_path = root + "/app/server/native/windows/bin/ruby.exe";
    QString prg_arg = root + "/app/server/bin/sonic-pi-server.rb";
    sample_path = root + "/etc/samples";
  #elif defined(Q_OS_MAC)
    QString prg_path = root + "/server/native/osx/ruby/bin/ruby";
    QString prg_arg = root + "/server/bin/sonic-pi-server.rb";
    sample_path = root + "/etc/samples";
  #else
    //assuming Raspberry Pi
    QString prg_path = root + "/app/server/native/raspberry/ruby/bin/ruby";
    QFile file(prg_path);
    if(!file.exists()) {
      // use system ruby if bundled ruby doesn't exist
      prg_path = "ruby";
    }

    QString prg_arg = root + "/app/server/bin/sonic-pi-server.rb";
    sample_path = root + "/etc/samples";
  #endif

    prg_path = QDir::toNativeSeparators(prg_path);
    prg_arg = QDir::toNativeSeparators(prg_arg);


    QStringList args;
    args << prg_arg;

    if(protocol == TCP){
        args << "-t";
    }

    QString sp_user_path = QDir::homePath() + QDir::separator() + ".sonic-pi";
    log_path =  sp_user_path + QDir::separator() + "log";
    QDir().mkdir(sp_user_path);
    QDir().mkdir(log_path);

  #if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    coutbuf = std::cout.rdbuf();
    stdlog.open(QString(log_path + "/stdout.log").toStdString().c_str());
    std::cout.rdbuf(stdlog.rdbuf());
  #endif

    //    std::cout << "[GUI] - exec "<< prg_path.toStdString() << " " << prg_arg.toStdString() << std::endl;
    std::cout << "[GUI] - booting live coding server" << std::endl;

    QString sp_error_log_path = log_path + QDir::separator() + "errors.log";
    QString sp_output_log_path = log_path + QDir::separator() + "output.log";
    serverProcess->setStandardErrorFile(sp_error_log_path);
    serverProcess->setStandardOutputFile(sp_output_log_path);
    serverProcess->start(prg_path, args);
    if (!serverProcess->waitForStarted()) {
      invokeStartupError(tr("ruby could not be started, is it installed and in your PATH?"));
      return;
    }
}

void MainWindow::waitForServiceSync() {
  int timeout = 30;
  qDebug() << "[GUI] - waiting for server to connect...";
  while (sonicPiServer->waitForServer() && timeout-- > 0) {
    sleep(1);
    if(sonicPiServer->isIncomingPortOpen()) {
      Message msg("/ping");
      msg.pushStr("QtClient/1/hello");
      sendOSC(msg);
    }
  }

  if (!sonicPiServer->isServerStarted()) {
    if (!startup_error_reported) {
      invokeStartupError(QString(tr("Failed to start server, please check %1.").arg(log_path)));
    }
    return;
  }

    qDebug() << "[GUI] - server connection established";

}

void MainWindow::splashClose() {
#if defined(Q_OS_MAC)
  splash->close();
#else
  splash->finish(this);
#endif
}

void MainWindow::serverStarted() {
  splashClose();
  loadWorkspaces();

  QSettings settings("uk.ac.cam.cl", "Sonic Pi");

  if(settings.value("first_time", 1).toInt() == 1) {
    this->showMaximized();
  } else {
    this->showNormal();

  }
  this->changeShowLineNumbers();
}


void MainWindow::serverError(QProcess::ProcessError error) {
  sonicPiServer->stopServer();
  std::cout << "[GUI] - Server Error: " << error <<std::endl;
  std::cout << serverProcess->readAllStandardError().data() << std::endl;
  std::cout << serverProcess->readAllStandardOutput().data() << std::endl;
}

void MainWindow::serverFinished(int exitCode, QProcess::ExitStatus exitStatus) {
  std::cout << "[GUI] - Server Finished: " << exitCode << ", " << exitStatus << std::endl;
  std::cout << serverProcess->readAllStandardError().data() << std::endl;
  std::cout << serverProcess->readAllStandardOutput().data() << std::endl;
}

void MainWindow::update_mixer_invert_stereo() {
  if (mixer_invert_stereo->isChecked()) {
    mixerInvertStereo();
  } else {
    mixerStandardStereo();
  }
}

void MainWindow::update_mixer_force_mono() {
  if (mixer_force_mono->isChecked()) {
    mixerMonoMode();
  } else {
    mixerStereoMode();
  }
}

void MainWindow::update_check_updates() {
  if (check_updates->isChecked()) {
    enableCheckUpdates();
  } else {
    disableCheckUpdates();
  }
}

void MainWindow::initPrefsWindow() {

  QGridLayout *grid = new QGridLayout;

  QGroupBox *volBox = new QGroupBox(tr("Raspberry Pi System Volume"));
  volBox->setToolTip(tr("Use this slider to change the system volume of your Raspberry Pi."));

  QGroupBox *advancedAudioBox = new QGroupBox(tr("Studio Settings"));
  advancedAudioBox->setToolTip(tr("Advanced audio settings for working with\nexternal PA systems when performing with Sonic Pi."));
  mixer_invert_stereo = new QCheckBox(tr("Invert Stereo"));
  connect(mixer_invert_stereo, SIGNAL(clicked()), this, SLOT(update_mixer_invert_stereo()));
  mixer_force_mono = new QCheckBox(tr("Force Mono"));
  connect(mixer_force_mono, SIGNAL(clicked()), this, SLOT(update_mixer_force_mono()));


  QVBoxLayout *advanced_audio_box_layout = new QVBoxLayout;
  advanced_audio_box_layout->addWidget(mixer_invert_stereo);
  advanced_audio_box_layout->addWidget(mixer_force_mono);
  // audio_box->addWidget(radio2);
  // audio_box->addWidget(radio3);
  // audio_box->addStretch(1);
  advancedAudioBox->setLayout(advanced_audio_box_layout);


  QGroupBox *audioOutputBox = new QGroupBox(tr("Raspberry Pi Audio Output"));
  audioOutputBox->setToolTip(tr("Your Raspberry Pi has two forms of audio output.\nFirstly, there is the headphone jack of the Raspberry Pi itself.\nSecondly, some HDMI monitors/TVs support audio through the HDMI port.\nUse these buttons to force the output to the one you want."));
  rp_force_audio_default = new QRadioButton(tr("&Default"));
  rp_force_audio_headphones = new QRadioButton(tr("&Headphones"));
  rp_force_audio_hdmi = new QRadioButton(tr("&HDMI"));


  connect(rp_force_audio_default, SIGNAL(clicked()), this, SLOT(setRPSystemAudioAuto()));
  connect(rp_force_audio_headphones, SIGNAL(clicked()), this, SLOT(setRPSystemAudioHeadphones()));
  connect(rp_force_audio_hdmi, SIGNAL(clicked()), this, SLOT(setRPSystemAudioHDMI()));

  QVBoxLayout *audio_box = new QVBoxLayout;
  audio_box->addWidget(rp_force_audio_default);
  audio_box->addWidget(rp_force_audio_headphones);
  audio_box->addWidget(rp_force_audio_hdmi);
  audio_box->addStretch(1);
  audioOutputBox->setLayout(audio_box);

  QHBoxLayout *vol_box = new QHBoxLayout;
  rp_system_vol = new QSlider(this);
  connect(rp_system_vol, SIGNAL(valueChanged(int)), this, SLOT(changeRPSystemVol(int)));
  vol_box->addWidget(rp_system_vol);
  volBox->setLayout(vol_box);

  QGroupBox *debug_box = new QGroupBox(tr("Debug Options"));
  print_output = new QCheckBox(tr("Print output"));
  check_args = new QCheckBox(tr("Check synth args"));
  clear_output_on_run = new QCheckBox(tr("Clear output on run"));


  QVBoxLayout *debug_box_layout = new QVBoxLayout;
  debug_box_layout->addWidget(print_output);
  debug_box_layout->addWidget(check_args);
  debug_box_layout->addWidget(clear_output_on_run);
  debug_box->setLayout(debug_box_layout);


  QGroupBox *update_box = new QGroupBox(tr("Updates"));
  check_updates = new QCheckBox(tr("Check for updates"));
  connect(check_updates, SIGNAL(clicked()), this, SLOT(update_check_updates()));

  update_box->setToolTip(tr("Configure whether Sonic Pi may check for new updates on launch.\nPlease note, the checking process includes sending\nanonymous information to the Sonic Pi server."));

  QVBoxLayout *update_box_layout = new QVBoxLayout;
  update_box_layout->addWidget(check_updates);
  update_box->setLayout(update_box_layout);


  QGroupBox *editor_box = new QGroupBox(tr("Editor"));
  show_line_numbers = new QCheckBox(tr("Show line numbers"));

  connect(show_line_numbers, SIGNAL(clicked()), this, SLOT(changeShowLineNumbers()));
  editor_box->setToolTip(tr("Editor Preferences"));

  QVBoxLayout *editor_box_layout = new QVBoxLayout;
  editor_box_layout->addWidget(show_line_numbers);
  editor_box->setLayout(editor_box_layout);

#if defined(Q_OS_LINUX)
   grid->addWidget(audioOutputBox, 0, 0);
   grid->addWidget(volBox, 0, 1);
#endif
  grid->addWidget(debug_box, 1, 1);
  grid->addWidget(advancedAudioBox, 1, 0);
  grid->addWidget(update_box, 2, 0);
  grid->addWidget(editor_box, 2, 1);
  prefsCentral->setLayout(grid);



  // Read in preferences from previous session
  QSettings settings("uk.ac.cam.cl", "Sonic Pi");
  check_args->setChecked(settings.value("prefs/check-args", true).toBool());
  print_output->setChecked(settings.value("prefs/print-output", true).toBool());
  clear_output_on_run->setChecked(settings.value("prefs/clear-output-on-run", true).toBool());
  show_line_numbers->setChecked(settings.value("prefs/show-line-numbers", true).toBool());
  mixer_force_mono->setChecked(settings.value("prefs/mixer-force-mono", false).toBool());
  mixer_invert_stereo->setChecked(settings.value("prefs/mixer-invert-stereo", false).toBool());

  rp_force_audio_default->setChecked(settings.value("prefs/rp/force-audio-default", true).toBool());
  rp_force_audio_headphones->setChecked(settings.value("prefs/rp/force-audio-headphones", false).toBool());
  rp_force_audio_hdmi->setChecked(settings.value("prefs/rp/force-audio-hdmi", false).toBool());

  check_updates->setChecked(settings.value("prefs/rp/check-updates", true).toBool());

  int stored_vol = settings.value("prefs/rp/system-vol", 50).toInt();
  rp_system_vol->setValue(stored_vol);

  // Ensure prefs are honoured on boot
  update_mixer_invert_stereo();
  update_mixer_force_mono();
  changeRPSystemVol(stored_vol);
  update_check_updates();

  if(settings.value("prefs/rp/force-audio-default", true).toBool()) {
    setRPSystemAudioAuto();
  }
  if(settings.value("prefs/rp/force-audio-headphones", false).toBool()) {
    setRPSystemAudioHeadphones();
  }
  if(settings.value("prefs/rp/force-audio-hdmi", false).toBool()) {
    setRPSystemAudioHDMI();
  }


}

void MainWindow::invokeStartupError(QString msg) {
  startup_error_reported = true;
  sonicPiServer->stopServer();
  QMetaObject::invokeMethod(this, "startupError",
			    Qt::QueuedConnection,
			    Q_ARG(QString, msg));
}

void MainWindow::startupError(QString msg) {
  splashClose();
  startup_error_reported = true;

  QString logtext = readFile(log_path + QDir::separator() + "output.log");
  QMessageBox *box = new QMessageBox(QMessageBox::Warning,
				     tr("We're sorry, but Sonic Pi was unable to start..."), msg);
  box->setDetailedText(logtext);

  QGridLayout* layout = (QGridLayout*)box->layout();
  QSpacerItem* hSpacer = new QSpacerItem(500, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
  QSpacerItem* vSpacer = new QSpacerItem(0, 400, QSizePolicy::Minimum, QSizePolicy::Expanding);
  layout->addItem(hSpacer, layout->rowCount(), 0, 1, layout->columnCount());
  layout->addItem(vSpacer, layout->rowCount(), 0, 1, layout->columnCount());
  box->exec();

  QTimer::singleShot(0, this, SLOT(close()));
}

void MainWindow::replaceBuffer(QString id, QString content, int line, int index, int first_line) {
  SonicPiScintilla* ws = filenameToWorkspace(id.toStdString());
  ws->selectAll();
  ws->replaceSelectedText(content);
  ws->setCursorPosition(line, index);
  ws->setFirstVisibleLine(first_line);
}

void MainWindow::replaceLines(QString id, QString content, int start_line, int finish_line, int point_line, int point_index) {
  SonicPiScintilla* ws = filenameToWorkspace(id.toStdString());
  ws->replaceLines(start_line, finish_line, content);
  ws->setCursorPosition(point_line, point_index);
}

std::string MainWindow::number_name(int i) {
  switch(i) {
  case 0: return "zero";
  case 1: return "one";
  case 2: return "two";
  case 3: return "three";
  case 4: return "four";
  case 5: return "five";
  case 6: return "six";
  case 7: return "seven";
  case 8: return "eight";
  case 9: return "nine";
  default: assert(false); return "";
  }
}

std::string MainWindow::workspaceFilename(SonicPiScintilla* text)
{
  for(int i = 0; i < workspace_max; i++) {
    if(text == workspaces[i]) {
      return "workspace_" + number_name(i);
    }
  }
  return "default";
}

void MainWindow::loadWorkspaces()
{
  std::cout << "[GUI] - loading workspaces" << std::endl;

  for(int i = 0; i < workspace_max; i++) {
    Message msg("/load-buffer");
    std::string s = "workspace_" + number_name(i);
    msg.pushStr(s);
    sendOSC(msg);
  }
}

void MainWindow::saveWorkspaces()
{
  std::cout << "[GUI] - saving workspaces" << std::endl;

  for(int i = 0; i < workspace_max; i++) {
    std::string code = workspaces[i]->text().toStdString();
    Message msg("/save-buffer");
    std::string s = "workspace_" + number_name(i);
    msg.pushStr(s);
    msg.pushStr(code);
    sendOSC(msg);
  }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  writeSettings();
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
  std::cout.rdbuf(coutbuf); // reset to stdout before exiting
#endif

  event->accept();
}

QString MainWindow::currentTabLabel()
{
  return tabs->tabText(tabs->currentIndex());
}


bool MainWindow::saveAs()
{
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save Current Workspace"), QDir::homePath() + "/Desktop");
  if(!fileName.isEmpty()){
    return saveFile(fileName, (SonicPiScintilla*)tabs->currentWidget());
  } else {
    return false;
  }
}

void MainWindow::sendOSC(Message m)
{
  int TIMEOUT = 30000;
  int PORT_NUM = 4557;

  if(protocol == UDP){
    UdpSocket sock;
    sock.connectTo("localhost", PORT_NUM);
    if (!sock.isOk()) {
        std::cerr << "Error connection to port " << PORT_NUM << ": " << sock.errorMessage() << "\n";
    } else {
        PacketWriter pw;
        pw.addMessage(m);
        sock.sendPacket(pw.packetData(), pw.packetSize());
    }
  }
  else{
    if (clientSock->state() != QAbstractSocket::ConnectedState){
      clientSock->connectToHost("localhost", PORT_NUM,  QIODevice::ReadWrite);
    }

    if(!clientSock->waitForConnected(TIMEOUT)){
      std::cerr <<  "Timeout, could not connect" << "\n";
      clientSock->abort();
      return;
    }

    if(clientSock->state() == QAbstractSocket::ConnectedState){
      PacketWriter pw;
      pw.addMessage(m);

      int bytesWritten = clientSock->write(pw.packetDataForStream(), pw.packetSize()+sizeof(uint32_t));
      clientSock->waitForBytesWritten();

      if (bytesWritten < 0){
        std::cerr <<  "Failed to send bytes" << "\n";
      }

    } else {
      std::cerr << "Client gone away: " << "\n";
    }
  }
}

void MainWindow::resetErrorPane() {
  errorPane->clear();
  errorPane->hide();
}


void MainWindow::runCode()
{
  SonicPiScintilla *ws = ((SonicPiScintilla*)tabs->currentWidget());
  if (currentLine == 0 && currentIndex == 0) {
    // only update saved position if we're not already highlighting code
    ws->getCursorPosition(&currentLine, &currentIndex);
  }
  ws->setReadOnly(true);
  ws->selectAll();
  resetErrorPane();
  statusBar()->showMessage(tr("Running Code..."), 1000);
  std::string code = ((SonicPiScintilla*)tabs->currentWidget())->text().toStdString();
  Message msg("/save-and-run-buffer");
  std::string filename = workspaceFilename( (SonicPiScintilla*)tabs->currentWidget());
  msg.pushStr(filename);
  if(!print_output->isChecked()) {
    code = "use_debug false #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }
  else{
    code = "use_debug true #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }
  if(!check_args->isChecked()) {
    code = "use_arg_checks false #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }
  else {
    code = "use_arg_checks true #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }
  if(clear_output_on_run->isChecked()){
    outputPane->clear();
  }

  msg.pushStr(code);
  msg.pushStr(QString(tr("Workspace %1")).arg(tabs->currentIndex()).toStdString());
  sendOSC(msg);

  QTimer::singleShot(500, this, SLOT(unhighlightCode()));


}

void MainWindow::unhighlightCode()
{
  SonicPiScintilla *ws = (SonicPiScintilla *)tabs->currentWidget();
  ws->selectAll(false);
  if (currentLine != 0 || currentIndex != 0) {
    ws->setCursorPosition(currentLine, currentIndex);
    currentLine = 0; currentIndex = 0;
  }
  ws->setReadOnly(false);
}

void MainWindow::beautifyCode()
{
  statusBar()->showMessage(tr("Beautifying..."), 2000);
  SonicPiScintilla* ws = ((SonicPiScintilla*)tabs->currentWidget());
  std::string code = ws->text().toStdString();
  int line = 0;
  int index = 0;
  ws->getCursorPosition(&line, &index);
  int first_line = ws->firstVisibleLine();
  Message msg("/beautify-buffer");
  std::string filename = workspaceFilename( (SonicPiScintilla*)tabs->currentWidget());
  msg.pushStr(filename);
  msg.pushStr(code);
  msg.pushInt32(line);
  msg.pushInt32(index);
  msg.pushInt32(first_line);
  sendOSC(msg);
}

void MainWindow::reloadServerCode()
{
  statusBar()->showMessage(tr("Reloading..."), 2000);
  Message msg("/reload");
  sendOSC(msg);
}

void MainWindow::enableCheckUpdates()
{
  statusBar()->showMessage(tr("Enabling update checking..."), 2000);
  Message msg("/enable-update-checking");
  sendOSC(msg);
}

void MainWindow::disableCheckUpdates()
{
  statusBar()->showMessage(tr("Disabling update checking..."), 2000);
  Message msg("/disable-update-checking");
  sendOSC(msg);
}

void MainWindow::mixerHpfEnable(float freq)
{
  statusBar()->showMessage(tr("Enabling Mixer HPF..."), 2000);
  Message msg("/mixer-hpf-enable");
  msg.pushFloat(freq);
  sendOSC(msg);
}

void MainWindow::mixerHpfDisable()
{
  statusBar()->showMessage(tr("Disabling Mixer HPF..."), 2000);
  Message msg("/mixer-hpf-disable");
  sendOSC(msg);
}

void MainWindow::mixerLpfEnable(float freq)
{
  statusBar()->showMessage(tr("Enabling Mixer LPF..."), 2000);
  Message msg("/mixer-lpf-enable");
  msg.pushFloat(freq);
  sendOSC(msg);
}

void MainWindow::mixerLpfDisable()
{
  statusBar()->showMessage(tr("Disabling Mixer LPF..."), 2000);
  Message msg("/mixer-lpf-disable");
  sendOSC(msg);
}

void MainWindow::mixerInvertStereo()
{
  statusBar()->showMessage(tr("Enabling Inverted Stereo..."), 2000);
  Message msg("/mixer-invert-stereo");
  sendOSC(msg);
}

void MainWindow::mixerStandardStereo()
{
  statusBar()->showMessage(tr("Enabling Standard Stereo..."), 2000);
  Message msg("/mixer-standard-stereo");
  sendOSC(msg);
}

void MainWindow::mixerMonoMode()
{
  statusBar()->showMessage(tr("Mono Mode..."), 2000);
  Message msg("/mixer-mono-mode");
  sendOSC(msg);
}

void MainWindow::mixerStereoMode()
{
  statusBar()->showMessage(tr("Stereo Mode..."), 2000);
  Message msg("/mixer-stereo-mode");
  sendOSC(msg);
}

void MainWindow::stopCode()
{
  stopRunningSynths();
  statusBar()->showMessage(tr("Stopping..."), 2000);
}

void MainWindow::about()
{
  // todo: this is returning true even after the window disappears
  // Qt::Tool windows get closed automatically when app loses focus
  if(infoWidg->isVisible()) {
    infoWidg->hide();
  } else {
    infoWidg->raise();
    infoWidg->show();
  }
}


void MainWindow::help()
{
  if(docWidget->isVisible()) {
    hidingDocPane = true;
    docWidget->hide();
  } else {
    docWidget->show();
  }
}

void MainWindow::helpContext()
{
  if (!docWidget->isVisible())
    docWidget->show();
  SonicPiScintilla *ws = ((SonicPiScintilla*)tabs->currentWidget());
  QString selection = ws->selectedText();
  if (selection == "") { // get current word instead
    int line, pos;
    ws->getCursorPosition(&line, &pos);
    QString text = ws->text(line);
    int start, end;
    for (start = pos; start > 0; start--) {
      if (!text[start-1].isLetter() && text[start-1] != '_') break;
    }
    for (end = pos; end < text.length(); end++) {
      if (!text[end].isLetter() && text[end] != '_') break;
    }
    selection = text.mid(start, end-start);
  }
  selection = selection.toLower();
  if (selection[0] == ':')
    selection = selection.mid(1);

  if (helpKeywords.contains(selection)) {
    struct help_entry entry = helpKeywords[selection];
    QMetaObject::invokeMethod(docsCentral, "setCurrentIndex",
			      Q_ARG(int, entry.pageIndex));
    QListWidget *list = helpLists[entry.pageIndex];
    list->setCurrentRow(entry.entryIndex);
  }
}

void MainWindow::changeRPSystemVol(int val)
{
#if defined(Q_OS_WIN)
  //do nothing
  val = val;
#elif defined(Q_OS_MAC)
  statusBar()->showMessage(tr("Updating System Volume..."), 2000);
  //do nothing, just print out what it would do on RPi
  float v = (float) val;
  float vol_float = pow(v/100.0, (float)1./3.) * 100.0;
  std::ostringstream ss;
  ss << vol_float;
  QString prog = "amixer cset numid=1 " + QString::fromStdString(ss.str()) + '%';
  std::cout << "[GUI] - " << prog.toStdString() << std::endl;
#else
  //assuming Raspberry Pi
  QProcess *p = new QProcess();
  float v = (float) val;
  // handle the fact that the amixer percentage range isn't linear
  float vol_float = std::pow(v/100.0, (float)1./3.) * 100.0;
  std::ostringstream ss;
  ss << vol_float;
  statusBar()->showMessage(tr("Updating System Volume..."), 2000);
  QString prog = "amixer cset numid=1 " + QString::fromStdString(ss.str()) + '%';
  p->start(prog);
#endif

}

void MainWindow::changeShowLineNumbers(){
  for(int i=0; i < tabs->count(); i++){
    SonicPiScintilla *ws = (SonicPiScintilla *)tabs->widget(i);
    if (show_line_numbers->isChecked()){
      ws->showLineNumbers();
    } else {
      ws->hideLineNumbers();
    }
  }
}

void MainWindow::setRPSystemAudioHeadphones()
{
#if defined(Q_OS_WIN)
  //do nothing
#elif defined(Q_OS_MAC)
  statusBar()->showMessage(tr("Switching To Headphone Audio Output..."), 2000);
  //do nothing, just print out what it would do on RPi
  QString prog = "amixer cset numid=3 1";
  std::cout << "[GUI] - " << prog.toStdString() << std::endl;
#else
  //assuming Raspberry Pi
  statusBar()->showMessage(tr("Switching To Headphone Audio Output..."), 2000);
  QProcess *p = new QProcess();
  QString prog = "amixer cset numid=3 1";
  p->start(prog);
#endif
}

void MainWindow::setRPSystemAudioHDMI()
{

#if defined(Q_OS_WIN)
  //do nothing
#elif defined(Q_OS_MAC)
  statusBar()->showMessage(tr("Switching To HDMI Audio Output..."), 2000);
  //do nothing, just print out what it would do on RPi
  QString prog = "amixer cset numid=3 2";
  std::cout << "[GUI] - " << prog.toStdString() << std::endl;
#else
  //assuming Raspberry Pi
  statusBar()->showMessage(tr("Switching To HDMI Audio Output..."), 2000);
  QProcess *p = new QProcess();
  QString prog = "amixer cset numid=3 2";
  p->start(prog);
#endif
}

void MainWindow::setRPSystemAudioAuto()
{
#if defined(Q_OS_WIN)
  //do nothing

#elif defined(Q_OS_MAC)
  statusBar()->showMessage(tr("Switching To Default Audio Output..."), 2000);
  //do nothing, just print out what it would do on RPi
  QString prog = "amixer cset numid=3 0";
  std::cout << "[GUI] - " << prog.toStdString() << std::endl;
#else
  //assuming Raspberry Pi
  statusBar()->showMessage(tr("Switching To Default Audio Output..."), 2000);
  QProcess *p = new QProcess();
  QString prog = "amixer cset numid=3 0";
  p->start(prog);
#endif
}

void MainWindow::showPrefsPane()
{
  if(prefsWidget->isVisible()) {
    prefsWidget->hide();
  } else {
    prefsWidget->show();
  }
}

void MainWindow::zoomFontIn()
{
  SonicPiScintilla* ws = ((SonicPiScintilla*)tabs->currentWidget());
  int zoom = ws->property("zoom").toInt();
  zoom++;
  if (zoom > 20) zoom = 20;
  ws->setProperty("zoom", QVariant(zoom));
  ws->zoomTo(zoom);
  if (show_line_numbers->isChecked()){
    ws->showLineNumbers();
  } else {
    ws->hideLineNumbers();
  }
}

void MainWindow::zoomFontOut()
{
  SonicPiScintilla* ws = ((SonicPiScintilla*)tabs->currentWidget());
  int zoom = ws->property("zoom").toInt();
  zoom--;
  if (zoom < -10) zoom = -10;
  ws->setProperty("zoom", QVariant(zoom));
  ws->zoomTo(zoom);
  if (show_line_numbers->isChecked()){
    ws->showLineNumbers();
  } else {
    ws->hideLineNumbers();
  }
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
#if defined(Q_OS_WIN)
  if (event->modifiers() & Qt::ControlModifier) {
    if (event->angleDelta().y() > 0)
      zoomFontIn();
    else
      zoomFontOut();
  }
#else
  (void)event;
#endif
}

void MainWindow::stopRunningSynths()
{
  Message msg("/stop-all-jobs");
  sendOSC(msg);
}

void MainWindow::clearOutputPanels()
{
  outputPane->clear();
  errorPane->clear();
}

QKeySequence MainWindow::ctrlKey(char key)
{
#ifdef Q_OS_MAC
  return QKeySequence(QString("Meta+%1").arg(key));
#else
  return QKeySequence(QString("Ctrl+%1").arg(key));
#endif
}

// Cmd on Mac, Alt everywhere else
QKeySequence MainWindow::metaKey(char key)
{
#ifdef Q_OS_MAC
  return QKeySequence(QString("Ctrl+%1").arg(key));
#else
  return QKeySequence(QString("alt+%1").arg(key));
#endif
}

QKeySequence MainWindow::shiftMetaKey(char key)
{
#ifdef Q_OS_MAC
  return QKeySequence(QString("Shift+Ctrl+%1").arg(key));
#else
  return QKeySequence(QString("Shift+alt+%1").arg(key));
#endif
}

QKeySequence MainWindow::ctrlMetaKey(char key)
{
#ifdef Q_OS_MAC
  return QKeySequence(QString("Ctrl+Meta+%1").arg(key));
#else
  return QKeySequence(QString("Ctrl+alt+%1").arg(key));
#endif
}

char MainWindow::int2char(int i){
  return '0' + i;
}

// set tooltips, connect event handlers, and add shortcut if applicable
void MainWindow::setupAction(QAction *action, char key, QString tooltip,
			     const char *slot)
{
  QString shortcut, tooltipKey;
  tooltipKey = tooltip;
  if (key != 0) {
#ifdef Q_OS_MAC
    tooltipKey = QString("%1 (⌘%2)").arg(tooltip).arg(key);
#else
    tooltipKey = QString("%1 (alt-%2)").arg(tooltip).arg(key);
#endif
  }

  action->setToolTip(tooltipKey);
  action->setStatusTip(tooltip);
  connect(action, SIGNAL(triggered()), this, slot);

  if (key != 0) {
    // create a QShortcut instead of setting the QAction's shortcut
    // so it will still be active with the toolbar hidden
    new QShortcut(metaKey(key), this, slot);
  }
}

void MainWindow::createShortcuts()
{
  new QShortcut(QKeySequence("F1"), this, SLOT(helpContext()));
  new QShortcut(ctrlKey('i'), this, SLOT(helpContext()));
  new QShortcut(metaKey('<'), this, SLOT(tabPrev()));
  new QShortcut(metaKey('>'), this, SLOT(tabNext()));
  //new QShortcut(metaKey('U'), this, SLOT(reloadServerCode()));
}

void MainWindow::createToolBar()
{
  // Run
  QAction *runAct = new QAction(QIcon(":/images/run.png"), tr("Run"), this);
  setupAction(runAct, 'R', tr("Run the code in the current workspace"),
	      SLOT(runCode()));

  // Stop
  QAction *stopAct = new QAction(QIcon(":/images/stop.png"), tr("Stop"), this);
  setupAction(stopAct, 'S', tr("Stop all running code"), SLOT(stopCode()));

  // Save
  QAction *saveAsAct = new QAction(QIcon(":/images/save.png"), tr("Save As..."), this);
  setupAction(saveAsAct, 0, tr("Save current workspace as an external file"), SLOT(saveAs()));

  // Info
  QAction *infoAct = new QAction(QIcon(":/images/info.png"), tr("Info"), this);
  setupAction(infoAct, 0, tr("See information about Sonic Pi"),
	      SLOT(about()));

  // Help
  QAction *helpAct = new QAction(QIcon(":/images/help.png"), tr("Help"), this);
  setupAction(helpAct, 'I', tr("Toggle help pane"), SLOT(help()));

  // Preferences
  QAction *prefsAct = new QAction(QIcon(":/images/prefs.png"), tr("Prefs"), this);
  setupAction(prefsAct, 'P', tr("Toggle preferences pane"),
	      SLOT(showPrefsPane()));

  // Record
  recAct = new QAction(QIcon(":/images/rec.png"), tr("Start Recording"), this);
  setupAction(recAct, 0, tr("Start Recording"), SLOT(toggleRecording()));

  // Align
  QAction *textAlignAct = new QAction(QIcon(":/images/align.png"),
			     tr("Auto-Align Text"), this);
  setupAction(textAlignAct, 'M', tr("Auto-align text"), SLOT(beautifyCode()));

  // Font Size Increase
  QAction *textIncAct = new QAction(QIcon(":/images/size_up.png"),
			    tr("Increase Text Size"), this);
  setupAction(textIncAct, '+', tr("Make text bigger"), SLOT(zoomFontIn()));
  new QShortcut(metaKey('='), this, SLOT(zoomFontIn()));

  // Font Size Decrease
  QAction *textDecAct = new QAction(QIcon(":/images/size_down.png"),
			    tr("Decrease Text Size"), this);
  setupAction(textDecAct, '-', tr("Make text smaller"), SLOT(zoomFontOut()));
  new QShortcut(metaKey('_'), this, SLOT(zoomFontOut()));

  QWidget *spacer = new QWidget();
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  toolBar = addToolBar(tr("Tools"));
  toolBar->setObjectName("toolbar");
  toolBar->setIconSize(QSize(270/3, 111/3));
  toolBar->addAction(runAct);
  toolBar->addAction(stopAct);

  toolBar->addAction(saveAsAct);
  toolBar->addAction(recAct);
  toolBar->addWidget(spacer);

  toolBar->addAction(textDecAct);
  toolBar->addAction(textIncAct);
  dynamic_cast<QToolButton*>(toolBar->widgetForAction(textDecAct))->setAutoRepeat(true);
  dynamic_cast<QToolButton*>(toolBar->widgetForAction(textIncAct))->setAutoRepeat(true);

  toolBar->addAction(textAlignAct);

  toolBar->addAction(infoAct);
  toolBar->addAction(helpAct);
  toolBar->addAction(prefsAct);
}

QString MainWindow::readFile(QString name)
{
  QFile file(name);
  if (!file.open(QFile::ReadOnly | QFile::Text))
    return "";

  QTextStream st(&file);
  st.setCodec("UTF-8");
  QString s;
  s.append(st.readAll());
  return s;
}

void MainWindow::createInfoPane() {
  QTabWidget *infoTabs = new QTabWidget(this);

  QStringList files, tabs;
  files << ":/html/info.html" << ":/info/CORETEAM.html" << ":/info/CONTRIBUTORS.html" <<
    ":/info/COMMUNITY.html" << ":/info/LICENSE.html" <<":/info/CHANGELOG.html";
  tabs << tr("About") << tr("Core Team") << tr("Contributors") <<
    tr("Community") << tr("License") << tr("History");

  for (int t=0; t < files.size(); t++) {
    QTextBrowser *pane = new QTextBrowser;
    addUniversalCopyShortcuts(pane);
    pane->setOpenExternalLinks(true);
    pane->setFixedSize(600, 615);
    pane->setHtml(readFile(files[t]));
    infoTabs->addTab(pane, tabs[t]);
  }

  infoTabs->setTabPosition(QTabWidget::South);

  QBoxLayout *infoLayout = new QBoxLayout(QBoxLayout::LeftToRight);
  infoLayout->addWidget(infoTabs);

  infoWidg = new QWidget;
  infoWidg->setWindowIcon(QIcon(":images/icon-smaller.png"));
  infoWidg->setLayout(infoLayout);
  infoWidg->setWindowFlags(Qt::Tool | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);
  infoWidg->setWindowTitle(tr("Sonic Pi - Info"));

  QAction *closeInfoAct = new QAction(this);
  closeInfoAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_W));
  connect(closeInfoAct, SIGNAL(triggered()), this, SLOT(about()));
  infoWidg->addAction(closeInfoAct);
}

void MainWindow::toggleRecordingOnIcon() {
  show_rec_icon_a = !show_rec_icon_a;
  if(show_rec_icon_a) {
    recAct->setIcon(QIcon(":/images/recording_a.png"));
  } else {
    recAct->setIcon(QIcon(":/images/recording_b.png"));
  }
}

void MainWindow::toggleRecording() {
  is_recording = !is_recording;
  if(is_recording) {
    recAct->setStatusTip(tr("Stop Recording"));
    recAct->setToolTip(tr("Stop Recording"));
    rec_flash_timer->start(500);
    Message msg("/start-recording");
    sendOSC(msg);
  } else {
    rec_flash_timer->stop();
    recAct->setStatusTip(tr("Start Recording"));
    recAct->setToolTip(tr("Start Recording"));
    recAct->setIcon(QIcon(":/images/rec.png"));
    Message msg("/stop-recording");
    sendOSC(msg);
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Recording"), QDir::homePath() + "/Desktop/my-recording.wav");
    if (!fileName.isEmpty()) {
      Message msg("/save-recording");
      msg.pushStr(fileName.toStdString());
      sendOSC(msg);
    } else {
      Message msg("/delete-recording");
      sendOSC(msg);
    }
  }
}


void MainWindow::createStatusBar()
{
  statusBar()->showMessage(tr("Ready..."));
}

void MainWindow::readSettings() {
  // Pref settings are read in MainWindow::initPrefsWindow()

  QSettings settings("uk.ac.cam.cl", "Sonic Pi");
  QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
  QSize size = settings.value("size", QSize(400, 400)).toSize();
  resize(size);
  move(pos);

  int index = settings.value("workspace", 0).toInt();
  if (index < tabs->count())
    tabs->setCurrentIndex(index);

  for (int w=0; w < workspace_max; w++) {
    // default zoom is 13
    int zoom = settings.value(QString("workspace%1zoom").arg(w), 13)
      .toInt();
    if (zoom < -5) zoom = -5;
    if (zoom > 20) zoom = 20;

    workspaces[w]->setProperty("zoom", QVariant(zoom));
    workspaces[w]->zoomTo(zoom);
  }


  restoreState(settings.value("windowState").toByteArray());

}

void MainWindow::writeSettings()
{
  QSettings settings("uk.ac.cam.cl", "Sonic Pi");
  settings.setValue("pos", pos());
  settings.setValue("size", size());
  settings.setValue("first_time", 0);


  settings.setValue("prefs/check-args", check_args->isChecked());
  settings.setValue("prefs/print-output", print_output->isChecked());
  settings.setValue("prefs/clear-output-on-run", clear_output_on_run->isChecked());
  settings.setValue("prefs/show-line-numbers", show_line_numbers->isChecked());
  settings.setValue("prefs/mixer-force-mono", mixer_force_mono->isChecked());
  settings.setValue("prefs/mixer-invert-stereo", mixer_invert_stereo->isChecked());

  settings.setValue("prefs/rp/force-audio-default", rp_force_audio_default->isChecked());
  settings.setValue("prefs/rp/force-audio-headphones", rp_force_audio_headphones->isChecked());
  settings.setValue("prefs/rp/force-audio-hdmi", rp_force_audio_hdmi->isChecked());
  settings.setValue("prefs/rp/system-vol", rp_system_vol->value());

  settings.setValue("prefs/rp/check-updates", check_updates->isChecked());

  settings.setValue("workspace", tabs->currentIndex());

  for (int w=0; w < workspace_max; w++) {
    settings.setValue(QString("workspace%1zoom").arg(w),
		      workspaces[w]->property("zoom"));
  }

  settings.setValue("windowState", saveState());
}

void MainWindow::loadFile(const QString &fileName, SonicPiScintilla* &text)
{
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly)) {
    QMessageBox::warning(this, tr("Sonic Pi"),
			 tr("Cannot read file %1:\n%2.")
			 .arg(fileName)
			 .arg(file.errorString()));
    return;
  }

  QTextStream in(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  text->setText(in.readAll());
  QApplication::restoreOverrideCursor();
  statusBar()->showMessage(tr("File loaded..."), 2000);
}

bool MainWindow::saveFile(const QString &fileName, SonicPiScintilla* text)
{
  QFile file(fileName);
  if (!file.open(QFile::WriteOnly)) {
    QMessageBox::warning(this, tr("Sonic Pi"),
			 tr("Cannot write file %1:\n%2.")
			 .arg(fileName)
			 .arg(file.errorString()));
    return false;
  }

  QTextStream out(&file);

  QApplication::setOverrideCursor(Qt::WaitCursor);
  QString code = text->text();
#if defined(Q_OS_WIN)
  code.replace("\n", "\r\n"); // CRLF for Windows users
  code.replace("\r\r\n", "\r\n"); // don't double-replace if already encoded
#endif
  out << code;
  QApplication::restoreOverrideCursor();

  statusBar()->showMessage(tr("File saved..."), 2000);
  return true;
}

SonicPiScintilla* MainWindow::filenameToWorkspace(std::string filename)
{
  std::string s;

  for(int i = 0; i < workspace_max; i++) {
    s = "workspace_" + number_name(i);
    if(filename == s) {
      return workspaces[i];
    }
  }
  return workspaces[0];
}

void MainWindow::onExitCleanup()
{
  if(serverProcess->state() == QProcess::NotRunning) {
    std::cout << "[GUI] - warning, server process is not running." << std::endl;
    sonicPiServer->stopServer();
    if(protocol == TCP){
      clientSock->close();
    }
  } else {
    if (loaded_workspaces)
      saveWorkspaces();
    sleep(1);
    std::cout << "[GUI] - asking server process to exit..." << std::endl;
    Message msg("/exit");
    sendOSC(msg);
  }
  if(protocol == UDP){
    osc_thread.waitForFinished();
  }
  std::cout << "[GUI] - exiting. Cheerio :-)" << std::endl;

}

void MainWindow::updateDocPane(QListWidgetItem *cur) {
  QString content = cur->data(32).toString();
  docPane->setHtml(content);
}

void MainWindow::updateDocPane2(QListWidgetItem *cur, QListWidgetItem *prev) {
  (void)prev;
  updateDocPane(cur);
}

void MainWindow::setHelpText(QListWidgetItem *item, const QString filename) {
  QFile file(filename);

  if(!file.open(QFile::ReadOnly | QFile::Text)) {
  }

  QString s;
  QTextStream st(&file);
  st.setCodec("UTF-8");
  s.append(st.readAll());

  item->setData(32, QVariant(s));
}

void MainWindow::addHelpPage(QListWidget *nameList,
                             struct help_page *helpPages, int len) {
  int i;
  struct help_entry entry;
  entry.pageIndex = docsCentral->count()-1;

  for(i = 0; i < len; i++) {
    QListWidgetItem *item = new QListWidgetItem(helpPages[i].title);
    setHelpText(item, QString(helpPages[i].filename));
    item->setSizeHint(QSize(item->sizeHint().width(), 25));
    nameList->addItem(item);
    entry.entryIndex = nameList->count()-1;

    if (helpPages[i].keyword != NULL) {
      helpKeywords.insert(helpPages[i].keyword, entry);
      // magic numbers ahoy
      // to be revamped along with the help system
      switch (entry.pageIndex) {
      case 2:
        autocomplete->addSymbol(SonicPiAPIs::Synth, helpPages[i].keyword);
        break;
      case 3:
        autocomplete->addSymbol(SonicPiAPIs::FX, helpPages[i].keyword);
        break;
      case 5:
        autocomplete->addKeyword(SonicPiAPIs::Func, helpPages[i].keyword);
        break;
      }
    }
  }
}

QListWidget *MainWindow::createHelpTab(QString name) {
  QListWidget *nameList = new QListWidget;
  connect(nameList,
	  SIGNAL(itemPressed(QListWidgetItem*)),
	  this, SLOT(updateDocPane(QListWidgetItem*)));
  connect(nameList,
	  SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
	  this, SLOT(updateDocPane2(QListWidgetItem*, QListWidgetItem*)));

  QShortcut *up = new QShortcut(ctrlKey('p'), nameList);
  up->setContext(Qt::WidgetShortcut);
  connect(up, SIGNAL(activated()), this, SLOT(helpScrollUp()));
  QShortcut *down = new QShortcut(ctrlKey('n'), nameList);
  down->setContext(Qt::WidgetShortcut);
  connect(down, SIGNAL(activated()), this, SLOT(helpScrollDown()));

  QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
  layout->addWidget(nameList);
  layout->setStretch(1, 1);
  QWidget *tabWidget = new QWidget;
  tabWidget->setLayout(layout);
  docsCentral->addTab(tabWidget, name);
  helpLists.append(nameList);
  return nameList;
}

void MainWindow::helpScrollUp() {
  int section = docsCentral->currentIndex();
  int entry = helpLists[section]->currentRow();

  if (entry > 0)
    entry--;
  helpLists[section]->setCurrentRow(entry);
}

void MainWindow::helpScrollDown() {
  int section = docsCentral->currentIndex();
  int entry = helpLists[section]->currentRow();

  if (entry < helpLists[section]->count()-1)
    entry++;
  helpLists[section]->setCurrentRow(entry);
}

void MainWindow::docScrollUp() {
  docPane->verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
}

void MainWindow::docScrollDown() {
  docPane->verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
}

void MainWindow::helpClosed(bool visible) {
  if (visible) return;
  // redock on close
  if (!hidingDocPane)
    docWidget->setFloating(false);
  hidingDocPane = false;
}

void MainWindow::tabNext() {
  int index = tabs->currentIndex();
  if (index == tabs->count()-1)
    index = 0;
  else
    index++;
  QMetaObject::invokeMethod(tabs, "setCurrentIndex", Q_ARG(int, index));
}

void MainWindow::tabPrev() {
  int index = tabs->currentIndex();
  if (index == 0)
    index = tabs->count() - 1;
  else
    index--;
  QMetaObject::invokeMethod(tabs, "setCurrentIndex", Q_ARG(int, index));
}

void MainWindow::addUniversalCopyShortcuts(QTextEdit *te){
  new QShortcut(ctrlKey('c'), te, SLOT(copy()));
  new QShortcut(ctrlKey('a'), te, SLOT(selectAll()));

  new QShortcut(metaKey('c'), te, SLOT(copy()));
  new QShortcut(metaKey('a'), te, SLOT(selectAll()));
}

void MainWindow::printAsciiArtLogo(){

  QFile file(":/images/logo.txt");
  if(!file.open(QFile::ReadOnly | QFile::Text)) {
  }

  QString s;
  QTextStream st(&file);
  st.setCodec("UTF-8");
  s.append(st.readAll());
  std::cout << std::endl << std::endl << std::endl;
#if QT_VERSION >= 0x050400
  qDebug().noquote() << s;
  std::cout << std::endl << std::endl;
#else
  //noquote requires QT 5.4
  qDebug() << s;
  std::cout << std::endl;
#endif


}

#include "ruby_help.h"
