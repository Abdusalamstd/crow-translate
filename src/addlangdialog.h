#ifndef ADDLANGDIALOG_H
#define ADDLANGDIALOG_H

#include "qonlinetranslator.h"

#include <QDialog>

namespace Ui {
class AddLangDialog;
}

class AddLangDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddLangDialog(QWidget *parent = nullptr);
    ~AddLangDialog();

    QOnlineTranslator::Language language() const;

private slots:
    void on_searchEdit_textChanged(const QString &text);
    void on_dialogButtonBox_accepted();

private:
    Ui::AddLangDialog *ui;

    QOnlineTranslator::Language m_lang;
};

#endif // ADDLANGDIALOG_H
