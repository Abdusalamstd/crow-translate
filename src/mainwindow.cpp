/*
 *  Copyright © 2018 Gennady Chernyshchuk <genaloner@gmail.com>
 *
 *  This file is part of Crow Translate.
 *
 *  Crow Translate is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a get of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "mainwindow.h"

#include <QClipboard>

#if defined(Q_OS_WIN)
#include <QMimeData>
#include <QTimer>
#include <windows.h>
#endif

#include "ui_mainwindow.h"
#include "popupwindow.h"
#include "settingsdialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui (new Ui::MainWindow),
    languagesMenu (new QMenu),
    trayMenu (new QMenu(this)),
    trayIcon (new QSystemTrayIcon(this)),
    closeWindowsShortcut (new QShortcut(this)),
    translateSelectedHotkey (new QHotkey(this)),
    saySelectedHotkey (new QHotkey(this)),
    showMainWindowHotkey (new QHotkey(this)),
    sourceGroup (new QButtonGroup(this)),
    translationGroup (new QButtonGroup(this))
{
    // Set object names for signals autoconnection
    trayIcon->setObjectName("tray");
    translateSelectedHotkey->setObjectName("translateSelectedHotkey");
    saySelectedHotkey->setObjectName("saySelectedHotkey");
    showMainWindowHotkey->setObjectName("showMainWindowHotkey");
    sourceGroup->setObjectName("sourceGroup");
    translationGroup->setObjectName("translationGroup");

    // Load translation
    QSettings settings;
    QString localeCode = settings.value("Language", "auto").toString();
    if (localeCode == "auto") {
        QLocale::setDefault(QLocale::system());
        localeCode = QLocale().name().left(QLocale().name().indexOf("_"));
    }
    else
        QLocale::setDefault(QLocale(localeCode));

    if (translator.load(QLocale(), QString("crow"), QString("_"), QString(":/translations"))) {
        qApp->installTranslator(&translator);
    }

    ui->setupUi(this);

    // Setup the auto-detect language button for translation
    ui->translationAutoButton->setToolTip(localeCode);
    ui->translationAutoButton->setText(tr("Auto") + " (" + QCoreApplication::translate("QOnlineTranslator", qPrintable(QOnlineTranslator::codeToLanguage(localeCode))) + ")");

    autoTranslateTimer.setSingleShot(true);

    // Add languageMenu to auto-language buttons
    languagesMenu->addActions(languagesList());
    ui->sourceAutoButton->setMenu(languagesMenu);
    ui->translationAutoButton->setMenu(languagesMenu);

    // Add all language buttons to button groups
    sourceGroup->addButton(ui->sourceAutoButton, 0);
    sourceGroup->addButton(ui->sourceFirstButton, 1);
    sourceGroup->addButton(ui->sourceSecondButton, 2);
    sourceGroup->addButton(ui->sourceThirdButton, 3);
    translationGroup->addButton(ui->translationAutoButton, 0);
    translationGroup->addButton(ui->translationFirstButton, 1);
    translationGroup->addButton(ui->translationSecondButton, 2);
    translationGroup->addButton(ui->translationThirdButton, 3);

    // Create context menu for tray
#if defined(Q_OS_LINUX)
    trayMenu->addAction(QIcon::fromTheme("window"), tr("Show window"), this, &MainWindow::show);
    trayMenu->addAction(QIcon::fromTheme("settings"), tr("Settings"), this, &MainWindow::on_settingsButton_clicked);
    trayMenu->addAction(QIcon::fromTheme("exit"), tr("Exit"), qApp, &QApplication::quit);
#elif defined(Q_OS_WIN)
    trayMenu->addAction(tr("Show window"), this, &MainWindow::show);
    trayMenu->addAction(tr("Settings"), this, &MainWindow::on_settingsButton_clicked);
    trayMenu->addAction(tr("Exit"), qApp, &QApplication::quit);
#endif
    trayIcon->setContextMenu(trayMenu);

    loadLanguageButtons(sourceGroup, "Source");
    loadLanguageButtons(translationGroup, "Translation");
    loadSettings();

    connect(closeWindowsShortcut, &QShortcut::activated, this, &MainWindow::close);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_translateButton_clicked()
{
    if (ui->sourceEdit->toPlainText() != "") {
        QSettings settings;
        QString sourcelanguage = sourceGroup->checkedButton()->toolTip();
        QString translationlanguage = translationGroup->checkedButton()->toolTip();
        QString translatorlanguage = settings.value("Language", "auto").toString();
        m_translationData.translate(ui->sourceEdit->toPlainText(), translationlanguage, sourcelanguage, translatorlanguage);

        // Show translation and transcription
        ui->translationEdit->setHtml(m_translationData.text());
        if (m_translationData.translationTranscription() != "")
            ui->translationEdit->append("<font color=\"grey\"><i>/" + m_translationData.translationTranscription() + "/</i></font>");
        if (m_translationData.sourceTranscription() != "")
            ui->translationEdit->append("<font color=\"grey\"><i><b>(" + m_translationData.sourceTranscription() + ")</b></i></font>");
        ui->translationEdit->append("");

        // Show translation options
        foreach (auto translationOptions, m_translationData.options()) {
            ui->translationEdit->append("<i>" + translationOptions.first + "</i>");
            foreach (QString wordsList, translationOptions.second) {
                wordsList.prepend("&nbsp;&nbsp;<b>");
                wordsList.insert(wordsList.indexOf(":") + 1, "</b>");
                ui->translationEdit->append(wordsList);
            }
            ui->translationEdit->append("");
        }

        ui->translationEdit->moveCursor(QTextCursor::Start);
        emit translationTextChanged(ui->translationEdit->toHtml());

        // Display the detected language on "Auto" button.
        if (ui->sourceAutoButton->isChecked() && m_translationData.sourceLanguage() != ui->sourceAutoButton->toolTip()) {
            ui->sourceAutoButton->setText(tr("Auto") + " (" + QCoreApplication::translate("QOnlineTranslator", qPrintable(QOnlineTranslator::codeToLanguage(m_translationData.sourceLanguage()))) + ")");
            ui->sourceAutoButton->setToolTip(m_translationData.sourceLanguage());
            connect(ui->sourceEdit, &QPlainTextEdit::textChanged, this, &MainWindow::resetAutoSourceButtonText);
            emit sourceButtonChanged(ui->sourceAutoButton, 0);
        }
    }
    else
        // Check if function called by pressing the button
        if (QObject::sender() == ui->translateButton)
            qDebug() << tr("Text field is empty");
}

void MainWindow::on_sourceAutoButton_triggered(QAction *language)
{
    insertLanguage(sourceGroup, "Source", language->toolTip());
}

void MainWindow::on_translationAutoButton_triggered(QAction *language)
{
    insertLanguage(translationGroup, "Translation", language->toolTip());
}

void MainWindow::on_sourceGroup_buttonToggled(QAbstractButton *button, const bool &checked)
{
    if (checked) {
        QSettings settings;

        // Check if the target language and source language are the same
        if (button->toolTip() == translationGroup->checkedButton()->toolTip()) {
            // Select the previous source language or system language as the target language and save it
            QString previousLanguage = sourceGroup->button(settings.value("Buttons/SourceCheckedButton", 0).toInt())->toolTip();
            if (previousLanguage != "auto") {
                insertLanguage(translationGroup, "Translation", previousLanguage);
                settings.setValue("Buttons/TranslationCheckedButton", translationGroup->checkedId());
            }
            else
                translationGroup->buttons().at(0)->setChecked(true);
        }

        settings.setValue("Buttons/SourceCheckedButton", sourceGroup->checkedId()); // Save the pressed source language button
    }
}

void MainWindow::on_translationGroup_buttonToggled(QAbstractButton *button, const bool &checked)
{
    if (checked) {
        QSettings settings;

        // Check if the target language and source language are the same
        if (button->toolTip() == sourceGroup->checkedButton()->toolTip()) {
            // Select the previous target language as the source language and save it
            QString previousLanguage = translationGroup->button(settings.value("Buttons/TranslationCheckedButton", 0).toInt())->toolTip();
            insertLanguage(sourceGroup, "Source", previousLanguage);
            settings.setValue("Buttons/SourceCheckedButton", sourceGroup->checkedId());
        }

        settings.setValue("Buttons/TranslationCheckedButton", translationGroup->checkedId());
    }
}

void MainWindow::on_swapButton_clicked()
{
    swapCheckedLanguages();
    on_translateButton_clicked();
}

void MainWindow::on_settingsButton_clicked()
{
    SettingsDialog config(this);
    connect(&config, &SettingsDialog::languageChanged, this, &MainWindow::reloadTranslation);
    if (config.exec()) {
        config.done(0);
        loadSettings();
    }
}

void MainWindow::on_sourceSayButton_clicked()
{
    if (ui->sourceEdit->toPlainText() != "")
        QOnlineTranslator::say(ui->sourceEdit->toPlainText(), sourceGroup->checkedButton()->toolTip());
    else
        qDebug() << tr("Text field is empty");
}

void MainWindow::on_translationSayButton_clicked()
{
    if (ui->translationEdit->toPlainText() != "")
        QOnlineTranslator::say(m_translationData.text(), translationGroup->checkedButton()->toolTip());
    else
        qDebug() << tr("Text field is empty");
}

void MainWindow::on_sourceCopyButton_clicked()
{
    if (ui->sourceEdit->toPlainText() != "")
        QApplication::clipboard()->setText(ui->sourceEdit->toPlainText());
    else
        qDebug() << tr("Text field is empty");
}

void MainWindow::on_translationCopyButton_clicked()
{
    if (ui->translationEdit->toPlainText() != "")
        QApplication::clipboard()->setText(ui->translationEdit->toPlainText());
    else
        qDebug() << tr("Text field is empty");
}

void MainWindow::on_tray_activated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        if (!this->isVisible())
            on_showMainWindowHotkey_activated();
        else
            this->hide();
    }
}

void MainWindow::on_translateSelectedHotkey_activated()
{
    // Send selected text to source field and translate it
    ui->sourceEdit->setPlainText(selectedText());
    on_translateButton_clicked();

    QSettings settings;
    if (this->isHidden() && settings.value("WindowMode", 0).toInt() == 0) {
        // Show popup
        PopupWindow *popup = new PopupWindow(languagesMenu, sourceGroup, translationGroup, this);
        connect(this, &MainWindow::translationTextChanged, popup, &PopupWindow::setTranslation);

        connect(this, &MainWindow::sourceButtonChanged, popup, &PopupWindow::copySourceButton);
        connect(this, &MainWindow::translationButtonChanged, popup, &PopupWindow::copyTranslationButton);

        connect(sourceGroup, qOverload<int, bool>(&QButtonGroup::buttonToggled), popup, &PopupWindow::setSourceButtonChecked);
        connect(translationGroup, qOverload<int, bool>(&QButtonGroup::buttonToggled), popup, &PopupWindow::setTranslationButtonChecked);

        connect(popup, &PopupWindow::sourceButtonClicked, this, &MainWindow::setSourceButtonChecked);
        connect(popup, &PopupWindow::sourceButtonClicked, this, &MainWindow::on_translateButton_clicked);
        connect(popup, &PopupWindow::translationButtonClicked, this, &MainWindow::setTranslationButtonChecked);
        connect(popup, &PopupWindow::translationButtonClicked, this, &MainWindow::on_translateButton_clicked);

        connect(popup, &PopupWindow::sourceLanguageInserted, this, &MainWindow::on_sourceAutoButton_triggered);
        connect(popup, &PopupWindow::sourceLanguageInserted, this, &MainWindow::on_translateButton_clicked);
        connect(popup, &PopupWindow::translationLanguageInserted, this, &MainWindow::on_translationAutoButton_triggered);
        connect(popup, &PopupWindow::translationLanguageInserted, this, &MainWindow::on_translateButton_clicked);

        connect(popup, &PopupWindow::swapButtonClicked, this, &MainWindow::on_swapButton_clicked);
        connect(popup, &PopupWindow::sayButtonClicked, this, &MainWindow::on_translationSayButton_clicked);

        popup->show();
    }
    else
        // Show main window
        on_showMainWindowHotkey_activated();
}

void MainWindow::on_saySelectedHotkey_activated()
{
    QOnlineTranslator::say(selectedText());
}

void MainWindow::on_showMainWindowHotkey_activated()
{
    this->showNormal();
    this->activateWindow();
}

void MainWindow::on_autoTranslateCheckBox_toggled(const bool &state)
{
    // Add a delay before translating when changing the text
    if (state) {
        on_translateButton_clicked();
        connect(ui->sourceEdit, &QPlainTextEdit::textChanged, this, &MainWindow::startTimer);
        connect(sourceGroup, qOverload<int>(&QButtonGroup::buttonClicked), this, &MainWindow::startTimer);
        connect(translationGroup, qOverload<int>(&QButtonGroup::buttonClicked), this, &MainWindow::startTimer);
        connect(&autoTranslateTimer, &QTimer::timeout, this, &MainWindow::on_translateButton_clicked);
    }
    else
        autoTranslateTimer.disconnect();

    QSettings settings;
    settings.setValue("AutoTranslate", state);
}

void MainWindow::reloadTranslation()
{
    // Install translation
    QSettings settings;
    QString localeCode = settings.value("Language", "auto").toString();
    if (localeCode == "auto") {
        QLocale::setDefault(QLocale::system());
        localeCode = QLocale().name().left(QLocale().name().indexOf("_"));
    }
    else
        QLocale::setDefault(QLocale(localeCode));
    if (translator.load(QLocale(), QString("crow"), QString("_"), QString(":/translations")))
        qApp->installTranslator(&translator);

    // Reload UI
    ui->retranslateUi(this);
    trayMenu->actions().at(0)->setText(tr("Show window"));
    trayMenu->actions().at(1)->setText(tr("Settings"));
    trayMenu->actions().at(2)->setText(tr("Exit"));
    languagesMenu->clear();
    languagesMenu->addActions(languagesList());
    loadLanguageButtons(sourceGroup, "Source");
    loadLanguageButtons(translationGroup, "Translation");
    ui->translationAutoButton->setToolTip(localeCode);
    ui->translationAutoButton->setText(tr("Auto") + " (" + QCoreApplication::translate("QOnlineTranslator", qPrintable(QOnlineTranslator::codeToLanguage(localeCode))) + ")");
}

void MainWindow::resetAutoSourceButtonText()
{
    ui->sourceAutoButton->setText(tr("Auto"));
    ui->sourceAutoButton->setToolTip("auto");
    disconnect(ui->sourceEdit, &QPlainTextEdit::textChanged, this, &MainWindow::resetAutoSourceButtonText);
    emit sourceButtonChanged(ui->sourceAutoButton, 0);
}

QList<QAction *> MainWindow::languagesList()
{
    // Load all languages and codes from QOnlineTranslator
    QList<QAction *> languagesList;
    for (auto i=0; i<QOnlineTranslator::LANGUAGE_NAMES.size(); i++) {
        QAction *action = new QAction(QCoreApplication::translate("QOnlineTranslator", qPrintable(QOnlineTranslator::LANGUAGE_NAMES.at(i))));
        action->setToolTip(QOnlineTranslator::LANGUAGE_SHORT_CODES.at(i));
        languagesList.append(action);
    }

    // Sort alphabetically for easy access
    std::sort(languagesList.begin() + 1, languagesList.end(), [](QAction *first, QAction *second) {
        return first->text() < second->text();
    } );

    return languagesList;
}

void MainWindow::startTimer()
{
    autoTranslateTimer.start(500);
}

void MainWindow::loadSettings()
{
    QSettings settings;

    // Load main window settings
    ui->autoTranslateCheckBox->setChecked(settings.value("AutoTranslate", true).toBool());
    on_autoTranslateCheckBox_toggled(ui->autoTranslateCheckBox->isChecked());

    // Load icons
    this->setWindowIcon(QIcon(SettingsDialog::ICONS.at(settings.value("AppIcon", 0).toInt())));
    trayIcon->setIcon(QIcon(SettingsDialog::ICONS.at(settings.value("TrayIcon", 0).toInt())));

    // Load tray visibility
    trayIcon->setVisible(settings.value("TrayIconVisible", true).toBool());
    QApplication::setQuitOnLastWindowClosed(!settings.value("TrayIconVisible", true).toBool());

    // Load shortcuts
    closeWindowsShortcut->setKey(QKeySequence(settings.value("Hotkeys/CloseWindow", "Ctrl+Q").toString()));
    translateSelectedHotkey->setShortcut(QKeySequence(settings.value("Hotkeys/TranslateSelected", "Ctrl+Alt+E").toString()), true);
    saySelectedHotkey->setShortcut(QKeySequence(settings.value("Hotkeys/SaySelected", "Ctrl+Alt+S").toString()), true);
    showMainWindowHotkey->setShortcut(QKeySequence(settings.value("Hotkeys/ShowMainWindow", "Ctrl+Alt+C").toString()), true);
    ui->translateButton->setShortcut(settings.value("Hotkeys/Translate", "Ctrl+Return").toString());
    ui->sourceSayButton->setShortcut(settings.value("Hotkeys/SaySource", "Ctrl+S").toString());
    ui->translationSayButton->setShortcut(settings.value("Hotkeys/SayTranslation", "Ctrl+Shift+S").toString());
}

void MainWindow::loadLanguageButtons(QButtonGroup *group, const QString &settingsName)
{
    // Load buttons text and tooltip
    QSettings settings;
    for (auto i=1; i < 4; i++) {
        QString languageCode = settings.value("Buttons/" + settingsName + "Button" + QString::number(i), "").toString();

        // Check if the code is set
        if (languageCode != "") {
            group->buttons().at(i)->setToolTip(languageCode);
            group->buttons().at(i)->setText(QCoreApplication::translate("QOnlineTranslator", qPrintable(QOnlineTranslator::codeToLanguage(languageCode))));
            group->buttons().at(i)->setVisible(true);
        }
        else
            group->buttons().at(i)->setVisible(false);

        // Load checked button
        group->button(settings.value("Buttons/" + settingsName + "CheckedButton", 0).toInt())->setChecked(true);
    }
}

void MainWindow::insertLanguage(QButtonGroup *group, const QString &settingsName, const QString &languageCode)
{
    // Exit the function if the current language already has a button
    for (auto i = 0; i < 4; i++) {
        if (languageCode == group->buttons().at(i)->toolTip()) {
            group->buttons().at(i)->setChecked(true);
            return;
        }
    }

    // Shift buttons (2 -> 3, 1 -> 2)
    QSettings settings;
    for (auto i = 3; i > 1; i--) {
        // Skip iteration, if previous button is not specified
        if (group->buttons().at(i-1)->toolTip() == "")
            continue;

        // Set values
        group->buttons().at(i)->setText(group->buttons().at(i-1)->text());
        group->buttons().at(i)->setToolTip(group->buttons().at(i-1)->toolTip());
        group->buttons().at(i)->setVisible(true);

        // Send signal
        if (group == sourceGroup)
            emit sourceButtonChanged(group->buttons().at(i), i);
        else
            emit translationButtonChanged(group->buttons().at(i), i);

        // Save settings
        settings.setValue("Buttons/" + settingsName + "Button" + QString::number(i), group->buttons().at(i)->toolTip());
    }

    // Insert new language to first button
    group->buttons().at(1)->setText(QCoreApplication::translate("QOnlineTranslator", qPrintable(QOnlineTranslator::codeToLanguage(languageCode))));
    group->buttons().at(1)->setToolTip(languageCode);
    group->buttons().at(1)->setVisible(true);
    group->buttons().at(1)->setChecked(true);

    // Send signal
    if (group == sourceGroup)
        emit sourceButtonChanged(group->buttons().at(1), 1);
    else
        emit translationButtonChanged(group->buttons().at(1), 1);

    // Save first button settings
    settings.setValue("Buttons/" + settingsName + "Button" + QString::number(1), group->buttons().at(1)->toolTip());
}

void MainWindow::swapCheckedLanguages()
{
    QString sourceLanguage = sourceGroup->checkedButton()->toolTip();
    QString translationLanguage = translationGroup->checkedButton()->toolTip();

    insertLanguage(sourceGroup, "Source", translationLanguage);

    if (sourceLanguage == "auto")
        translationGroup->buttons().at(0)->setChecked(true);
    else
        insertLanguage(translationGroup, "Translation", sourceLanguage);
}

void MainWindow::setSourceButtonChecked(const int &id)
{
    sourceGroup->button(id)->setChecked(true);
}

void MainWindow::setTranslationButtonChecked(const int &id)
{
    translationGroup->button(id)->setChecked(true);
}

QString MainWindow::selectedText()
{
    QString selectedText;
#if defined(Q_OS_LINUX)
    selectedText = QApplication::clipboard()->text(QClipboard::Selection);
#elif defined(Q_OS_WIN) // Send Ctrl + C to get selected text
    // Save original clipboard data
    QVariant originalClipboard;
    if (QApplication::clipboard()->mimeData()->hasImage())
        originalClipboard = QApplication::clipboard()->image();
    else
        originalClipboard = QApplication::clipboard()->text();

    // Wait until the hot key is pressed
    while (GetAsyncKeyState(translateSelectedHotkey->currentNativeShortcut().key) || GetAsyncKeyState(VK_CONTROL)
           || GetAsyncKeyState(VK_MENU) || GetAsyncKeyState(VK_SHIFT))
        Sleep(50);

    // Generate key sequence
    INPUT copyText[4];

    // Set the press of the "Ctrl" key
    copyText[0].ki.wVk = VK_CONTROL;
    copyText[0].ki.dwFlags = 0; // 0 for key press
    copyText[0].type = INPUT_KEYBOARD;

    // Set the press of the "C" key
    copyText[1].ki.wVk = 'C';
    copyText[1].ki.dwFlags = 0;
    copyText[1].type = INPUT_KEYBOARD;

    // Set the release of the "C" key
    copyText[2].ki.wVk = 'C';
    copyText[2].ki.dwFlags = KEYEVENTF_KEYUP;
    copyText[2].type = INPUT_KEYBOARD;

    // Set the release of the "Ctrl" key
    copyText[3].ki.wVk = VK_CONTROL;
    copyText[3].ki.dwFlags = KEYEVENTF_KEYUP;
    copyText[3].type = INPUT_KEYBOARD;

    // Send key sequence to system
    SendInput(4, copyText, sizeof(INPUT));

    // Wait for the clipboard to change
    QEventLoop loop;
    QTimer timer; // Add a timer for the case where the text is not selected
    loop.connect(QApplication::clipboard(), &QClipboard::changed, &loop, &QEventLoop::quit);
    loop.connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(1000);
    loop.exec();

    // Translate the text from the clipboard if the selected text was not copied
    if (timer.isActive())
        return QApplication::clipboard()->text();
    else
        timer.stop();

    // Get clipboard data
    selectedText = QApplication::clipboard()->text();

    // Return original clipboard
    if (originalClipboard.type() == QVariant::Image)
        QApplication::clipboard()->setImage(originalClipboard.value<QImage>());
    else
        QApplication::clipboard()->setText(originalClipboard.toString());
#endif
    return selectedText;
}
