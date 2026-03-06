#include "mainwindow.h"

#include <QApplication>
#include <vector>
#include <QKeyEvent>
#include <QFileInfo>
#include <QPixmap>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QSplitter>
#include <QFrame>
#include <QScrollArea>

// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    buildUi();
    applyStyles();

    // Wire up Canon controller callbacks
    m_ctrl.onStatus = [this](const std::string& msg) {
        QMetaObject::invokeMethod(this, [this, msg]() {
            setStatus(QString::fromStdString(msg), "#00c853");
            logAction(QString::fromStdString(msg));
        }, Qt::QueuedConnection);
    };

    m_ctrl.onError = [this](const std::string& msg) {
        QMetaObject::invokeMethod(this, [this, msg]() {
            setStatus(QString::fromStdString(msg), "#d50000");
            logAction("ERROR: " + QString::fromStdString(msg));
        }, Qt::QueuedConnection);
    };

    m_ctrl.onImageSaved = [this](const std::string& path) {
        QMetaObject::invokeMethod(this, [this, path]() {
            logAction("Image saved: " + QString::fromStdString(path));
            addThumbnail(QString::fromStdString(path));
        }, Qt::QueuedConnection);
    };

    m_ctrl.onLiveViewFrame = [this](const std::vector<uint8_t>& jpeg) {
        QMetaObject::invokeMethod(this, [this, jpeg]() {
            QPixmap pix;
            if (pix.loadFromData(jpeg.data(), static_cast<int>(jpeg.size()), "JPEG")) {
                m_cameraPreview->setPixmap(
                    pix.scaled(m_cameraPreview->size(),
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation));
            }
        }, Qt::QueuedConnection);
    };

    // SDK pump timer (event processing)
    m_pumpTimer = new QTimer(this);
    connect(m_pumpTimer, &QTimer::timeout, this, &MainWindow::onPumpTimer);
    m_pumpTimer->start(50);

    // Live view frame grab timer (~30 fps)
    m_liveViewTimer = new QTimer(this);
    connect(m_liveViewTimer, &QTimer::timeout, this, [this]() {
        m_ctrl.grabLiveViewFrame();
    });

    // Initialize SDK
    if (!m_ctrl.initialize()) {
        setStatus("Canon SDK init failed", "#d50000");
    } else {
        setStatus("Ready — scan or enter a part number", "#ffffff");
    }
}

MainWindow::~MainWindow()
{
    m_pumpTimer->stop();
}

// ---------------------------------------------------------------------------
void MainWindow::buildUi()
{
    setWindowTitle("ZU Documentation");
    resize(1280, 800);

    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---- HEADER ----
    QWidget* header = new QWidget;
    header->setObjectName("header");
    header->setFixedHeight(80);
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 0, 20, 0);

    m_headerTitle = new QLabel("ZU Documentation — Emergency Part Photo Capture");
    m_headerTitle->setObjectName("headerTitle");

    m_headerStatus = new QLabel("Initializing...");
    m_headerStatus->setObjectName("headerStatus");
    m_headerStatus->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QPushButton* exitBtn = new QPushButton("Exit");
    exitBtn->setObjectName("exitBtn");
    exitBtn->setFixedSize(80, 36);
    connect(exitBtn, &QPushButton::clicked, qApp, &QApplication::quit);

    headerLayout->addWidget(m_headerTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(m_headerStatus);
    headerLayout->addSpacing(20);
    headerLayout->addWidget(exitBtn);

    rootLayout->addWidget(header);

    // ---- MAIN AREA (left + center + right) ----
    QWidget* mainArea = new QWidget;
    QHBoxLayout* mainLayout = new QHBoxLayout(mainArea);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(1);

    // -- LEFT PANEL --
    QWidget* leftPanel = new QWidget;
    leftPanel->setObjectName("leftPanel");
    leftPanel->setFixedWidth(320);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(16, 24, 16, 24);
    leftLayout->setSpacing(16);

    QLabel* partLabel = new QLabel("Part Number");
    partLabel->setObjectName("inputLabel");
    m_partNumberEdit = new QLineEdit;
    m_partNumberEdit->setObjectName("inputField");
    m_partNumberEdit->setPlaceholderText("Scan or enter part number...");
    m_partNumberEdit->setFixedHeight(48);
    connect(m_partNumberEdit, &QLineEdit::textChanged,   this, &MainWindow::onPartNumberChanged);
    // Barcode scanners send Enter after the code — auto-start immediately
    connect(m_partNumberEdit, &QLineEdit::returnPressed, this, &MainWindow::onStartClicked);

    QLabel* pathLabel = new QLabel("Save Path");
    pathLabel->setObjectName("inputLabel");
    m_savePathEdit = new QLineEdit;
    m_savePathEdit->setObjectName("inputField");
    m_savePathEdit->setText("C:\\Users\\vrbicama\\Pictures\\Versuche");
    m_savePathEdit->setFixedHeight(48);

    m_startBtn = new QPushButton("Start");
    m_startBtn->setObjectName("startBtn");
    m_startBtn->setFixedHeight(56);
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartClicked);

    leftLayout->addWidget(partLabel);
    leftLayout->addWidget(m_partNumberEdit);
    leftLayout->addSpacing(8);
    leftLayout->addWidget(pathLabel);
    leftLayout->addWidget(m_savePathEdit);
    leftLayout->addSpacing(24);
    leftLayout->addWidget(m_startBtn);
    leftLayout->addStretch();

    // -- CENTER PANEL --
    QWidget* centerPanel = new QWidget;
    centerPanel->setObjectName("centerPanel");
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(16, 24, 16, 16);
    centerLayout->setSpacing(12);

    m_cameraPreview = new QLabel("No camera connected\nConnect via USB and press Start");
    m_cameraPreview->setObjectName("cameraPreview");
    m_cameraPreview->setAlignment(Qt::AlignCenter);
    m_cameraPreview->setFixedSize(640, 480);

    m_captureBtn = new QPushButton("Capture Photo  [Space]");
    m_captureBtn->setObjectName("captureBtn");
    m_captureBtn->setFixedHeight(64);
    m_captureBtn->setEnabled(false);
    connect(m_captureBtn, &QPushButton::clicked, this, &MainWindow::onCaptureClicked);

    centerLayout->addWidget(m_cameraPreview);
    centerLayout->addWidget(m_captureBtn);

    // -- RIGHT PANEL --
    QWidget* rightPanel = new QWidget;
    rightPanel->setObjectName("rightPanel");
    rightPanel->setFixedWidth(320);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(16, 24, 16, 24);
    rightLayout->setSpacing(16);

    QLabel* capturesLabel = new QLabel("Captured Images");
    capturesLabel->setObjectName("inputLabel");

    m_thumbnailList = new QListWidget;
    m_thumbnailList->setObjectName("thumbnailList");
    m_thumbnailList->setIconSize(QSize(100, 100));
    m_thumbnailList->setViewMode(QListView::IconMode);
    m_thumbnailList->setResizeMode(QListView::Adjust);
    m_thumbnailList->setSpacing(6);

    m_finishedBtn = new QPushButton("Finished  [F]");
    m_finishedBtn->setObjectName("finishedBtn");
    m_finishedBtn->setFixedHeight(56);
    m_finishedBtn->setEnabled(false);
    connect(m_finishedBtn, &QPushButton::clicked, this, &MainWindow::onFinishedClicked);

    rightLayout->addWidget(capturesLabel);
    rightLayout->addWidget(m_thumbnailList);
    rightLayout->addWidget(m_finishedBtn);

    mainLayout->addWidget(leftPanel);
    mainLayout->addWidget(centerPanel, 1);
    mainLayout->addWidget(rightPanel);

    rootLayout->addWidget(mainArea, 1);

    // ---- FOOTER ----
    QWidget* footer = new QWidget;
    footer->setObjectName("footer");
    footer->setFixedHeight(100);
    QVBoxLayout* footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(16, 8, 16, 8);

    QLabel* footerHint = new QLabel("Shortcuts:  Enter = Start  |  Space = Capture  |  F = Finished  |  Esc = Exit");
    footerHint->setObjectName("footerHint");

    m_logArea = new QTextEdit;
    m_logArea->setObjectName("logArea");
    m_logArea->setReadOnly(true);
    m_logArea->setFixedHeight(60);

    footerLayout->addWidget(footerHint);
    footerLayout->addWidget(m_logArea);

    rootLayout->addWidget(footer);
}

// ---------------------------------------------------------------------------
void MainWindow::applyStyles()
{
    setStyleSheet(R"(
        QWidget {
            background-color: #1e1e2e;
            color: #cdd6f4;
            font-family: Arial;
            font-size: 14px;
        }
        #header {
            background-color: #181825;
            border-bottom: 2px solid #313244;
        }
        #headerTitle {
            font-size: 18px;
            font-weight: bold;
            color: #cdd6f4;
        }
        #headerStatus {
            font-size: 14px;
            color: #ffffff;
        }
        #exitBtn {
            background-color: #f38ba8;
            color: #1e1e2e;
            border: none;
            border-radius: 6px;
            font-weight: bold;
        }
        #exitBtn:hover { background-color: #eba0ac; }

        #leftPanel, #rightPanel {
            background-color: #181825;
            border-right: 1px solid #313244;
        }
        #rightPanel { border-right: none; border-left: 1px solid #313244; }

        #centerPanel { background-color: #1e1e2e; }

        #inputLabel {
            font-size: 13px;
            color: #a6adc8;
            font-weight: bold;
        }
        #inputField {
            background-color: #313244;
            border: 1px solid #45475a;
            border-radius: 6px;
            padding: 8px;
            color: #cdd6f4;
            font-size: 16px;
        }
        #inputField:focus { border: 1px solid #89b4fa; }

        #startBtn {
            background-color: #89b4fa;
            color: #1e1e2e;
            border: none;
            border-radius: 8px;
            font-size: 18px;
            font-weight: bold;
        }
        #startBtn:hover { background-color: #b4befe; }
        #startBtn:disabled { background-color: #45475a; color: #6c7086; }

        #cameraPreview {
            background-color: #11111b;
            border: 2px dashed #45475a;
            border-radius: 8px;
            color: #6c7086;
            font-size: 18px;
        }

        #captureBtn {
            background-color: #a6e3a1;
            color: #1e1e2e;
            border: none;
            border-radius: 8px;
            font-size: 20px;
            font-weight: bold;
        }
        #captureBtn:hover { background-color: #94e2d5; }
        #captureBtn:disabled { background-color: #45475a; color: #6c7086; }

        #thumbnailList {
            background-color: #11111b;
            border: 1px solid #313244;
            border-radius: 6px;
        }

        #finishedBtn {
            background-color: #fab387;
            color: #1e1e2e;
            border: none;
            border-radius: 8px;
            font-size: 18px;
            font-weight: bold;
        }
        #finishedBtn:hover { background-color: #f9e2af; }
        #finishedBtn:disabled { background-color: #45475a; color: #6c7086; }

        #footer {
            background-color: #181825;
            border-top: 2px solid #313244;
        }
        #footerHint {
            color: #6c7086;
            font-size: 12px;
        }
        #logArea {
            background-color: #11111b;
            border: 1px solid #313244;
            border-radius: 4px;
            color: #a6adc8;
            font-size: 12px;
            font-family: Consolas, monospace;
        }
    )");
}

// ---------------------------------------------------------------------------
void MainWindow::setStatus(const QString& msg, const QString& color)
{
    m_headerStatus->setText(msg);
    m_headerStatus->setStyleSheet("color: " + color + "; font-size: 14px;");
}

void MainWindow::logAction(const QString& msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logArea->append("[" + timestamp + "] " + msg);
}

// ---------------------------------------------------------------------------
void MainWindow::onPartNumberChanged(const QString& text)
{
    setSavePathFromPart(text);
}

void MainWindow::setSavePathFromPart(const QString& partNumber)
{
    if (partNumber.isEmpty()) {
        m_savePathEdit->setText("C:\\Users\\vrbicama\\Pictures\\Versuche");
    } else {
        m_savePathEdit->setText("C:\\Users\\vrbicama\\Pictures\\Versuche\\" + partNumber);
    }
}

// ---------------------------------------------------------------------------
void MainWindow::onStartClicked()
{
    QString partNumber = m_partNumberEdit->text().trimmed();
    if (partNumber.isEmpty()) {
        setStatus("Enter or scan a part number first", "#ffb300");
        return;
    }

    m_ctrl.savePath   = m_savePathEdit->text().toStdString();
    m_ctrl.partNumber = partNumber.toStdString();

    if (!m_ctrl.isConnected()) {
        setStatus("Connecting to camera...", "#89b4fa");
        if (!m_ctrl.connectCamera()) {
            setStatus("No camera found — connect via USB", "#d50000");
            return;
        }
    }

    m_ctrl.startCapture();

    // Start live view — if the camera doesn't support it just show a message
    if (m_ctrl.startLiveView()) {
        m_liveViewTimer->start(33);   // ~30 fps
        m_cameraPreview->clear();
    } else {
        m_cameraPreview->setText("Live View not available for this camera.\nTake photos with the camera shutter.");
    }

    m_partNumberEdit->setEnabled(false);
    m_savePathEdit->setEnabled(false);
    m_startBtn->setEnabled(false);
    m_captureBtn->setEnabled(true);
    m_finishedBtn->setEnabled(true);
    m_sessionActive = true;

    setStatus("Capturing for part: " + partNumber, "#00c853");
    logAction("Session started for part: " + partNumber);
}

// ---------------------------------------------------------------------------
void MainWindow::onCaptureClicked()
{
    // With Canon SDK, photos are triggered from the camera body.
    // This button is a placeholder for tethered capture trigger if needed.
    logAction("Manual capture triggered (use camera shutter to take photo)");
}

// ---------------------------------------------------------------------------
void MainWindow::onFinishedClicked()
{
    int count = m_thumbnailList->count();
    QString msg = QString("Part %1 done — %2 image(s) saved. Load next part?")
                      .arg(QString::fromStdString(m_ctrl.partNumber))
                      .arg(count);

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Part Finished", msg,
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        resetForNextPart();
    }
}

// ---------------------------------------------------------------------------
void MainWindow::resetForNextPart()
{
    if (m_ctrl.isCapturing())      m_ctrl.stopCapture();
    if (m_ctrl.isLiveViewActive()) {
        m_liveViewTimer->stop();
        m_ctrl.stopLiveView();
    }

    m_thumbnailList->clear();
    m_partNumberEdit->clear();
    m_partNumberEdit->setEnabled(true);
    m_savePathEdit->setEnabled(true);
    m_startBtn->setEnabled(true);
    m_captureBtn->setEnabled(false);
    m_finishedBtn->setEnabled(false);
    m_cameraPreview->setPixmap(QPixmap());
    m_cameraPreview->setText("No camera connected\nConnect via USB and press Start");
    m_sessionActive = false;

    setStatus("Ready — scan or enter a part number", "#ffffff");
    logAction("Session finished. Ready for next part.");

    m_partNumberEdit->setFocus();
}

// ---------------------------------------------------------------------------
void MainWindow::addThumbnail(const QString& imagePath)
{
    QPixmap pix(imagePath);
    QListWidgetItem* item = new QListWidgetItem;
    if (!pix.isNull()) {
        item->setIcon(QIcon(pix.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    }
    item->setText(QFileInfo(imagePath).fileName());
    item->setToolTip(imagePath);
    m_thumbnailList->addItem(item);
    m_thumbnailList->scrollToBottom();
}

// ---------------------------------------------------------------------------
void MainWindow::onPumpTimer()
{
    m_ctrl.pumpEvents();
}

// ---------------------------------------------------------------------------
void MainWindow::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        qApp->quit();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (!m_sessionActive) onStartClicked();
        break;
    case Qt::Key_Space:
        if (m_captureBtn->isEnabled()) onCaptureClicked();
        break;
    case Qt::Key_F:
        if (m_finishedBtn->isEnabled()) onFinishedClicked();
        break;
    default:
        QMainWindow::keyPressEvent(event);
    }
}
