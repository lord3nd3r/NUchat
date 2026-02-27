#include "ImageDownloader.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QCryptographicHash>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QImageReader>
#include <QBuffer>
#include <QUrl>
#include <QGuiApplication>
#include <QClipboard>
#include <QFileInfo>

ImageDownloader *ImageDownloader::instance()
{
    static ImageDownloader inst;
    return &inst;
}

ImageDownloader::ImageDownloader(QObject *parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
    m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                 + QStringLiteral("/images");
    QDir().mkpath(m_cacheDir);
}

// ---------- helpers ----------

QString ImageDownloader::urlToFilename(const QString &url) const
{
    QByteArray hash = QCryptographicHash::hash(url.toUtf8(),
                                               QCryptographicHash::Sha256);
    QString hex = QString::fromLatin1(hash.toHex().left(16));

    // Determine extension from URL path
    QString ext = QStringLiteral(".dat");
    QUrl qurl(url);
    QString path = qurl.path().toLower();
    if (path.endsWith(QLatin1String(".png")))        ext = QStringLiteral(".png");
    else if (path.endsWith(QLatin1String(".jpg"))
             || path.endsWith(QLatin1String(".jpeg"))) ext = QStringLiteral(".jpg");
    else if (path.endsWith(QLatin1String(".gif")))     ext = QStringLiteral(".gif");
    else if (path.endsWith(QLatin1String(".webp")))    ext = QStringLiteral(".webp");
    else if (path.endsWith(QLatin1String(".svg")))     ext = QStringLiteral(".svg");
    else if (path.endsWith(QLatin1String(".bmp")))     ext = QStringLiteral(".bmp");
    return hex + ext;
}

bool ImageDownloader::isCached(const QString &url) const
{
    return QFile::exists(m_cacheDir + QChar('/') + urlToFilename(url));
}

QString ImageDownloader::cachedPath(const QString &url) const
{
    return m_cacheDir + QChar('/') + urlToFilename(url);
}

// ---------- URL classification ----------

bool ImageDownloader::isImageUrl(const QString &url)
{
    static const QStringList exts = {
        ".png", ".jpg", ".jpeg", ".gif", ".webp", ".bmp", ".svg", ".tiff"
    };
    QUrl qurl(url);
    QString path = qurl.path().toLower();
    for (const auto &e : exts) {
        if (path.endsWith(e)) return true;
    }
    // Well-known image hosts that may omit extensions
    QString host = qurl.host().toLower();
    if (host == QLatin1String("i.imgur.com")
        || host == QLatin1String("i.redd.it")
        || host.endsWith(QLatin1String(".gyazo.com")))
        return true;
    return false;
}

bool ImageDownloader::isVideoUrl(const QString &url)
{
    static const QStringList exts = {
        ".mp4", ".webm", ".mov", ".avi", ".mkv", ".m4v"
    };
    QUrl qurl(url);
    QString path = qurl.path().toLower();
    for (const auto &e : exts) {
        if (path.endsWith(e)) return true;
    }
    return false;
}

// ---------- download ----------

void ImageDownloader::download(const QString &url)
{
    if (m_pending.contains(url))
        return;

    // Already cached – emit immediately
    if (isCached(url)) {
        QString path = cachedPath(url);
        QImageReader reader(path);
        QSize sz = reader.size();
        emit imageReady(url, path, sz.width(), sz.height());
        return;
    }

    m_pending.insert(url);

    QNetworkRequest req{QUrl(url)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setMaximumRedirectsAllowed(5);
    req.setHeader(QNetworkRequest::UserAgentHeader, "NUchat/1.0");

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        reply->deleteLater();
        m_pending.remove(url);

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Image download failed:" << reply->errorString() << url;
            emit downloadFailed(url);
            return;
        }

        QByteArray data = reply->readAll();
        if (data.size() > MAX_DOWNLOAD_SIZE) {
            qWarning() << "Image too large, skipping:" << url;
            emit downloadFailed(url);
            return;
        }

        // Save to cache
        QString path = cachedPath(url);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            emit downloadFailed(url);
            return;
        }
        file.write(data);
        file.close();

        // Read dimensions
        QBuffer buf(&data);
        buf.open(QIODevice::ReadOnly);
        QImageReader reader(&buf);
        QSize sz = reader.size();

        emit imageReady(url, path, sz.width(), sz.height());
    });
}

// ---------- save / clipboard ----------

bool ImageDownloader::saveImageToDownloads(const QString &url)
{
    if (!isCached(url))
        return false;

    QString destDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (destDir.isEmpty())
        destDir = QDir::homePath() + QStringLiteral("/Downloads");
    QDir().mkpath(destDir);

    QString src = cachedPath(url);
    QFileInfo fi(QUrl(url).path());
    QString filename = fi.fileName();
    if (filename.isEmpty()) filename = QStringLiteral("image.png");

    QString dest = destDir + QChar('/') + filename;
    // Don't overwrite
    int n = 1;
    while (QFile::exists(dest)) {
        dest = destDir + QChar('/') + fi.completeBaseName()
               + QStringLiteral("_%1.").arg(n++) + fi.suffix();
    }
    return QFile::copy(src, dest);
}

void ImageDownloader::copyToClipboard(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);
}
