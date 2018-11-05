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

#ifndef POPUPWINDOW_H
#define POPUPWINDOW_H

#include <QMenu>
#include <QToolButton>
#include <QTextEdit>
#include <QShortcut>

#include "qonlinetranslator.h"
#include "langbuttongroup.h"

namespace Ui {
class PopupWindow;
}

class PopupWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PopupWindow(QMenu *languagesMenu, LangButtonGroup *sourceGroup, LangButtonGroup *translationGroup, QWidget *parent = nullptr);
    ~PopupWindow() override;

    QTextEdit *translationEdit();
    QToolButton *swapButton();

    QToolButton *autoSourceButton();
    QToolButton *playSourceButton();
    QToolButton *stopSourceButton();
    QToolButton *copySourceButton();

    QToolButton *autoTranslationButton();
    QToolButton *playTranslationButton();
    QToolButton *stopTranslationButton();
    QToolButton *copyTranslationButton();
    QToolButton *copyAllTranslationButton();

    LangButtonGroup *sourceButtons();
    LangButtonGroup *translationButtons();

public slots:
    void loadButton(LangButtonGroup *group, int id);

private:
    void showEvent(QShowEvent *event) override;
    void copyButton(LangButtonGroup *group, QAbstractButton *button, int id);
    void copyLanguageButtons(LangButtonGroup *existingGroup, LangButtonGroup *copyingGroup);

    Ui::PopupWindow *ui;
    QShortcut *closeWindowsShortcut;
    LangButtonGroup *sourceButtonGroup;
    LangButtonGroup *translationButtonGroup;
};

#endif // POPUPWINDOW_H
