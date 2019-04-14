/*
 *  Copyright © 2018-2019 Hennadii Chernyshchyk <genaloner@gmail.com>
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

#include "langbuttongroup.h"
#include "appsettings.h"

#include <QAbstractButton>

LangButtonGroup::LangButtonGroup(QObject *parent) :
    QButtonGroup(parent)
{
    // Setup "Checked" signal
    connect(this, qOverload<int, bool>(&LangButtonGroup::buttonToggled), [&](int id, bool checked) {
        if (checked)
            emit buttonChecked(id);
    });
}

void LangButtonGroup::addButton(QAbstractButton *button)
{
    const int buttonId = buttons().count();

    QButtonGroup::addButton(button, buttons().count());
    if (buttonId == 0) {
       button->setText(tr("Auto"));
       button->setToolTip(tr("Auto"));
       button->setProperty("Lang", QOnlineTranslator::Auto); // Save language id in property
    }
}

void LangButtonGroup::loadLanguages()
{
    // Load buttons data from settings
    const AppSettings settings;
    for (int i = 1; i < buttons().size(); ++i)
        setLanguage(i, settings.buttonLanguage(m_type, i));

    // Load checked button
    button(settings.checkedButton(m_type))->setChecked(true);
}

void LangButtonGroup::loadLanguages(const LangButtonGroup *group)
{
    // Check group sizes
    if (group->buttons().size() != buttons().size()) {
        qCritical() << tr("Different number of buttons in copied groups");
        return;
    }

    // Copy all languages from buttons
    for (int i = 0; i < buttons().size(); ++i)
        setLanguage(i, group->language(i));

    // Copy checked button
    button(group->checkedId())->setChecked(true);
}

void LangButtonGroup::insertLanguage(QOnlineTranslator::Language lang)
{
    // Exit the function if the current language already has a button
    for (int i = 1; i < buttons().size(); ++i) {
        if (lang == language(i)) {
            button(i)->setChecked(true);
            return;
        }
    }

    // Shift buttons (..., 3 <- 2, 2 <- 1)
    AppSettings settings;
    for (int i = buttons().size() - 1; i > 1; --i) {
        setLanguage(i, language(i - 1));

        // Save button in settings
        settings.setButtonLanguage(m_type, i, language(i));
    }

    // Shift checked button in the settings
    if (checkedId() != 0 && checkedId() != 3)
        settings.setCheckedButton(m_type, settings.checkedButton(m_type) + 1);

    // Insert new language to first button
    setLanguage(1, lang);

    // Save first button settings
    settings.setButtonLanguage(m_type, 1, lang);

    if (button(1)->isChecked())
        emit buttonChecked(1); // Emit signal, because first button has shifted to second
    else
        button(1)->setChecked(true);
}

void LangButtonGroup::retranslate()
{
    for (int i = 0; i < buttons().size(); ++i) {
        const QOnlineTranslator::Language lang = language(i);

        if (lang != QOnlineTranslator::NoLanguage) {
            const QString langName = QOnlineTranslator::languageString(lang);

            button(i)->setToolTip(langName);
            if (i != 0) {
                // Language button
                button(i)->setText(langName);
            } else {
                // Auto language button
                if (lang == QOnlineTranslator::Auto)
                    button(i)->setText(tr("Auto"));
                else
                    button(i)->setText(tr("Auto") + " (" + langName + ")");
            }
        }
    }
}

QOnlineTranslator::Language LangButtonGroup::checkedLanguage() const
{
    return checkedButton()->property("Lang").value<QOnlineTranslator::Language>();
}

QOnlineTranslator::Language LangButtonGroup::language(int id) const
{
    return button(id)->property("Lang").value<QOnlineTranslator::Language>();
}

LangButtonGroup::GroupType LangButtonGroup::type() const
{
    return m_type;
}

void LangButtonGroup::setType(const GroupType &type)
{
    m_type = type;
}

void LangButtonGroup::checkButton(int id)
{
    button(id)->setChecked(true);
}

void LangButtonGroup::setLanguage(int id, QOnlineTranslator::Language lang)
{
    if (lang == language(id))
        return;

    button(id)->setProperty("Lang", lang); // Save language id in property

    if (lang != QOnlineTranslator::NoLanguage) {
        const QString languageName = QOnlineTranslator::languageString(lang);

        if (id != 0) {
            // Language button
            button(id)->setText(languageName);
            button(id)->setIcon(QIcon(":/icons/flags/" + QOnlineTranslator::languageCode(lang) + ".svg"));
        } else {
            // Auto language button
            if (lang == QOnlineTranslator::Auto)
                button(id)->setText(tr("Auto"));
            else
                button(id)->setText(tr("Auto") + " (" + languageName + ")");
        }

        button(id)->setToolTip(languageName);
        button(id)->setVisible(true);
    } else {
        button(id)->setVisible(false);
    }

    emit languageChanged(id, lang);
}
