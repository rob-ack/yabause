#include "QGameInfo.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QLocale>
#include <QSettings>
#include <QDir>
#include <stdexcept>
#include <memory>
#include "chd.h"

QString QGameInfo::convertShiftJISToUnicode(const QByteArray& input) {
  UErrorCode status = U_ZERO_ERROR;
  UConverter* converter = ucnv_open("Shift-JIS", &status);

  if (U_FAILURE(status)) {
    throw std::runtime_error("Failed to create Shift-JIS converter");
  }

  // Use RAII to ensure converter is properly closed
  std::unique_ptr<UConverter, decltype(&ucnv_close)> converterGuard(converter, ucnv_close);

  // First, calculate the required buffer size
  const char* source = input.constData();
  int32_t sourceLength = input.length();
  status = U_ZERO_ERROR;

  int32_t targetSize = ucnv_toUChars(converter, nullptr, 0, source, sourceLength, &status);
  if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status)) {
    throw std::runtime_error("Failed to calculate buffer size");
  }

  // Allocate buffer and perform conversion
  std::unique_ptr<UChar[]> target(new UChar[targetSize + 1]);
  status = U_ZERO_ERROR;

  int32_t length = ucnv_toUChars(converter, target.get(), targetSize + 1,
    source, sourceLength, &status);

  if (U_FAILURE(status)) {
    throw std::runtime_error("Failed to convert Shift-JIS to Unicode");
  }

  // Convert to QString and trim whitespace
  return QString::fromUtf16(target.get(), length).trimmed();
}



QGameInfo* QGameInfo::fromChdFile(const QString& filePath) {

  QByteArray filePathUtf8 = filePath.toUtf8();
  const char* path = filePathUtf8.constData();

  chd_file* chd;
  char* hunk_buffer;
  int current_hunk_id;
  const int len = 256;
  char* buf = (char*)malloc(sizeof(char) * len);

  chd_error error = chd_open(path, CHD_OPEN_READ, NULL, &chd);
  if (error != CHDERR_NONE) {
    return NULL;
  }
  const chd_header* header = chd_get_header(chd);
  if (header == NULL) {
    return NULL;
  }

  hunk_buffer = (char*)malloc(header->hunkbytes);
  chd_read(chd, 0, hunk_buffer);

  memcpy(buf, &hunk_buffer[16], len);
  buf[len - 1] = 0;
  //putc(buf[0], stdout);
  //putc(buf[1], stdout);
  //putc(buf[2], stdout);
  //putc(buf[3], stdout);
  free(hunk_buffer);
  chd_close(chd);
  
  auto ptr = fromBuffer(filePath, QByteArray(buf));
  free(buf);
  return ptr;

}



QGameInfo* QGameInfo::fromIsoFile(const QString& filePath) {
  if (filePath.isEmpty()) return nullptr;

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) return nullptr;

  QByteArray header = file.read(0xFF);
  file.close();

  return fromBuffer(filePath, header);
}

QGameInfo* QGameInfo::fromMdsFile(const QString& filePath) {
  if (filePath.isEmpty()) return nullptr;

  QString isoFileName = filePath;
  isoFileName.replace(".mds", ".mdf");

  if (!QFile::exists(isoFileName)) {
    throw GameInfoError(QString("ISO file %1 does not exist.").arg(isoFileName));
  }

  QGameInfo* info = fromIsoFile(isoFileName);
  if (info) {
    info->filePath = filePath;
  }
  return info;
}

QGameInfo* QGameInfo::fromCcdFile(const QString& filePath) {
  if (filePath.isEmpty()) return nullptr;

  QString isoFileName = filePath;
  isoFileName.replace(".ccd", ".img");

  if (!QFile::exists(isoFileName)) {
    throw GameInfoError(QString("ISO file %1 does not exist.").arg(isoFileName));
  }

  QGameInfo* info = fromIsoFile(isoFileName);
  if (info) {
    info->filePath = filePath;
  }
  return info;
}

QGameInfo* QGameInfo::fromCueFile(const QString& filePath) {
  if (filePath.isEmpty()) return nullptr;

  QFileInfo fileInfo(filePath);
  QString dirPath = fileInfo.absolutePath();
  QString isoFileName;

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return nullptr;
  }

  QTextStream in(&file);
  QRegularExpression filePattern("FILE \"(.*)\"");

  while (!in.atEnd()) {
    QString line = in.readLine();
    auto match = filePattern.match(line);
    if (match.hasMatch()) {
      isoFileName = match.captured(1);
      break;
    }
  }
  file.close();

  if (isoFileName.isEmpty()) {
    throw GameInfoError("ISO file does not exist.");
  }

  if (!dirPath.isEmpty()) {
    isoFileName = QDir(dirPath).filePath(isoFileName);

    if (!QFile::exists(isoFileName)) {
      return nullptr;
    }
  }

  QGameInfo* info = fromIsoFile(isoFileName);
  if (info) {
    info->filePath = filePath;
  }
  return info;
}

QGameInfo* QGameInfo::fromBuffer(const QString& filePath, const QByteArray& header) {
  if (filePath.isEmpty() || header.isEmpty()) return nullptr;

  QByteArray checkStr = "SEGA ";
  int startIndex = header.indexOf(checkStr);
  if (startIndex == -1) return nullptr;

  QGameInfo* info = new QGameInfo();
  info->filePath = filePath;

  try {
    auto readField = [&](int offset, int length) -> QString {
      QByteArray data = header.mid(startIndex + offset, length);
      return convertShiftJISToUnicode(data);
    };

    info->makerId = readField(0x10, 0x10);
    info->productNumber = readField(0x20, 0x0A);

    if (!info->productNumber.isEmpty()) {
      QSettings settings("settings.ini", QSettings::IniFormat);
      QString cloudfront = settings.value("CloudService/cloudfront").toString();
      if (!cloudfront.isEmpty()) {
        info->imageUrl = QString("https://d3edktb2n8l35b.cloudfront.net/BOXART/%1.PNG?%2")
          .arg(info->productNumber)
          .arg(cloudfront);
      }
    }

    info->version = readField(0x2A, 0x10);
    info->releaseDate = readField(0x30, 0x08);
    info->area = readField(0x40, 0x0A);
    info->inputDevice = readField(0x50, 0x10);
    info->deviceInformation = readField(0x38, 0x08);
    info->gameTitle = readField(0x60, 0x70);

    // タイトルの処理
    QStringList titles = info->gameTitle.split("U:");
    if (titles.size() >= 2) {
      QString japaneseTitle = titles[0].replace("J:", "").trimmed();
      QString englishTitle = titles[1].trimmed();

      if (QLocale::system().language() == QLocale::Japanese) {
        info->displayName = japaneseTitle;
      }
      else {
        info->displayName = englishTitle;
      }
    }
    else {
      info->displayName = info->gameTitle;
    }

    if (info->deviceInformation != "CD-1/1") {
      info->displayName = info->displayName + " " + info->deviceInformation;
    }

    return info;

  }
  catch (const std::exception& e) {
    delete info;
    throw GameInfoError(QString("Error processing game info: %1").arg(e.what()));
  }
}