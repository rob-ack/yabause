#ifndef GAMEINFO_H
#define GAMEINFO_H

#include <QString>
#include <QObject>
#include <QByteArray>
#include <unicode/ucnv.h>

class QGameInfo {
public:
    QString filePath;
    QString makerId;
    QString productNumber;
    QString version;
    QString releaseDate;
    QString area;
    QString inputDevice;
    QString deviceInformation;
    QString gameTitle;
    QString displayName;
    QString imageUrl;
    
    static QGameInfo* fromIsoFile(const QString& filePath);
    static QGameInfo* fromMdsFile(const QString& filePath);
    static QGameInfo* fromCcdFile(const QString& filePath);
    static QGameInfo* fromCueFile(const QString& filePath);
    static QGameInfo* fromChdFile(const QString& filePath);
    
private:
    static QGameInfo* fromBuffer(const QString& filePath, const QByteArray& header);
    static QString convertShiftJISToUnicode(const QByteArray& input);
};

Q_DECLARE_METATYPE(QGameInfo)

class GameInfoError : public QObject {
    Q_OBJECT
public:
    explicit GameInfoError(const QString& message) : errorMessage(message) {}
    QString errorMessage;
};



#endif // GAMEINFO_H
