#include "ImageLoader.h"
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>

ImageLoader::ImageLoader(QObject* parent) : QObject(parent) {
    // デフォルトのメモリキャッシュサイズを50MBに設定
    memoryCache.setMaxCost(50 * 1024 * 1024);
    
    // デフォルトのディスクキャッシュディレクトリを設定
    diskCachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/imageCache";
    QDir().mkpath(diskCachePath);
    
    // ネットワークリプライのハンドリング
    connect(&networkManager, &QNetworkAccessManager::finished,
            this, &ImageLoader::handleNetworkReply);
}

ImageLoader::~ImageLoader() {
    // 保留中のリクエストをクリーンアップ
    for (auto reply : pendingCallbacks.keys()) {
        reply->abort();
        reply->deleteLater();
    }
}

ImageLoader& ImageLoader::instance() {
    static ImageLoader instance;
    return instance;
}

void ImageLoader::setMemoryCacheSize(int megabytes) {
    memoryCache.setMaxCost(megabytes * 1024 * 1024);
}

void ImageLoader::setDiskCacheDir(const QString& path) {
    diskCachePath = path;
    QDir().mkpath(diskCachePath);
}

void ImageLoader::clearCache() {
    // メモリキャッシュをクリア
    memoryCache.clear();
    
    // ディスクキャッシュをクリア
    QDir cacheDir(diskCachePath);
    cacheDir.removeRecursively();
    cacheDir.mkpath(".");
}

QString ImageLoader::getCacheFilePath(const QUrl& url) const {
    // URLからハッシュを生成
    QByteArray urlData = url.toString().toUtf8();
    QString fileName = QCryptographicHash::hash(urlData, QCryptographicHash::Sha1).toHex();
    return diskCachePath + "/" + fileName;
}

bool ImageLoader::saveToCache(const QUrl& url, const QByteArray& data) {
    QString filePath = getCacheFilePath(url);
    QFile file(filePath);
    
    if (file.open(QIODevice::WriteOnly)) {
        qint64 written = file.write(data);
        file.close();
        return written == data.size();
    }
    return false;
}

QPixmap ImageLoader::loadFromCache(const QUrl& url) {
    QString filePath = getCacheFilePath(url);
    QPixmap pixmap;
    
    // ファイルが存在し、7日以内に作成されたかチェック
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && fileInfo.lastModified().daysTo(QDateTime::currentDateTime()) <= 31) {
        pixmap.load(filePath);
    }
    
    return pixmap;
}

void ImageLoader::loadImage(const QString& url, const std::function<void(const QPixmap&)>& callback) {
    QUrl imageUrl(url);
    QString cacheKey = imageUrl.toString();
    
    // メモリキャッシュをチェック
    if (QPixmap* cached = memoryCache.object(cacheKey)) {
        callback(*cached);
        return;
    }
    
    // ディスクキャッシュをチェック
    QPixmap diskCached = loadFromCache(imageUrl);
    if (!diskCached.isNull()) {
        memoryCache.insert(cacheKey, new QPixmap(diskCached));
        callback(diskCached);
        return;
    }
    
    // ネットワークリクエストを作成
    QNetworkRequest request(imageUrl);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    
    QNetworkReply* reply = networkManager.get(request);
    pendingCallbacks.insert(reply, callback);
}

void ImageLoader::handleNetworkReply(QNetworkReply* reply) {
    reply->deleteLater();
    
    auto callback = pendingCallbacks.take(reply);
    if (!callback) return;
    
    if (reply->error() != QNetworkReply::NoError) {
        emit loadError(reply->url().toString(), reply->errorString());
        return;
    }
    
    QByteArray imageData = reply->readAll();
    QPixmap pixmap;
    
    if (pixmap.loadFromData(imageData)) {
        // ディスクキャッシュに保存
        saveToCache(reply->url(), imageData);
        
        // メモリキャッシュに保存
        QString cacheKey = reply->url().toString();
        memoryCache.insert(cacheKey, new QPixmap(pixmap));
        
        // コールバックを呼び出し
        callback(pixmap);
    } else {
        emit loadError(reply->url().toString(), "Failed to create pixmap from data");
    }
}