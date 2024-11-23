#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCache>
#include <QPixmap>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <functional>

class ImageLoader : public QObject {
    Q_OBJECT
    
public:
    static ImageLoader& instance();
    
    // 画像ロードのメインメソッド
    void loadImage(const QString& url, const std::function<void(const QPixmap&)>& callback);
    
    // キャッシュの設定
    void setMemoryCacheSize(int megabytes);
    void setDiskCacheDir(const QString& path);
    void clearCache();
    
signals:
    void loadError(const QString& url, const QString& error);
    
private:
    explicit ImageLoader(QObject* parent = nullptr);
    ~ImageLoader();
    
    // シングルトンのため、コピーを禁止
    ImageLoader(const ImageLoader&) = delete;
    ImageLoader& operator=(const ImageLoader&) = delete;
    
    // ディスクキャッシュのパス生成
    QString getCacheFilePath(const QUrl& url) const;
    
    // ディスクキャッシュの操作
    bool saveToCache(const QUrl& url, const QByteArray& data);
    QPixmap loadFromCache(const QUrl& url);
    
    // メモリキャッシュとネットワークマネージャ
    QCache<QString, QPixmap> memoryCache;
    QNetworkAccessManager networkManager;
    QString diskCachePath;
    
    // 保留中のコールバック
    QMap<QNetworkReply*, std::function<void(const QPixmap&)>> pendingCallbacks;
    
private slots:
    void handleNetworkReply(QNetworkReply* reply);
};

// 便利な使用のためのマクロ
#define ImageLoaderInstance ImageLoader::instance()

#endif // IMAGELOADER_H