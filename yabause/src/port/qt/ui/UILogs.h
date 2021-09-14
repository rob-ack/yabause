#ifndef UILOGS_H
#define UILOGS_H

#include <QDialog>

namespace Ui {
class UILogs;
}

class UILogs : public QDialog
{
    Q_OBJECT

public:
    explicit UILogs(QWidget *parent = nullptr);
    ~UILogs() override;

protected:
    void closeEvent(QCloseEvent *) override;
    void populateErrors();

private:
    Ui::UILogs *ui;
    int errorsDisplayed;
};

#endif // UILOGS_H
