#include "filesearchwidget.h"
#include "filesearchworker.h"
#include "qprogressindicator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QDir>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QFileInfo>
#include <QDateTime>
#include <QHeaderView>
#include <QTableView>
#include <QSortFilterProxyModel>
#include <QComboBox>
#include <QSettings>
#include <QMessageBox>
#include <QApplication>

#include "QGameInfo.h"
#include "GametableView.h"

FileSearchWidget::FileSearchWidget(QWidget *parent)
    : QWidget(parent)
    , m_selectDirButton(nullptr)
    , m_pathLabel(nullptr)
    , m_tableView(nullptr)
    , m_model(nullptr)
    , m_searchEdit(nullptr)
    , m_proxyModel(nullptr)
    , m_filterColumnCombo(nullptr)
    , m_progressIndicator(nullptr)
    , m_worker(nullptr)
    , m_isSearching(false)
{
    // ワーカーの初期化
    m_worker = new FileSearchWorker;
    m_worker->moveToThread(&m_workerThread);

    // シグナル/スロット接続
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &FileSearchWidget::destroyed, m_worker, &FileSearchWorker::stop);

    qRegisterMetaType<QGameInfo>("QGameInfo");

    bool isConnected = connect(m_worker, &FileSearchWorker::fileFound, this, &FileSearchWidget::handleFileFound,Qt::QueuedConnection);
    if (!isConnected) {
      qDebug() << "Failed to connect fileFound signal to handleFileFound slot.";
    }
    connect(m_worker, &FileSearchWorker::searchCompleted, this, &FileSearchWidget::handleSearchCompleted);
    connect(m_worker, &FileSearchWorker::searchStarted, this, &FileSearchWidget::handleSearchStarted);

    m_workerThread.start();

    initUI();
    loadSettings();
    
    // 前回のディレクトリが存在する場合は自動的に検索を開始
    QString lastDir = getLastDirectory();
    if (!lastDir.isEmpty() && QDir(lastDir).exists()) {
        m_pathLabel->setText(tr(" %1").arg(lastDir));
        QMetaObject::invokeMethod(m_worker, "search", Qt::QueuedConnection,
                                Q_ARG(QString, lastDir));
    }
}

FileSearchWidget::~FileSearchWidget()
{
    saveSettings();
    m_worker->stop();
    m_workerThread.quit();
    m_workerThread.wait();
}

void FileSearchWidget::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *controlLayout = new QHBoxLayout();
    
    // ディレクトリ選択ボタンとプログレスインジケーター
    QHBoxLayout *topLayout = new QHBoxLayout();
    m_selectDirButton = new QPushButton(tr("Select Directory"), this);
    connect(m_selectDirButton, &QPushButton::clicked, this, &FileSearchWidget::selectDirectory);
    topLayout->addWidget(m_selectDirButton);


    m_progressIndicator = new QProgressIndicator(this);
    m_progressIndicator->setFixedSize(24, 24);
    m_progressIndicator->setColor(QColor(0, 0, 0));

    // パス表示ラベル
    m_pathLabel = new QLabel(tr("None"), this);

    topLayout->addWidget(m_progressIndicator);
    topLayout->addWidget(m_pathLabel);
    topLayout->addStretch();

    mainLayout->addLayout(topLayout);

    // 検索フィルター
    m_filterColumnCombo = new QComboBox(this);
    m_filterColumnCombo->addItems({
        tr("Title"),
        tr("Product No."),
        tr("Release Date"),
        tr("FilePath"),
    });
    controlLayout->addWidget(m_filterColumnCombo);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &FileSearchWidget::filterFiles);
    controlLayout->addWidget(m_searchEdit);

    mainLayout->addLayout(controlLayout);

    m_tableView = new GameTableView(this);

    // ダブルクリックシグナルの接続
    connect(m_tableView, &GameTableView::doubleClicked,
      this, &FileSearchWidget::handleDoubleClick);

    mainLayout->addWidget(m_tableView);
}

void FileSearchWidget::handleDoubleClick(const QModelIndex& index)
{
  if (!index.isValid())
    return;

  QString fullPath = m_tableView->getFullPath(index);
  if (!fullPath.isEmpty()) {
    emit fileSelected(fullPath);
  }
}


void FileSearchWidget::selectDirectory()
{
    if (m_isSearching) {
        m_worker->stop();
        return;
    }

    QString startDir = getLastDirectory();
    if (startDir.isEmpty() || !QDir(startDir).exists()) {
        startDir = QDir::homePath();
    }
    QString directory = QFileDialog::getExistingDirectory(
        this,
        tr("Select DIrectory"),
        startDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!directory.isEmpty()) {
        setLastDirectory(directory);
        m_pathLabel->setText(tr(" %1").arg(directory));
        //m_model->removeRows(0, m_model->rowCount());
        m_tableView->clear();
        QMetaObject::invokeMethod(m_worker, "search", Qt::QueuedConnection,
                                Q_ARG(QString, directory));
    }
}


void FileSearchWidget::handleFileFound(const QFileInfo &fileInfo, const QGameInfo * gameInfo)
{
    QGameInfo agameInfo = *gameInfo;
    agameInfo.filePath = fileInfo.absoluteFilePath();
    m_tableView->addGameInfo(&agameInfo);
    delete gameInfo;
}

void FileSearchWidget::handleSearchStarted()
{
    m_isSearching = true;
    m_progressIndicator->startAnimation();
    m_selectDirButton->setText(tr("Stop searching"));
    m_searchEdit->setEnabled(false);
    m_filterColumnCombo->setEnabled(false);
}

void FileSearchWidget::handleSearchCompleted(int fileCount)
{
    m_isSearching = false;
    m_progressIndicator->stopAnimation();
    m_selectDirButton->setText(tr("Select Directory"));
    m_searchEdit->setEnabled(true);
    m_filterColumnCombo->setEnabled(true);
    m_tableView->sortByColumn(0, Qt::AscendingOrder);
}

void FileSearchWidget::filterFiles(const QString &text)
{
  m_tableView->filterFiles(text, m_filterColumnCombo->currentIndex());
  this->repaint();
}

void FileSearchWidget::updateFilterColumn(int column)
{
  m_tableView->updateFilterColumn(column);
  this->repaint();
}

QString FileSearchWidget::formatFileSize(qint64 bytes) const
{
    constexpr qint64 KB = 1024;
    constexpr qint64 MB = 1024 * KB;
    constexpr qint64 GB = 1024 * MB;

    if (bytes >= GB) {
        return QString::number(static_cast<double>(bytes) / GB, 'f', 2) + " GB";
    } else if (bytes >= MB) {
        return QString::number(static_cast<double>(bytes) / MB, 'f', 2) + " MB";
    } else if (bytes >= KB) {
        return QString::number(static_cast<double>(bytes) / KB, 'f', 2) + " KB";
    }
    return QString::number(bytes) + " B";
}

void FileSearchWidget::loadSettings()
{
    QSettings settings("org.devmiyax", "Yabasanshiro");
    
    if (settings.contains("geometry")) {
        setGeometry(settings.value("geometry").toRect());
    }
}

void FileSearchWidget::saveSettings()
{
  QSettings settings("org.devmiyax", "Yabasanshiro");
    
    settings.setValue("geometry", geometry());
    
    QByteArray widths;
    QDataStream stream(&widths, QIODevice::WriteOnly);
    for (int i = 0; i < m_tableView->model()->columnCount(); ++i) {
        stream << m_tableView->columnWidth(i);
    }
    settings.setValue("columnWidths", widths);
    
    settings.setValue("filterColumn", m_filterColumnCombo->currentIndex());
}

QString FileSearchWidget::getLastDirectory() const
{
  QSettings settings("org.devmiyax", "Yabasanshiro");
    return settings.value("lastDirectory").toString();
}

void FileSearchWidget::setLastDirectory(const QString &path)
{
  QSettings settings("org.devmiyax", "Yabasanshiro");
    settings.setValue("lastDirectory", path);
}    