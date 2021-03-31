#include "frmmain.h"
#include "ui_frmmain.h"
#include "engine.h"
#include "levelmeter.h"
#include "waveform.h"
#include "progressbar.h"
#include "settingsdialog.h"
#include "spectrograph.h"
#include "tonegeneratordialog.h"
#include "utils.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStyle>
#include <QMenu>
#include <QFileDialog>
#include <QTimerEvent>
#include <QMessageBox>
#include <QPainter>

const int NullTimerId = -1;
frmMain::frmMain(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::frmMain),
    m_infoMessageTimerId(NullTimerId),
    m_mode(NoMode),
    m_engine(new Engine(this)),
#ifndef DISABLE_WAVEFORM
    m_waveform(new Waveform(this))
#endif
{
    ui->setupUi(this);
    connectUi();
    ui->w_Spectrograph->setParams(SpectrumNumBands, SpectrumLowFreq, SpectrumHighFreq);
    connect(ui->btn_OpenFile, SIGNAL(clicked()), this, SLOT(showFileDialog()));
    connect(ui->btn_Analysis, SIGNAL(clicked()), this, SLOT(playAnalysis()));

    ui->btn_Analysis->setEnabled(false);
    setFixedSize(800,550);
    setWindowTitle(tr("Spectrum Analyser"));
    m_waveform->setGeometry(201,82,575,70);
}

frmMain::~frmMain()
{
    delete ui;
}

void frmMain::stateChanged(QAudio::Mode mode, QAudio::State state)
{
    Q_UNUSED(mode);

    updateButtonStates();

    if (QAudio::ActiveState != state && QAudio::SuspendedState != state) {
        ui->w_LevelMeter->reset();
        ui->w_Spectrograph->reset();
    }
}

void frmMain::formatChanged(const QAudioFormat &format)
{
   infoMessage(formatToString(format), NullMessageTimeout);

#ifndef DISABLE_WAVEFORM
    if (QAudioFormat() != format) {
        m_waveform->initialize(format, WaveformTileLength,
                               WaveformWindowDuration);
    }
#endif
}

void frmMain::spectrumChanged(qint64 position, qint64 length,
                                 const FrequencySpectrum &spectrum)
{
    ui->w_ProgressBar->windowChanged(position, length);
    ui->w_Spectrograph->spectrumChanged(spectrum);
}

void frmMain::infoMessage(const QString &message, int timeoutMs)
{
    if (NullTimerId != m_infoMessageTimerId) {
        killTimer(m_infoMessageTimerId);
        m_infoMessageTimerId = NullTimerId;
    }

    if (NullMessageTimeout != timeoutMs)
        m_infoMessageTimerId = startTimer(timeoutMs);
}

void frmMain::errorMessage(const QString &heading, const QString &detail)
{
    QMessageBox::warning(this, heading, detail, QMessageBox::Close);
}

void frmMain::timerEvent(QTimerEvent *event)
{
    Q_ASSERT(event->timerId() == m_infoMessageTimerId);
    Q_UNUSED(event) // suppress warnings in release builds
    killTimer(m_infoMessageTimerId);
    m_infoMessageTimerId = NullTimerId;
}

void frmMain::audioPositionChanged(qint64 position)
{
#ifndef DISABLE_WAVEFORM
    m_waveform->audioPositionChanged(position);
#else
    Q_UNUSED(position)
#endif
}

void frmMain::bufferLengthChanged(qint64 length)
{
    ui->w_ProgressBar->bufferLengthChanged(length);
}

void frmMain::showFileDialog()
{
    ui->w_Spectrograph->reset();
    m_waveform->reset();
    const QString dir;
    const QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open WAV file"), dir, "*.wav");
    if(fileNames.count() == 0)
        return;

    if (fileNames.count()) {
        reset();
        m_engine->loadFile(fileNames.front());
        updateButtonStates();
        ui->btn_Analysis->setEnabled(true);
    }
    ui->lbl_FilePath->setText(fileNames.front());
}

void frmMain::connectUi()
{
    CHECKED_CONNECT(m_engine, SIGNAL(stateChanged(QAudio::Mode,QAudio::State)),
            this, SLOT(stateChanged(QAudio::Mode,QAudio::State)));

    CHECKED_CONNECT(m_engine, SIGNAL(formatChanged(const QAudioFormat &)),
            this, SLOT(formatChanged(const QAudioFormat &)));

    ui->w_ProgressBar->bufferLengthChanged(m_engine->bufferLength());

    CHECKED_CONNECT(m_engine, SIGNAL(bufferLengthChanged(qint64)),
            this, SLOT(bufferLengthChanged(qint64)));

    CHECKED_CONNECT(m_engine, SIGNAL(dataLengthChanged(qint64)),
            this, SLOT(updateButtonStates()));

    CHECKED_CONNECT(m_engine, SIGNAL(recordPositionChanged(qint64)),
            ui->w_ProgressBar, SLOT(recordPositionChanged(qint64)));

    CHECKED_CONNECT(m_engine, SIGNAL(playPositionChanged(qint64)),
            ui->w_ProgressBar, SLOT(playPositionChanged(qint64)));

    CHECKED_CONNECT(m_engine, SIGNAL(recordPositionChanged(qint64)),
            this, SLOT(audioPositionChanged(qint64)));

    CHECKED_CONNECT(m_engine, SIGNAL(playPositionChanged(qint64)),
            this, SLOT(audioPositionChanged(qint64)));

    CHECKED_CONNECT(m_engine, SIGNAL(levelChanged(qreal, qreal, int)),
            ui->w_LevelMeter, SLOT(levelChanged(qreal, qreal, int)));

    CHECKED_CONNECT(m_engine, SIGNAL(spectrumChanged(qint64, qint64, const FrequencySpectrum &)),
            this, SLOT(spectrumChanged(qint64, qint64, const FrequencySpectrum &)));

    CHECKED_CONNECT(m_engine, SIGNAL(infoMessage(QString, int)),
            this, SLOT(infoMessage(QString, int)));

    CHECKED_CONNECT(m_engine, SIGNAL(errorMessage(QString, QString)),
            this, SLOT(errorMessage(QString, QString)));

    CHECKED_CONNECT(ui->w_Spectrograph, SIGNAL(infoMessage(QString, int)),
            this, SLOT(infoMessage(QString, int)));

#ifndef DISABLE_WAVEFORM
    CHECKED_CONNECT(m_engine, SIGNAL(bufferChanged(qint64, qint64, const QByteArray &)),
            m_waveform, SLOT(bufferChanged(qint64, qint64, const QByteArray &)));
#endif
}

void frmMain::updateButtonStates()
{
    const bool playEnabled = (/*m_engine->dataLength() &&*/
                              (QAudio::AudioOutput != m_engine->mode() ||
                               (QAudio::ActiveState != m_engine->state() &&
                                QAudio::IdleState != m_engine->state())));
    ui->btn_Analysis->setEnabled(playEnabled);
}

void frmMain::reset()
{
#ifndef DISABLE_WAVEFORM
    m_waveform->reset();
#endif
    m_engine->reset();
    ui->w_LevelMeter->reset();
    ui->w_Spectrograph->reset();
    ui->w_ProgressBar->reset();
}

void frmMain::setMode(Mode mode)
{
    m_mode = mode;
}

void frmMain::playAnalysis()
{
    ui->w_Spectrograph->barsClear();
    m_engine->startPlayback();
}
