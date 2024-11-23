#include "filesearchworker.h"
#include <QDirIterator>
#include "QGameInfo.h"

FileSearchWorker::FileSearchWorker(QObject *parent)
    : QObject(parent)
    , m_shouldStop(false)
{
}

void FileSearchWorker::search(const QString &path)
{
    m_shouldStop = false;
    emit searchStarted();
    
    QDirIterator it(path, QDirIterator::Subdirectories);
    int fileCount = 0;

    while (it.hasNext() && !m_shouldStop) {
        QString filePath = it.next();
        QFileInfo fileInfo(filePath);
        
        if (fileInfo.isFile()) {
          try {
            QString extension = fileInfo.suffix().toLower();
            if (extension == "chd" || extension == "cue" || extension == "mds" || extension == "ccd") {
              QGameInfo* info = nullptr;
              if (extension == "chd") {
                info = QGameInfo::fromChdFile(fileInfo.filePath());
              }
              else if (extension == "cue") {
                info = QGameInfo::fromCueFile(fileInfo.filePath());
              }
              else if (extension == "mdf") {
                info = QGameInfo::fromMdsFile(fileInfo.filePath());
              }
              else if (extension == "ccd") {
                info = QGameInfo::fromCcdFile(fileInfo.filePath());
              }
              if (info == nullptr) {
                continue;
              }
              emit fileFound(fileInfo, info);
              fileCount++;
            }
          }
          catch (const GameInfoError& e) {
            qDebug() << e.errorMessage;
          }
        }
    }

    emit searchCompleted(fileCount);
}

void FileSearchWorker::stop()
{
    m_shouldStop = true;
}