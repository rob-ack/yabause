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

Q_SIGNALS:
    void closing();

protected:
    void closeEvent(QCloseEvent *) override;
    void populateErrors() const;
private:
    Ui::UILogs *ui;
};

#endif // UILOGS_H
