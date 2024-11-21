#include "GameTableView.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QTimer>
#include <QPainter>
#include <QStyledItemDelegate>
#include "QGameInfo.h"

#define ImageSize 128

// カスタムデリゲート（画像列用）の改善
class GameImageDelegate : public QStyledItemDelegate {
public:
  explicit GameImageDelegate(int size = ImageSize, QObject* parent = nullptr)
    : QStyledItemDelegate(parent), IMAGE_SIZE(size) {}

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
    const QModelIndex& index) const override {
    
    if (option.state & QStyle::State_Selected) {
      painter->fillRect(option.rect, option.palette.buttonText());
    }


    QPixmap pixmap = qvariant_cast<QPixmap>(index.data(Qt::DecorationRole));
    if (!pixmap.isNull()) {
      QRect rect = option.rect;

      // 中央に配置するための計算
      int x = rect.x() + (rect.width() - IMAGE_SIZE) / 2;
      int y = rect.y() + (rect.height() - IMAGE_SIZE) / 2;

      painter->drawPixmap(x, y, IMAGE_SIZE, IMAGE_SIZE, pixmap);
    }
    
  }

  QSize sizeHint(const QStyleOptionViewItem& option,
    const QModelIndex& index) const override {
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(IMAGE_SIZE + 10, IMAGE_SIZE + 10); // パディングを追加
  }

private:
  const int IMAGE_SIZE;
};

GameTableView::GameTableView(QWidget *parent) : QTableView(parent) {
    setupModel();
    setupViewOptimizations();
    
    // スクロールタイマーの設定
    scrollTimer = new QTimer(this);
    scrollTimer->setSingleShot(true);
    scrollTimer->setInterval(100);
    connect(scrollTimer, &QTimer::timeout, this, &GameTableView::loadVisibleImages);
    
    // スクロールバーのシグナルに接続
    connect(verticalScrollBar(), &QScrollBar::valueChanged, 
            [this](int) { scrollTimer->start(); });
}

QString GameTableView::getFullPath(const QModelIndex& index) const {

  if (mProxyModel == nullptr || mModel == nullptr ) {
    return QString();
  }

  // プロキシモデルからソースモデルのインデックスを取得
  QModelIndex sourceIndex = mProxyModel->mapToSource(index);

  // 行のインデックスを取得
  int row = sourceIndex.row();

  // ファイル名とパスを取得
  QString fileName = mModel->data(mModel->index(row, 4)).toString();
  // QString filePath = m_model->data(m_model->index(row, 1)).toString();

   // フルパスを構築
  return fileName;
}

void GameTableView::filterFiles(const QString& text, int currentIndex)
{
  if (mProxyModel) {
    mProxyModel->setFilterKeyColumn(currentIndex+1);
    mProxyModel->setFilterFixedString(text);

    // フィルタ適用後に表示範囲の画像をリロード
    loadedIndexes.clear();
    viewport()->update();
    QTimer::singleShot(100, this, &GameTableView::loadVisibleImages);
    verticalScrollBar()->setValue(0);
  }
}

void GameTableView::updateFilterColumn(int column)
{
  if (mProxyModel) {
    mProxyModel->setFilterKeyColumn(column+1);
    // フィルタ適用後に表示範囲の画像をリロード
    loadedIndexes.clear();
    QTimer::singleShot(100, this, &GameTableView::loadVisibleImages);
  }
}


// setupModel関数の列幅設定を調整
void GameTableView::setupModel() {
  mModel = new QStandardItemModel(0, COLUMN_COUNT, this);

  // ヘッダーの設定
  mModel->setHeaderData(COL_IMAGE, Qt::Horizontal, tr("Image"));
  mModel->setHeaderData(COL_TITLE, Qt::Horizontal, tr("Title"));
  mModel->setHeaderData(COL_PRODUCT_NUMBER, Qt::Horizontal, tr("Product No."));
  mModel->setHeaderData(COL_RELEASE_DATE, Qt::Horizontal, tr("Release Date"));
  mModel->setHeaderData(COL_FILEPATH, Qt::Horizontal, tr("FilePath"));

  mProxyModel = new QSortFilterProxyModel(this);
  mProxyModel->setSourceModel(mModel);
  setModel(mProxyModel);

  verticalHeader()->hide();

  setSortingEnabled(true);
  horizontalHeader()->setSortIndicatorShown(true);

  // ソート変更時のシグナルを接続
  connect(horizontalHeader(), &QHeaderView::sortIndicatorChanged,
    this, &GameTableView::handleSortingChanged);

  // 画像列用のデリゲートを設定（サイズを指定）
  //setItemDelegateForColumn(COL_IMAGE, new GameImageDelegate(ImageSize, this));

  // 列幅の設定
  horizontalHeader()->setSectionResizeMode(COL_IMAGE, QHeaderView::Fixed);
  setColumnWidth(COL_IMAGE, ImageSize*1.5);  // 画像サイズ + パディング
  horizontalHeader()->setSectionResizeMode(COL_TITLE, QHeaderView::Stretch);
  //horizontalHeader()->setSectionResizeMode(COL_MAKER, QHeaderView::ResizeToContents);
  //horizontalHeader()->setSectionResizeMode(COL_PRODUCT_NUMBER, QHeaderView::ResizeToContents);
  //horizontalHeader()->setSectionResizeMode(COL_RELEASE_DATE, QHeaderView::ResizeToContents);

  // ヘッダーの設定
  horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
}

void GameTableView::handleSortingChanged() {
  // ソート後に画像のロードをリセット
  loadedIndexes.clear();
  // 表示範囲の画像を再ロード
  QTimer::singleShot(100, this, &GameTableView::loadVisibleImages);
}

void GameTableView::setupViewOptimizations() {

  // スクロールモードの最適化
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  verticalScrollBar()->setSingleStep(20);

  // 行の高さを固定
  verticalHeader()->setDefaultSectionSize(ImageSize+10);
  verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

  // ビューのアップデートモード設定
  setUpdatesEnabled(true);
  //viewport()->setAttribute(Qt::WA_OpaquePaintEvent);

  // ダブルバッファリングを有効化
  viewport()->setAttribute(Qt::WA_NoSystemBackground);

  // 選択モードの最適化
  setSelectionMode(QAbstractItemView::SingleSelection);
  setSelectionBehavior(QAbstractItemView::SelectRows);

  // アイテムのキャッシュポリシーを設定
  if (mModel) {
    int visibleRows = viewport()->height() / verticalHeader()->defaultSectionSize();
    mModel->setItemPrototype(new QStandardItem());
    // 表示行数の2倍をキャッシュ
    verticalScrollBar()->setRange(0, qMax(0, mModel->rowCount() - visibleRows));
  }

}

// addGameInfo関数内のプレースホルダー設定を修正
void GameTableView::addGameInfo(const QGameInfo* info) {
  if (!info) return;

  QList<QStandardItem*> items;

  // 画像列のアイテム
  QStandardItem* imageItem = new QStandardItem;
  if (!info->imageUrl.isEmpty()) {
    imageItem->setData(info->imageUrl, Qt::UserRole);
    imageItem->setData(createPlaceholderImage(ImageSize), Qt::DecorationRole);
  }
  items.append(imageItem);

  // その他の列のアイテム

  QStandardItem* titleItem = new QStandardItem(info->displayName);
  titleItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  items.append(titleItem);
  
  QString formattedDate;
  if (info->releaseDate.mid(0, 4).toInt() < 1994) {
    // DDMMYYYY 形式
    formattedDate = info->releaseDate.mid(4, 4) + "/" +  // 年（YYYY）
      info->releaseDate.mid(2, 2) + "/" +  // 月（MM）
      info->releaseDate.mid(0, 2);         // 日（DD）
  }
  else {
    // YYYYMMDD 形式
    formattedDate = info->releaseDate.mid(0, 4) + "/" +  // 年（YYYY）
      info->releaseDate.mid(4, 2) + "/" +  // 月（MM）
      info->releaseDate.mid(6, 2);         // 日（DD）
  }

  items.append(new QStandardItem(info->productNumber));
  items.append(new QStandardItem(formattedDate));
  items.append(new QStandardItem(info->filePath));

  // すべてのアイテムを中央揃えに設定
  for (int i = 2; i < items.size(); ++i) {
    items[i]->setTextAlignment(Qt::AlignCenter);
  }

  mModel->appendRow(items);

  // 追加した行が表示範囲内なら画像をロード
  int row = mModel->rowCount() - 1;
  QModelIndex index = mModel->index(row, COL_IMAGE);
  if (visualRect(index).intersects(viewport()->rect())) {
    loadGameImage(info->imageUrl, QPersistentModelIndex(index));
  }
}

void GameTableView::clear() {
  mModel->clear();
    loadedIndexes.clear();
    setupModel();  // ヘッダーを再設定
}

QPair<int, int> GameTableView::getVisibleRowRange() const {
    QRect viewportRect = viewport()->rect();
    int rowHeight = verticalHeader()->defaultSectionSize();
    
    // スクロール位置から表示範囲の行を計算
    int firstVisibleRow = verticalScrollBar()->value() / rowHeight;
    int lastVisibleRow = (verticalScrollBar()->value() + viewportRect.height()) / rowHeight;
    
    // バッファを追加
    const int BUFFER_ROWS = 1;
    return qMakePair(
        qMax(0, firstVisibleRow - BUFFER_ROWS),
        qMin(mModel->rowCount() - 1, lastVisibleRow + BUFFER_ROWS)
    );
}

void GameTableView::loadVisibleImages() {

  auto range = getVisibleRowRange();

  for (int row = range.first; row <= range.second; ++row) {
    // プロキシモデルのインデックスを取得
    QModelIndex proxyIndex = mProxyModel->index(row, COL_IMAGE);
    if (!proxyIndex.isValid()) continue;

    QPersistentModelIndex persistentProxyIndex(proxyIndex);

    if (!loadedIndexes.contains(persistentProxyIndex)) {
      // ソースモデルのインデックスを取得してURLを取得
      QModelIndex sourceIndex = mProxyModel->mapToSource(proxyIndex);
      QString url = mModel->data(sourceIndex, Qt::UserRole).toString();
      if (!url.isEmpty()) {
        loadGameImage(url, persistentProxyIndex);
      }
    }
  }
}

void GameTableView::loadGameImage(const QString& url, const QPersistentModelIndex& proxyIndex) {
  if (!proxyIndex.isValid() || loadedIndexes.contains(proxyIndex)) {
    return;
  }

  loadedIndexes.insert(proxyIndex);

  ImageLoaderInstance.loadImage(url, [this, proxyIndex](const QPixmap& originalPixmap) {
    if (proxyIndex.isValid()) {
      // 固定サイズにスケール
      const int IMAGE_SIZE = ImageSize; // デリゲートと同じサイズ
      QPixmap scaledPixmap = originalPixmap.scaled(IMAGE_SIZE, IMAGE_SIZE,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);

      // 正方形のキャンバスを作成
      QPixmap finalPixmap(IMAGE_SIZE, IMAGE_SIZE);
      finalPixmap.fill(Qt::transparent);

      // 中央に配置
      QPainter painter(&finalPixmap);
      int x = (IMAGE_SIZE - scaledPixmap.width()) / 2;
      int y = (IMAGE_SIZE - scaledPixmap.height()) / 2;
      painter.drawPixmap(x, y, scaledPixmap);

      mProxyModel->setData(proxyIndex, finalPixmap, Qt::DecorationRole);
    }
  });
}

// プレースホルダー画像の生成を関数化
QPixmap GameTableView::createPlaceholderImage(int size) const {
  QPixmap placeholder(size, size);
  placeholder.fill(Qt::transparent);

  QPainter painter(&placeholder);
  painter.setPen(QPen(Qt::lightGray, 1));
  painter.drawRect(0, 0, size - 1, size - 1);

  // オプション：プレースホルダーアイコンや「Loading...」テキストを追加
  painter.setPen(Qt::lightGray);
  painter.drawText(placeholder.rect(), Qt::AlignCenter, "...");

  return placeholder;
}


void GameTableView::scrollContentsBy(int dx, int dy) {
    QTableView::scrollContentsBy(dx, dy);
    if (dx != 0 || dy != 0) {
        scrollTimer->start();
    }
}

void GameTableView::resizeEvent(QResizeEvent *event) {
    QTableView::resizeEvent(event);
    loadVisibleImages();
}

void GameTableView::showEvent(QShowEvent *event) {
    QTableView::showEvent(event);
    loadVisibleImages();
}