#include "UILogs.h"
#include "ui_UILogs.h"
#include "QtYabause.h"
#include "UIYabause.h"

UILogs::UILogs(QWidget *parent) : QDialog(parent), ui(new Ui::UILogs), errorsDisplayed(0)
{
    ui->setupUi(this);
    ui->logsListContainer->clear();

    auto const * const yabauseInstance = QtYabause::mainWindow(false)->GetYabauseThread();
    populateErrors();
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
}

void UILogs::populateErrors()
{
    auto const & stack = QtYabause::getErrorStack();
    if (!(stack.size() > errorsDisplayed))
    {
	    return;
    }

    auto iterator = stack.cbegin();
    std::advance(iterator, errorsDisplayed);
    errorsDisplayed = stack.size();

    auto* const container = ui->logsListContainer;
    for (auto it = iterator; it != stack.end(); ++it)
    {
        auto const& element = *it;
        container->appendPlainText(element.ErrorTimestamp.toString() + ": " + element.ErrorMessage + "\n");
    }
}
