// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors
#include "generalconf.h"
#include "imgupload/imguploadermanager.h"
#include "imgupload/storages/imguploaderbase.h"
#include "src/utils/confighandler.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QGuiApplication>
#include <QImageWriter>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTextCodec>
#include <QVBoxLayout>
#include <QScreen>

GeneralConf::GeneralConf(QWidget* parent)
  : QWidget(parent)
  , m_historyConfirmationToDelete(nullptr)
  , m_undoLimit(nullptr)
{

    m_scrollAreaLayout = new QVBoxLayout(this);
    m_scrollAreaLayout->setContentsMargins(0, 0, 0, 0);


    // Scroll area adapts the size of the content on small screens.
    // It must be initialized before the checkboxes.
    initScrollArea();

    initAutostart();
#if !defined(Q_OS_WIN)
    initAutoCloseIdleDaemon();
#endif
    initShowTrayIcon();
    initShowDesktopNotification();
#if !defined(DISABLE_UPDATE_CHECKER)
    initCheckForUpdates();
#endif
    initShowStartupLaunchMessage();
    initAllowMultipleGuiInstances();
    initSaveLastRegion();
    initShowHelp();
    initShowSidePanelButton();
    initUseJpgForClipboard();
    initCopyOnDoubleClick();
    initServerTPU();
    initWindowOffsets();
    initSaveAfterCopy();
    initCopyPathAfterSave();
    initCopyAndCloseAfterUpload();
    initUploadWithoutConfirmation();
    initHistoryConfirmationToDelete();
    initAntialiasingPinZoom();
    initUploadHistoryMax();
    initUndoLimit();
    // initUploadClientSecret();
    initPredefinedColorPaletteLarge();
    initShowSelectionGeometry();
    initShowMagnifier();
    initSquareMagnifier();
    initJpegQuality();
    // this has to be at the end
    initConfigButtons();
    updateComponents();
}

void GeneralConf::_updateComponents(bool allowEmptySavePath)
{
    ConfigHandler config;
    m_helpMessage->setChecked(config.showHelp());
    m_sidePanelButton->setChecked(config.showSidePanelButton());
    m_sysNotifications->setChecked(config.showDesktopNotification());
    m_autostart->setChecked(config.startupLaunch());
    m_copyURLAfterUpload->setChecked(config.copyURLAfterUpload());
    m_saveAfterCopy->setChecked(config.saveAfterCopy());
    m_copyPathAfterSave->setChecked(config.copyPathAfterSave());
    m_antialiasingPinZoom->setChecked(config.antialiasingPinZoom());
    m_useJpgForClipboard->setChecked(config.useJpgForClipboard());
    m_copyOnDoubleClick->setChecked(config.copyOnDoubleClick());
    m_uploadWithoutConfirmation->setChecked(config.uploadWithoutConfirmation());
    m_historyConfirmationToDelete->setChecked(
      config.historyConfirmationToDelete());
#if !defined(DISABLE_UPDATE_CHECKER)
    m_checkForUpdates->setChecked(config.checkForUpdates());
#endif
    m_allowMultipleGuiInstances->setChecked(config.allowMultipleGuiInstances());
    m_showMagnifier->setChecked(config.showMagnifier());
    m_squareMagnifier->setChecked(config.squareMagnifier());
    m_saveLastRegion->setChecked(config.saveLastRegion());

#if !defined(Q_OS_WIN)
    m_autoCloseIdleDaemon->setChecked(config.autoCloseIdleDaemon());
#endif

    m_predefinedColorPaletteLarge->setChecked(
      config.predefinedColorPaletteLarge());
    m_showStartupLaunchMessage->setChecked(config.showStartupLaunchMessage());
    m_screenshotPathFixedCheck->setChecked(config.savePathFixed());
    m_uploadHistoryMax->setValue(config.uploadHistoryMax());
    m_undoLimit->setValue(config.undoLimit());

    if (allowEmptySavePath || !config.savePath().isEmpty()) {
        m_savePath->setText(config.savePath());
    }
#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
    m_showTray->setChecked(!config.disabledTrayIcon());
#endif
}

void GeneralConf::updateComponents()
{
    _updateComponents(false);
}

void GeneralConf::saveLastRegion(bool checked)
{
    ConfigHandler().setSaveLastRegion(checked);
}

void GeneralConf::showHelpChanged(bool checked)
{
    ConfigHandler().setShowHelp(checked);
}

void GeneralConf::showSidePanelButtonChanged(bool checked)
{
    ConfigHandler().setShowSidePanelButton(checked);
}

void GeneralConf::showDesktopNotificationChanged(bool checked)
{
    ConfigHandler().setShowDesktopNotification(checked);
}

#if !defined(DISABLE_UPDATE_CHECKER)
void GeneralConf::checkForUpdatesChanged(bool checked)
{
    ConfigHandler().setCheckForUpdates(checked);
}
#endif

void GeneralConf::allowMultipleGuiInstancesChanged(bool checked)
{
    ConfigHandler().setAllowMultipleGuiInstances(checked);
}

void GeneralConf::autoCloseIdleDaemonChanged(bool checked)
{
    ConfigHandler().setAutoCloseIdleDaemon(checked);
}

void GeneralConf::autostartChanged(bool checked)
{
    ConfigHandler().setStartupLaunch(checked);
}

static int openWindowCount = 0;

void GeneralConf::importConfiguration()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import"));
    if (fileName.isEmpty()) {
        return;
    }
    QFile file(fileName);
    QTextCodec* codec = QTextCodec::codecForLocale();
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::about(this, tr("Error"), tr("Unable to read file."));
        return;
    }
    QString text = codec->toUnicode(file.readAll());
    file.close();

    QFile config(ConfigHandler().configFilePath());
    if (!config.open(QFile::WriteOnly)) {
        QMessageBox::about(this, tr("Error"), tr("Unable to write file."));
        return;
    }
    config.write(codec->fromUnicode(text));
    config.close();
}

void GeneralConf::exportFileConfiguration()
{
    QString defaultFileName = QSettings().fileName();
    QString fileName =
      QFileDialog::getSaveFileName(this, tr("Save File"), defaultFileName);

    // Cancel button or target same as source
    if (fileName.isNull() || fileName == defaultFileName) {
        return;
    }

    QFile targetFile(fileName);
    if (targetFile.exists()) {
        targetFile.remove();
    }
    bool ok = QFile::copy(ConfigHandler().configFilePath(), fileName);
    if (!ok) {
        QMessageBox::about(this, tr("Error"), tr("Unable to write file."));
    }
}

void GeneralConf::resetConfiguration()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
      this,
      tr("Confirm Reset"),
      tr("Are you sure you want to reset the configuration?"),
      QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_savePath->setText(
          QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
        ConfigHandler().setDefaultSettings();
        _updateComponents(true);
    }
}

void GeneralConf::initScrollArea()
{
    m_scrollArea = new QScrollArea(this);
    m_scrollAreaLayout->addWidget(m_scrollArea);

    auto* content = new QWidget(m_scrollArea);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidget(content);

    content->setObjectName("content");
    m_scrollArea->setObjectName("scrollArea");
    m_scrollArea->setStyleSheet(
      "#content, #scrollArea { background: transparent; border: 0px; }");
    m_scrollAreaLayout = new QVBoxLayout(content);
    m_scrollAreaLayout->setContentsMargins(0, 0, 30, 0);
}

void GeneralConf::initShowHelp()
{
    m_helpMessage = new QCheckBox(tr("Show help message"), this);
    m_helpMessage->setToolTip(tr("Show the help message at the beginning "
                                 "in the capture mode"));
    m_scrollAreaLayout->addWidget(m_helpMessage);

    connect(
      m_helpMessage, &QCheckBox::clicked, this, &GeneralConf::showHelpChanged);
}

void GeneralConf::initSaveLastRegion()
{
    m_saveLastRegion = new QCheckBox(tr("Use last region for GUI mode"), this);
    m_saveLastRegion->setToolTip(
      tr("Use the last region as the default selection for the next screenshot "
         "in GUI mode"));
    m_scrollAreaLayout->addWidget(m_saveLastRegion);

    connect(m_saveLastRegion,
            &QCheckBox::clicked,
            this,
            &GeneralConf::saveLastRegion);
}

void GeneralConf::initShowSidePanelButton()
{
    m_sidePanelButton = new QCheckBox(tr("Show the side panel button"), this);
    m_sidePanelButton->setToolTip(
      tr("Show the side panel toggle button in the capture mode"));
    m_scrollAreaLayout->addWidget(m_sidePanelButton);

    connect(m_sidePanelButton,
            &QCheckBox::clicked,
            this,
            &GeneralConf::showSidePanelButtonChanged);
}

void GeneralConf::initShowDesktopNotification()
{
    m_sysNotifications = new QCheckBox(tr("Show desktop notifications"), this);
    m_sysNotifications->setToolTip(tr("Enable desktop notifications"));
    m_scrollAreaLayout->addWidget(m_sysNotifications);

    connect(m_sysNotifications,
            &QCheckBox::clicked,
            this,
            &GeneralConf::showDesktopNotificationChanged);
}

void GeneralConf::initShowTrayIcon()
{
#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
    m_showTray = new QCheckBox(tr("Show tray icon"), this);
    m_showTray->setToolTip(tr("Show icon in the system tray"));
    m_scrollAreaLayout->addWidget(m_showTray);

    connect(m_showTray, &QCheckBox::clicked, this, [](bool checked) {
        ConfigHandler().setDisabledTrayIcon(!checked);
    });
#endif
}

void GeneralConf::initHistoryConfirmationToDelete()
{
    m_historyConfirmationToDelete = new QCheckBox(
      tr("Confirmation required to delete screenshot from the latest uploads"),
      this);
    m_historyConfirmationToDelete->setToolTip(
      tr("Ask for confirmation to delete screenshot from the latest uploads"));
    m_scrollAreaLayout->addWidget(m_historyConfirmationToDelete);

    connect(m_historyConfirmationToDelete,
            &QCheckBox::clicked,
            this,
            &GeneralConf::historyConfirmationToDelete);
}

void GeneralConf::initConfigButtons()
{
    auto* buttonLayout = new QHBoxLayout();
    auto* box = new QGroupBox(tr("Configuration File"));
    box->setFlat(true);
    box->setLayout(buttonLayout);
    m_scrollAreaLayout->addWidget(box);

    m_exportButton = new QPushButton(tr("Export"));
    buttonLayout->addWidget(m_exportButton);
    connect(m_exportButton,
            &QPushButton::clicked,
            this,
            &GeneralConf::exportFileConfiguration);

    m_importButton = new QPushButton(tr("Import"));
    buttonLayout->addWidget(m_importButton);
    connect(m_importButton,
            &QPushButton::clicked,
            this,
            &GeneralConf::importConfiguration);

    m_resetButton = new QPushButton(tr("Reset"));
    buttonLayout->addWidget(m_resetButton);
    connect(m_resetButton,
            &QPushButton::clicked,
            this,
            &GeneralConf::resetConfiguration);
}

#if !defined(DISABLE_UPDATE_CHECKER)
void GeneralConf::initCheckForUpdates()
{
    m_checkForUpdates = new QCheckBox(tr("Automatic check for updates"), this);
    m_checkForUpdates->setToolTip(tr("Check for updates automatically"));
    m_scrollAreaLayout->addWidget(m_checkForUpdates);

    connect(m_checkForUpdates,
            &QCheckBox::clicked,
            this,
            &GeneralConf::checkForUpdatesChanged);
}
#endif

void GeneralConf::initAllowMultipleGuiInstances()
{
    m_allowMultipleGuiInstances = new QCheckBox(
      tr("Allow multiple flameshot GUI instances simultaneously"), this);
    m_allowMultipleGuiInstances->setToolTip(tr(
      "This allows you to take screenshots of Flameshot itself for example"));
    m_scrollAreaLayout->addWidget(m_allowMultipleGuiInstances);
    connect(m_allowMultipleGuiInstances,
            &QCheckBox::clicked,
            this,
            &GeneralConf::allowMultipleGuiInstancesChanged);
}

void GeneralConf::initAutoCloseIdleDaemon()
{
    m_autoCloseIdleDaemon = new QCheckBox(
      tr("Automatically unload from memory when it is not needed"), this);
    m_autoCloseIdleDaemon->setToolTip(tr(
      "Automatically close daemon (background process) when it is not needed"));
    m_scrollAreaLayout->addWidget(m_autoCloseIdleDaemon);
    connect(m_autoCloseIdleDaemon,
            &QCheckBox::clicked,
            this,
            &GeneralConf::autoCloseIdleDaemonChanged);
}

void GeneralConf::initAutostart()
{
    m_autostart = new QCheckBox(tr("Launch in background at startup"), this);
    m_autostart->setToolTip(tr(
      "Launch Flameshot daemon (background process) when computer is booted"));
    m_scrollAreaLayout->addWidget(m_autostart);

    connect(
      m_autostart, &QCheckBox::clicked, this, &GeneralConf::autostartChanged);
}

void GeneralConf::initShowStartupLaunchMessage()
{
    m_showStartupLaunchMessage =
      new QCheckBox(tr("Show welcome message on launch"), this);
    ConfigHandler config;
    m_showStartupLaunchMessage->setToolTip(
      tr("Show the welcome message box in the middle of the screen while "
         "taking a screenshot"));
    m_scrollAreaLayout->addWidget(m_showStartupLaunchMessage);

    connect(m_showStartupLaunchMessage, &QCheckBox::clicked, [](bool checked) {
        ConfigHandler().setShowStartupLaunchMessage(checked);
    });
}

void GeneralConf::initPredefinedColorPaletteLarge()
{
    m_predefinedColorPaletteLarge =
      new QCheckBox(tr("Use large predefined color palette"), this);
    m_predefinedColorPaletteLarge->setToolTip(
      tr("Use a large predefined color palette"));
    m_scrollAreaLayout->addWidget(m_predefinedColorPaletteLarge);

    connect(
      m_predefinedColorPaletteLarge, &QCheckBox::clicked, [](bool checked) {
          ConfigHandler().setPredefinedColorPaletteLarge(checked);
      });
}
void GeneralConf::initCopyOnDoubleClick()
{
    m_copyOnDoubleClick = new QCheckBox(tr("Copy on double click"), this);
    m_copyOnDoubleClick->setToolTip(
      tr("Enable Copy to clipboard on Double Click"));
    m_scrollAreaLayout->addWidget(m_copyOnDoubleClick);

    connect(m_copyOnDoubleClick, &QCheckBox::clicked, [](bool checked) {
        ConfigHandler().setCopyOnDoubleClick(checked);
    });
}

void GeneralConf::initCopyAndCloseAfterUpload()
{
    m_copyURLAfterUpload = new QCheckBox(tr("Copy URL after upload"), this);
    m_copyURLAfterUpload->setToolTip(
      tr("Copy URL after uploading was successful"));
    m_scrollAreaLayout->addWidget(m_copyURLAfterUpload);

    connect(m_copyURLAfterUpload, &QCheckBox::clicked, [](bool checked) {
        ConfigHandler().setCopyURLAfterUpload(checked);
    });
}

void GeneralConf::initSaveAfterCopy()
{
    m_saveAfterCopy = new QCheckBox(tr("Save image after copy"), this);
    m_saveAfterCopy->setToolTip(
      tr("After copying the screenshot, save it to a file as well"));
    m_scrollAreaLayout->addWidget(m_saveAfterCopy);
    connect(m_saveAfterCopy,
            &QCheckBox::clicked,
            this,
            &GeneralConf::saveAfterCopyChanged);

    auto* box = new QGroupBox(tr("Save Path"));
    box->setFlat(true);
    m_scrollAreaLayout->addWidget(box);

    auto* vboxLayout = new QVBoxLayout();
    box->setLayout(vboxLayout);

    auto* pathLayout = new QHBoxLayout();

    QString path = ConfigHandler().savePath();
    m_savePath = new QLineEdit(path, this);
    m_savePath->setDisabled(true);
    QString foreground = this->palette().windowText().color().name();
    m_savePath->setStyleSheet(QStringLiteral("color: %1").arg(foreground));
    pathLayout->addWidget(m_savePath);

    m_changeSaveButton = new QPushButton(tr("Change..."), this);
    pathLayout->addWidget(m_changeSaveButton);
    connect(m_changeSaveButton,
            &QPushButton::clicked,
            this,
            &GeneralConf::changeSavePath);

    m_screenshotPathFixedCheck =
      new QCheckBox(tr("Use fixed path for screenshots to save"), this);
    connect(m_screenshotPathFixedCheck,
            &QCheckBox::toggled,
            this,
            &GeneralConf::togglePathFixed);

    vboxLayout->addLayout(pathLayout);
    vboxLayout->addWidget(m_screenshotPathFixedCheck);

    auto* extensionLayout = new QHBoxLayout();

    m_setSaveAsFileExtension = new QComboBox(this);

    QStringList imageFormatList;
    foreach (auto mimeType, QImageWriter::supportedImageFormats())
        imageFormatList.append(mimeType);

    m_setSaveAsFileExtension->addItems(imageFormatList);

    int currentIndex =
      m_setSaveAsFileExtension->findText(ConfigHandler().saveAsFileExtension());
    m_setSaveAsFileExtension->setCurrentIndex(currentIndex);

    connect(m_setSaveAsFileExtension,
            &QComboBox::currentTextChanged,
            this,
            &GeneralConf::setSaveAsFileExtension);

    extensionLayout->addWidget(m_setSaveAsFileExtension);
    extensionLayout->addWidget(
      new QLabel(tr("Preferred save file extension")));
    vboxLayout->addLayout(extensionLayout);
}

void GeneralConf::historyConfirmationToDelete(bool checked)
{
    ConfigHandler().setHistoryConfirmationToDelete(checked);
}

void GeneralConf::initUploadHistoryMax()
{
    auto* layout = new QHBoxLayout();
    auto* layoutLabel = new QLabel(tr("Latest Uploads Max Size"), this);
    m_uploadHistoryMax = new QSpinBox(this);
    m_uploadHistoryMax->setMaximum(50);
    QString foreground = this->palette().windowText().color().name();
    m_uploadHistoryMax->setStyleSheet(QStringLiteral("color: %1").arg(foreground));
    layout->addWidget(m_uploadHistoryMax);
    layout->addWidget(layoutLabel);


    connect(m_uploadHistoryMax,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadHistoryMaxChanged);

    m_scrollAreaLayout->addLayout(layout);

}

void GeneralConf::initUploadClientSecret()
{
    auto* box = new QGroupBox(tr("Imgur Application Client ID"));
    box->setFlat(true);
    m_scrollAreaLayout->addWidget(box);

    auto* vboxLayout = new QVBoxLayout();
    box->setLayout(vboxLayout);

    m_uploadClientKey = new QLineEdit(this);
    QString foreground = this->palette().windowText().color().name();
    m_uploadClientKey->setStyleSheet(
      QStringLiteral("color: %1").arg(foreground));
    m_uploadClientKey->setText(ConfigHandler().uploadClientSecret());
    connect(m_uploadClientKey,
            &QLineEdit::editingFinished,
            this,
            &GeneralConf::uploadClientKeyEdited);
    vboxLayout->addWidget(m_uploadClientKey);
}

void GeneralConf::initServerTPU()
{
    auto* box = new QGroupBox(tr("PrivateUploader"));
    auto* box2 = new QGroupBox();
    box->setFlat(true);
    box2->setFlat(true);
    m_scrollAreaLayout->addWidget(box);
    m_scrollAreaLayout->addWidget(box2);

    auto* hboxLayout = new QHBoxLayout();
    auto* hboxLayoutKey = new QHBoxLayout();
    box->setLayout(hboxLayout);
    box2->setLayout(hboxLayoutKey);


    m_serverTPU = new QLineEdit(this);
    QString foreground = this->palette().windowText().color().name();
    m_serverTPU->setStyleSheet(QStringLiteral("color: %1").arg(foreground));
    m_serverTPU->setText(ConfigHandler().serverTPU());
    connect(m_serverTPU,
            &QLineEdit::editingFinished,
            this,
            &GeneralConf::serverTPUEdited);

    auto* label = new QLabel(tr("Server URL"), this);
    hboxLayout->addWidget(m_serverTPU);
    hboxLayout->addWidget(label);

    m_uploadToken = new QLineEdit(this);
    m_uploadToken->setStyleSheet(QStringLiteral("color: %1").arg(foreground));
    m_uploadToken->setText(ConfigHandler().uploadTokenTPU());
    connect(m_uploadToken,
            &QLineEdit::editingFinished,
            this,
            &GeneralConf::uploadTokenTPUEdited);

    auto* labelAPIKey = new QLabel(tr("API Key"), this);
    hboxLayoutKey->addWidget(m_uploadToken);
    hboxLayoutKey->addWidget(labelAPIKey);
}

void GeneralConf::initWindowOffsets()
{
    auto* box = new QGroupBox(tr("Upload Notification Settings"));

    box->setFlat(true);
    m_scrollAreaLayout->addWidget(box);

    auto* vboxLayout = new QVBoxLayout();
    box->setLayout(vboxLayout);

    m_uploadWindowEnabled =
      new QCheckBox(tr("Enable Upload Notification"), this);
    m_uploadWindowEnabled->setChecked(ConfigHandler().uploadWindowEnabled());
    auto* uploadWindowEnabledWarning =
      new QLabel(tr("WAYLAND USERS: This upload notification window does not "
                    "currently function correctly under Wayland. Please "
                    "disable if you experience problems and use Wayland."),
                 this);
    uploadWindowEnabledWarning->setWordWrap(true);

    auto* posY = new QHBoxLayout();
    auto* posYLabel = new QLabel(tr("Position Offset Y (px)"), this);
    m_uploadWindowOffsetY = new QSpinBox(this);
    m_uploadWindowOffsetY->setMinimum(-99999);
    m_uploadWindowOffsetY->setMaximum(99999);
    m_uploadWindowOffsetY->setValue(ConfigHandler().uploadWindowOffsetY());
    posY->addWidget(m_uploadWindowOffsetY);
    posY->addWidget(posYLabel);

    auto* posX = new QHBoxLayout();
    auto* posXLabel = new QLabel(tr("Position Offset X (px)"), this);
    m_uploadWindowOffsetX = new QSpinBox(this);
    m_uploadWindowOffsetX->setMinimum(-99999);
    m_uploadWindowOffsetX->setMaximum(99999);
    m_uploadWindowOffsetX->setValue(ConfigHandler().uploadWindowOffsetX());
    posX->addWidget(m_uploadWindowOffsetX);
    posX->addWidget(posXLabel);

    auto* scaleW = new QHBoxLayout();
    auto* scaleWLabel = new QLabel(tr("Window Width (px)"), this);
    m_uploadWindowScaleWidth = new QSpinBox(this);
    m_uploadWindowScaleWidth->setMinimum(0);
    m_uploadWindowScaleWidth->setMaximum(9999);
    m_uploadWindowScaleWidth->setValue(ConfigHandler().uploadWindowScaleWidth());
    scaleW->addWidget(m_uploadWindowScaleWidth);
    scaleW->addWidget(scaleWLabel);

    auto* scaleH = new QHBoxLayout();
    auto* scaleHLabel = new QLabel(tr("Window Height (px)"), this);
    m_uploadWindowScaleHeight = new QSpinBox(this);
    m_uploadWindowScaleHeight->setMinimum(0);
    m_uploadWindowScaleHeight->setMaximum(9999);
    m_uploadWindowScaleHeight->setValue(ConfigHandler().uploadWindowScaleHeight());
    scaleH->addWidget(m_uploadWindowScaleHeight);
    scaleH->addWidget(scaleHLabel);

    auto* imageW = new QHBoxLayout();
    auto* imageWLabel = new QLabel(tr("Preview Image Width (px)"), this);
    m_uploadWindowImageWidth = new QSpinBox(this);
    m_uploadWindowImageWidth->setMinimum(0);
    m_uploadWindowImageWidth->setMaximum(9999);
    m_uploadWindowImageWidth->setValue(ConfigHandler().uploadWindowImageWidth());
    imageW->addWidget(m_uploadWindowImageWidth);
    imageW->addWidget(imageWLabel);


    QString foreground = this->palette().windowText().color().name();
    m_uploadWindowOffsetY->setStyleSheet(
      QStringLiteral("color: %1").arg(foreground));
    m_uploadWindowOffsetX->setStyleSheet(
      QStringLiteral("color: %1").arg(foreground));

    auto* timeout = new QHBoxLayout();
    auto* timeoutLabel = new QLabel(tr("Timeout (ms)"), this);
    m_uploadWindowTimeout = new QSpinBox(this);
    m_uploadWindowTimeout->setMinimum(0);
    m_uploadWindowTimeout->setMaximum(99999999);
    m_uploadWindowTimeout->setValue(ConfigHandler().uploadWindowTimeout());
    timeout->addWidget(m_uploadWindowTimeout);
    timeout->addWidget(timeoutLabel);

    auto* stackPadding = new QHBoxLayout();
    auto* stackPaddingLabel = new QLabel(tr("Window Stack Padding (px)"), this);
    m_uploadWindowStackPadding = new QSpinBox(this);
    m_uploadWindowStackPadding->setMinimum(0);
    m_uploadWindowStackPadding->setMaximum(99999);
    m_uploadWindowStackPadding->setValue(
      ConfigHandler().uploadWindowStackPadding());
    stackPadding->addWidget(m_uploadWindowStackPadding);
    stackPadding->addWidget(stackPaddingLabel);

    auto* displayLayout = new QHBoxLayout();
    auto* displayLabel = new QLabel(tr("Display"), this);
    m_selectDisplay = new QComboBox(this);
    m_selectDisplay->addItem(tr("Primary (Default)"), -1);

    QList<QScreen*> screens = QGuiApplication::screens();

    for (int i = 0; i < screens.size(); ++i) {
        QString screenName = screens[i]->name();
        m_selectDisplay->addItem(screenName, i);
    }

    m_selectDisplay->setCurrentIndex(ConfigHandler().uploadWindowDisplay() + 1);

    displayLayout->addWidget(m_selectDisplay);
    displayLayout->addWidget(displayLabel);

    connect(m_uploadWindowOffsetY,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowOffsetYEdited);

    connect(m_uploadWindowOffsetX,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowOffsetXEdited);

    connect(m_uploadWindowTimeout,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowTimeoutEdited);

    connect(m_uploadWindowStackPadding,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowStackPaddingEdited);

    connect(m_uploadWindowEnabled,
            &QCheckBox::clicked,
            this,
            &GeneralConf::uploadWindowEnabledEdited);

    connect(m_uploadWindowScaleWidth,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowScaleWidthEdited);

    connect(m_uploadWindowScaleHeight,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowScaleHeightEdited);

    connect(m_uploadWindowImageWidth,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowImageWidthEdited);

    connect(m_selectDisplay,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &GeneralConf::uploadWindowDisplayEdited);

    // reset button
    auto* resetButton = new QPushButton(tr("Reset Window Options"), this);
    connect(resetButton, &QPushButton::clicked, this, [this]() {
        m_uploadWindowOffsetY->setValue(100);
        m_uploadWindowOffsetX->setValue(10);
        m_uploadWindowTimeout->setValue(25000);
        m_uploadWindowStackPadding->setValue(25);
        m_uploadWindowScaleWidth->setValue(580);
        m_uploadWindowScaleHeight->setValue(120);
        m_uploadWindowImageWidth->setValue(125);
    });

    auto* testButton = new QPushButton(tr("Test Window"), this);

    connect(testButton, &QPushButton::clicked, this, []() {
        ImgUploaderBase* widget = ImgUploaderManager().uploader(nullptr);

        openWindowCount++;

        QObject::connect(
          widget, &QObject::destroyed, [=]() { openWindowCount--; });

        widget->showPreUploadDialog(openWindowCount);
        widget->showPostUploadDialog(openWindowCount);
    });

    vboxLayout->addWidget(m_uploadWindowEnabled);
    vboxLayout->addWidget(uploadWindowEnabledWarning);
    vboxLayout->addLayout(posY);
    vboxLayout->addLayout(posX);
    vboxLayout->addLayout(scaleW);
    vboxLayout->addLayout(scaleH);
    vboxLayout->addLayout(imageW);
    vboxLayout->addLayout(timeout);
    vboxLayout->addLayout(stackPadding);
    vboxLayout->addLayout(displayLayout);
    vboxLayout->addWidget(resetButton);
    vboxLayout->addWidget(testButton);
}

void GeneralConf::uploadClientKeyEdited()
{
    ConfigHandler().setUploadClientSecret(m_uploadClientKey->text());
}

void GeneralConf::uploadTokenTPUEdited()
{
    ConfigHandler().setUploadTokenTPU(m_uploadToken->text());
}

void GeneralConf::serverTPUEdited()
{
    ConfigHandler().setServerTPU(m_serverTPU->text());
}

void GeneralConf::uploadWindowOffsetYEdited()
{
    ConfigHandler().setUploadWindowOffsetY(
      m_uploadWindowOffsetY->text().toInt());
}

void GeneralConf::uploadWindowOffsetXEdited()
{
    ConfigHandler().setUploadWindowOffsetX(
      m_uploadWindowOffsetX->text().toInt());
}

void GeneralConf::uploadWindowScaleWidthEdited()
{
    ConfigHandler().setUploadWindowScaleWidth(
      m_uploadWindowScaleWidth->text().toDouble());
}

void GeneralConf::uploadWindowScaleHeightEdited()
{
    ConfigHandler().setUploadWindowScaleHeight(
      m_uploadWindowScaleHeight->text().toDouble());
}

void GeneralConf::uploadWindowImageWidthEdited()
{
    ConfigHandler().setUploadWindowImageWidth(
      m_uploadWindowImageWidth->text().toDouble());
}

void GeneralConf::uploadWindowTimeoutEdited()
{
    ConfigHandler().setUploadWindowTimeout(
      m_uploadWindowTimeout->text().toInt());
}

void GeneralConf::uploadWindowStackPaddingEdited()
{
    ConfigHandler().setUploadWindowStackPadding(
      m_uploadWindowStackPadding->text().toInt());
}

void GeneralConf::uploadWindowEnabledEdited(bool checked)
{
    ConfigHandler().setUploadWindowEnabled(checked);
}

void GeneralConf::uploadWindowDisplayEdited()
{
    ConfigHandler().setUploadWindowDisplay(
      m_selectDisplay->currentData().toInt());
}

void GeneralConf::uploadHistoryMaxChanged(int max)
{
    ConfigHandler().setUploadHistoryMax(max);
}

void GeneralConf::initUndoLimit()
{
    auto* layout = new QHBoxLayout();
    auto* layoutLabel = new QLabel(tr("Undo limit"), this);
    m_undoLimit = new QSpinBox(this);
    m_undoLimit->setMinimum(1);
    m_undoLimit->setMaximum(999);
    QString foreground = this->palette().windowText().color().name();
    m_undoLimit->setStyleSheet(QStringLiteral("color: %1").arg(foreground));
    layout->addWidget(m_undoLimit);
    layout->addWidget(layoutLabel);

    connect(m_undoLimit,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::undoLimit);

    m_scrollAreaLayout->addLayout(layout);
}


void GeneralConf::undoLimit(int limit)
{
    ConfigHandler().setUndoLimit(limit);
}

void GeneralConf::initUseJpgForClipboard()
{
    m_useJpgForClipboard =
      new QCheckBox(tr("Use JPG format for clipboard (PNG default)"), this);
    m_useJpgForClipboard->setToolTip(
      tr("Use lossy JPG format for clipboard (lossless PNG default)"));
    m_scrollAreaLayout->addWidget(m_useJpgForClipboard);

#if defined(Q_OS_MACOS)
    // FIXME - temporary fix to disable option for MacOS
    m_useJpgForClipboard->hide();
#endif
    connect(m_useJpgForClipboard,
            &QCheckBox::clicked,
            this,
            &GeneralConf::useJpgForClipboardChanged);
}

void GeneralConf::saveAfterCopyChanged(bool checked)
{
    ConfigHandler().setSaveAfterCopy(checked);
}

void GeneralConf::changeSavePath()
{
    QString path = ConfigHandler().savePath();
    path = chooseFolder(path);
    if (!path.isEmpty()) {
        m_savePath->setText(path);
        ConfigHandler().setSavePath(path);
    }
}

void GeneralConf::initCopyPathAfterSave()
{
    m_copyPathAfterSave = new QCheckBox(tr("Copy file path after save"), this);
    m_copyPathAfterSave->setToolTip(tr("Copy the file path to clipboard after "
                                       "the file is saved"));
    m_scrollAreaLayout->addWidget(m_copyPathAfterSave);
    connect(m_copyPathAfterSave, &QCheckBox::clicked, [](bool checked) {
        ConfigHandler().setCopyPathAfterSave(checked);
    });
}

void GeneralConf::initAntialiasingPinZoom()
{
    m_antialiasingPinZoom =
      new QCheckBox(tr("Anti-aliasing image when zoom the pinned image"), this);
    m_antialiasingPinZoom->setToolTip(
      tr("After zooming the pinned image, should the image get smoothened or "
         "stay pixelated"));
    m_scrollAreaLayout->addWidget(m_antialiasingPinZoom);
    connect(m_antialiasingPinZoom, &QCheckBox::clicked, [](bool checked) {
        ConfigHandler().setAntialiasingPinZoom(checked);
    });
}

void GeneralConf::initUploadWithoutConfirmation()
{
    m_uploadWithoutConfirmation =
      new QCheckBox(tr("Upload image without confirmation"), this);
    m_uploadWithoutConfirmation->setToolTip(
      tr("Upload image without confirmation"));
    m_scrollAreaLayout->addWidget(m_uploadWithoutConfirmation);
    connect(m_uploadWithoutConfirmation, &QCheckBox::clicked, [](bool checked) {
        ConfigHandler().setUploadWithoutConfirmation(checked);
    });
}

const QString GeneralConf::chooseFolder(const QString& pathDefault)
{
    QString path;
    if (pathDefault.isEmpty()) {
        path =
          QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }
    path = QFileDialog::getExistingDirectory(
      this,
      tr("Choose a Folder"),
      path,
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (path.isEmpty()) {
        return path;
    }

    if (!QFileInfo(path).isWritable()) {
        QMessageBox::about(
          this, tr("Error"), tr("Unable to write to directory."));
        return QString();
    }

    return path;
}

void GeneralConf::initShowMagnifier()
{
    m_showMagnifier = new QCheckBox(tr("Show magnifier"), this);
    m_showMagnifier->setToolTip(tr("Enable a magnifier while selecting the "
                                   "screenshot area"));

    m_scrollAreaLayout->addWidget(m_showMagnifier);
    connect(m_showMagnifier, &QCheckBox::clicked, [](bool checked) {
        ConfigHandler().setShowMagnifier(checked);
    });
}

void GeneralConf::initSquareMagnifier()
{
    m_squareMagnifier = new QCheckBox(tr("Square shaped magnifier"), this);
    m_squareMagnifier->setToolTip(tr("Make the magnifier to be square-shaped"));
    m_scrollAreaLayout->addWidget(m_squareMagnifier);
    connect(m_squareMagnifier, &QCheckBox::clicked, [](bool checked) {
        ConfigHandler().setSquareMagnifier(checked);
    });
}

void GeneralConf::initShowSelectionGeometry()
{
    auto* tobox = new QHBoxLayout();

    int timeout =
      ConfigHandler().value("showSelectionGeometryHideTime").toInt();
    m_xywhTimeout = new QSpinBox();
    m_xywhTimeout->setRange(0, INT_MAX);
    m_xywhTimeout->setToolTip(
      tr("Milliseconds before geometry display hides; 0 means do not hide"));
    m_xywhTimeout->setValue(timeout);
    tobox->addWidget(m_xywhTimeout);
    tobox->addWidget(new QLabel(tr("Set geometry display timeout (ms)")));

    m_scrollAreaLayout->addLayout(tobox);
    connect(m_xywhTimeout,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::setSelGeoHideTime);

    auto* box = new QGroupBox(tr("Selection Geometry Display"));
    box->setFlat(true);
    m_scrollAreaLayout->addWidget(box);

    auto* vboxLayout = new QVBoxLayout();
    box->setLayout(vboxLayout);
    auto* selGeoLayout = new QHBoxLayout();
    m_selectGeometryLocation = new QComboBox(this);

    m_selectGeometryLocation->addItem(tr("None"), GeneralConf::xywh_none);
    m_selectGeometryLocation->addItem(tr("Top Left"),
                                      GeneralConf::xywh_top_left);
    m_selectGeometryLocation->addItem(tr("Top Right"),
                                      GeneralConf::xywh_top_right);
    m_selectGeometryLocation->addItem(tr("Bottom Left"),
                                      GeneralConf::xywh_bottom_left);
    m_selectGeometryLocation->addItem(tr("Bottom Right"),
                                      GeneralConf::xywh_bottom_right);
    m_selectGeometryLocation->addItem(tr("Center"), GeneralConf::xywh_center);

    // pick up int from config and use findData
    int pos = ConfigHandler().value("showSelectionGeometry").toInt();
    m_selectGeometryLocation->setCurrentIndex(
      m_selectGeometryLocation->findData(pos));

    connect(
      m_selectGeometryLocation,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this,
      &GeneralConf::setGeometryLocation);

    selGeoLayout->addWidget(m_selectGeometryLocation);
    selGeoLayout->addWidget(new QLabel(tr("Display Location")));
    vboxLayout->addLayout(selGeoLayout);
    vboxLayout->addStretch();
}

void GeneralConf::initJpegQuality()
{
    auto* tobox = new QHBoxLayout();

    int quality = ConfigHandler().value("jpegQuality").toInt();
    m_jpegQuality = new QSpinBox();
    m_jpegQuality->setRange(0, 100);
    m_jpegQuality->setToolTip(tr("Quality range of 0-100; Higher number is "
                                 "better quality and larger file size"));
    m_jpegQuality->setValue(quality);
    tobox->addWidget(m_jpegQuality);
    tobox->addWidget(new QLabel(tr("JPEG Quality")));

    m_scrollAreaLayout->addLayout(tobox);
    connect(m_jpegQuality,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::setJpegQuality);
}

void GeneralConf::setSelGeoHideTime(int v)
{
    ConfigHandler().setValue("showSelectionGeometryHideTime", v);
}

void GeneralConf::setJpegQuality(int v)
{
    ConfigHandler().setJpegQuality(v);
}

void GeneralConf::setGeometryLocation(int index)
{
    ConfigHandler().setValue("showSelectionGeometry",
                             m_selectGeometryLocation->itemData(index));
}

void GeneralConf::togglePathFixed()
{
    ConfigHandler().setSavePathFixed(m_screenshotPathFixedCheck->isChecked());
}

void GeneralConf::setSaveAsFileExtension(const QString& extension)
{
    ConfigHandler().setSaveAsFileExtension(extension);
}

void GeneralConf::useJpgForClipboardChanged(bool checked)
{
    ConfigHandler().setUseJpgForClipboard(checked);
}
