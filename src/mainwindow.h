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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTranslator>
#include <QShortcut>
#include <QTimer>
#include <QButtonGroup>
#include <QMediaPlayer>

#include "qhotkey.h"
#include "qonlinetranslator.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void translationTextChanged(const QString &text);
    void sourceButtonChanged(QAbstractButton *button, int id);
    void translationButtonChanged(QAbstractButton *button, int id);
    void playSourceButtonIconChanged(QIcon icon);
    void stopSourceButtonEnabled(bool enabled);
    void playTranslationButtonIconChanged(QIcon icon);
    void stopTranslationButtonEnabled(bool enabled);

private slots:
    void on_translateButton_clicked();
    void on_swapButton_clicked();
    void on_settingsButton_clicked();

    void on_playSourceButton_clicked();
    void on_playTranslationButton_clicked();

    void on_stopSourceButton_clicked();
    void on_stopTranslationButton_clicked();

    void on_copySourceButton_clicked();
    void on_copyTranslationButton_clicked();
    void on_copyAllTranslationButton_clicked();

    void on_autoSourceButton_triggered(QAction *language);
    void on_autoTranslationButton_triggered(QAction *language);

    void on_autoTranslateCheckBox_toggled(bool checked);

    // Show windows
    void showMainWindow();
    void showPopupWindow();

    // Players
    void playSelection();
    void playTranslatedSelection();
    void changeSourcePlayerIcons(QMediaPlayer::State state);
    void changeTranslationPlayerIcons(QMediaPlayer::State state);

    // Language buttons
    void checkSourceButton(int id, bool checked);
    void checkTranslationButton(int id, bool checked);
    void toggleSourceButton(QAbstractButton *button, bool checked);
    void toggleTranslationButton(QAbstractButton *button, bool checked);
    void resetAutoSourceButtonText();

    // Other
    void copyTranslatedSelection();

private:
    // Settings
    void loadLanguageButtons(QButtonGroup *group);
    void loadSettings();
    void loadLocale();
    void setProxy();

    // Translation
    bool translate(QOnlineTranslator::Language translationLang, QOnlineTranslator::Language sourceLang);
    bool translateOutside(const QString &text, QOnlineTranslator::Language translationLang);

    // Helper functions
    void insertLanguage(QButtonGroup *group, QOnlineTranslator::Language language);
    void play(QMediaPlayer *player, QMediaPlaylist *playlist, const QString &text, QOnlineTranslator::Language lang = QOnlineTranslator::Auto);
    QList<QAction *> languagesList();
    QString selectedText();

    Ui::MainWindow *ui;

    QTranslator *interfaceTranslator;
    QOnlineTranslator *onlineTranslator;
    QOnlineTranslator::Language uiLang;

    QMediaPlayer *sourcePlayer;
    QMediaPlayer *translationPlayer;
    QMediaPlayer *selectionPlayer;
    QMediaPlaylist *sourcePlaylist;
    QMediaPlaylist *translationPlaylist;
    QMediaPlaylist *selectionPlaylist;

    QMenu *trayMenu;
    QSystemTrayIcon *trayIcon;
    QMenu *languagesMenu;
    QTimer *autoTranslateTimer;

    QShortcut *closeWindowsShortcut;
    QHotkey *translateSelectionHotkey;
    QHotkey *playSelectionHotkey;
    QHotkey *playTranslatedSelectionHotkey;
    QHotkey *stopSelectionHotkey;
    QHotkey *showMainWindowHotkey;
    QHotkey *copyTranslatedSelectionHotkey;

    QButtonGroup *sourceButtonGroup;
    QButtonGroup *translationButtonGroup;
};

#endif // MAINWINDOW_H
