#ifndef FILESEARCHWIDGET_H
#define FILESEARCHWIDGET_H

#include <QWidget>
#include <QThread>
#include <QFileInfo>  // 追加: QFileInfoのインクルード

QT_BEGIN_NAMESPACE
class QPushButton;
class QTableView;
class QLabel;
class QStandardItemModel;
class QLineEdit;
class QSortFilterProxyModel;
class QComboBox;
class QProgressIndicator;
QT_END_NAMESPACE

class FileSearchWorker;
class QGameInfo; 
class GameTableView;

class FileSearchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileSearchWidget(QWidget *parent = nullptr);
    ~FileSearchWidget();

signals:
  void fileSelected(const QString& filePath);

private slots:
    void selectDirectory();
    void filterFiles(const QString &text);
    void updateFilterColumn(int column);
    void handleFileFound(const QFileInfo &fileInfo, const QGameInfo * gameInfo);
    void handleSearchCompleted(int fileCount);
    void handleSearchStarted();
    void handleDoubleClick(const QModelIndex& index);

private:
    void initUI();
    void loadSettings();
    void saveSettings();
    QString getLastDirectory() const;
    void setLastDirectory(const QString &path);
    QString formatFileSize(qint64 bytes) const;
    QString getFullPath(const QModelIndex& index) const;
    
    QPushButton *m_selectDirButton;
    QLabel *m_pathLabel;
    GameTableView*m_tableView;
    QStandardItemModel *m_model;
    QLineEdit *m_searchEdit;
    QSortFilterProxyModel *m_proxyModel;
    QComboBox *m_filterColumnCombo;
    QProgressIndicator *m_progressIndicator;
    
    QThread m_workerThread;
    FileSearchWorker *m_worker;
    bool m_isSearching;
};

#endif // FILESEARCHWIDGET_H