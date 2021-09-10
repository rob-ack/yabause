#include "UILogs.h"
#include "ui_UILogs.h"
#include "QtYabause.h"
#include "UIYabause.h"

UILogs::UILogs(QWidget *parent) : QDialog(parent), ui(new Ui::UILogs)
{
    ui->setupUi(this);

    auto const * const yabauseInstance = QtYabause::mainWindow(false)->GetYabauseThread();
    connect(yabauseInstance, &YabauseThread::error, this, &UILogs::populateErrors);
}

UILogs::~UILogs()
{
    delete ui;
}

void UILogs::closeEvent(QCloseEvent * closeEvent)
{
    auto const* const yabauseInstance = QtYabause::mainWindow(false)->GetYabauseThread();
    disconnect(yabauseInstance, &YabauseThread::error, this, &UILogs::populateErrors);
    QDialog::closeEvent(closeEvent);
    emit closing();
}

void UILogs::populateErrors() const
{
    QListWidget* const container = ui->logsListContainer;
    container->clear();

    auto const& stack = QtYabause::getErrorStack();
    for (auto const& element : stack)
    {
        container->addItem(element.ErrorMessage);
    }
}
