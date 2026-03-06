#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>

#include "canoncontroller.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onStartClicked();
    void onCaptureClicked();
    void onFinishedClicked();
    void onPartNumberChanged(const QString& text);
    void onPumpTimer();

private:
    void buildUi();
    void applyStyles();
    void setStatus(const QString& msg, const QString& color = "#ffffff");
    void logAction(const QString& msg);
    void addThumbnail(const QString& imagePath);
    void resetForNextPart();
    void setSavePathFromPart(const QString& partNumber);

    // --- UI elements ---
    // Header
    QLabel*      m_headerTitle  = nullptr;
    QLabel*      m_headerStatus = nullptr;

    // Left panel
    QLineEdit*   m_partNumberEdit = nullptr;
    QLineEdit*   m_savePathEdit   = nullptr;
    QPushButton* m_startBtn       = nullptr;

    // Center panel
    QLabel*      m_cameraPreview  = nullptr;   // placeholder; replace with video widget later
    QPushButton* m_captureBtn     = nullptr;

    // Right panel
    QListWidget* m_thumbnailList  = nullptr;
    QPushButton* m_finishedBtn    = nullptr;

    // Footer
    QTextEdit*   m_logArea        = nullptr;

    // --- Backend ---
    CanonController m_ctrl;
    QTimer*         m_pumpTimer   = nullptr;
    bool            m_sessionActive = false;
};
