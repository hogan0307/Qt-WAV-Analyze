#ifndef FRMMAIN_H
#define FRMMAIN_H

#include <QWidget>
#include <QAudio>
#include <QIcon>
#include <QAudioFormat>

class Engine;
class FrequencySpectrum;
class LevelMeter;
class ProgressBar;
class SettingsDialog;
class Spectrograph;
class ToneGeneratorDialog;
class Waveform;

namespace Ui {
class frmMain;
}

class frmMain : public QWidget
{
    Q_OBJECT

public:
    explicit frmMain(QWidget *parent = nullptr);
    ~frmMain();
    void timerEvent(QTimerEvent *event) override;

public slots:
    void stateChanged(QAudio::Mode mode, QAudio::State state);
    void formatChanged(const QAudioFormat &format);
    void spectrumChanged(qint64 position, qint64 length,
                         const FrequencySpectrum &spectrum);
    void infoMessage(const QString &message, int timeoutMs);
    void errorMessage(const QString &heading, const QString &detail);
    void audioPositionChanged(qint64 position);
    void bufferLengthChanged(qint64 length);

private slots:
    void showFileDialog();
    void updateButtonStates();
    void playAnalysis();

private:
    Ui::frmMain *ui;
    void connectUi();
    void reset();

    enum Mode {
        NoMode,
        RecordMode,
        GenerateToneMode,
        LoadFileMode
    };

    void setMode(Mode mode);

private:
    Mode                    m_mode;

    Engine*                 m_engine;

#ifndef DISABLE_WAVEFORM
    Waveform*               m_waveform;
#endif
    int                     m_infoMessageTimerId;
};

#endif // FRMMAIN_H
