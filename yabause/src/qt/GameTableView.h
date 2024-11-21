#ifndef GAMETABLEVIEW_H
#define GAMETABLEVIEW_H

#include <QTableView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QSet>
#include "ImageLoader.h"

class QGameInfo;

class GameTableView : public QTableView {
    Q_OBJECT
public:
    explicit GameTableView(QWidget *parent = nullptr);
    void addGameInfo(const QGameInfo* info);
    void clear();

    // 列の定義
    enum Column {
        COL_IMAGE = 0,
        COL_TITLE,
        COL_PRODUCT_NUMBER,
        COL_RELEASE_DATE,
        COL_FILEPATH,
        COLUMN_COUNT
    };

    QString getFullPath(const QModelIndex& index) const;
    void filterFiles(const QString& text, int currentIndex);
    void updateFilterColumn(int column);

protected:
    void scrollContentsBy(int dx, int dy) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
        
private slots:
    void loadVisibleImages();
    void handleSortingChanged();

private:
    QStandardItemModel* mModel;
    QSortFilterProxyModel* mProxyModel;
    QSet<QPersistentModelIndex> loadedIndexes;
    QTimer* scrollTimer;
    
    void setupModel();
    void loadGameImage(const QString& url, const QPersistentModelIndex& index);
    QPair<int, int> getVisibleRowRange() const;
    void setupViewOptimizations();
    QPixmap createPlaceholderImage(int size) const;
};

#endif // GAMETABLEVIEW_H