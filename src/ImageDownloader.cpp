#include "ImageDownloader.h"
#include "NetSafety.h"

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
#include <QHostAddress>
#include <QHostInfo>
#include <algorithm>

namespace {

QString urlForLog(const QUrl &url) {
    QUrl copy = url;
    copy.setUserName(QString());
    copy.setPassword(QString());
    return copy.toString(QUrl::FullyEncoded);
}

QString extensionForUrl(const QString &url) {
    QString ext = QStringLiteral(".dat");
    const QString path = QUrl(url).path().toLower();
    if (path.endsWith(QLatin1String(".png")))
        ext = QStringLiteral(".png");
    else if (path.endsWith(QLatin1String(".jpg")) || path.endsWith(QLatin1String(".jpeg")))
        ext = QStringLiteral(".jpg");
    else if (path.endsWith(QLatin1String(".gif")))
        ext = QStringLiteral(".gif");
    else if (path.endsWith(QLatin1String(".webp")))
        ext = QStringLiteral(".webp");
    else if (path.endsWith(QLatin1String(".svg")))
        ext = QStringLiteral(".svg");
    else if (path.endsWith(QLatin1String(".bmp")))
        ext = QStringLiteral(".bmp");
    return ext;
}

bool isImageContentType(const QByteArray &contentType) {
    static const QList<QByteArray> kImageMimeTypes = {
        "image/png", "image/jpeg", "image/jpg", "image/gif", "image/webp",
        "image/bmp", "image/svg+xml", "image/tiff"
    };
    for (const QByteArray &mime : kImageMimeTypes) {
        if (contentType.startsWith(mime))
            return true;
    }
    return false;
}

} // namespace

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

QString ImageDownloader::cacheFileName(const QString &url)
{
    const QByteArray hash = QCryptographicHash::hash(url.toUtf8(),
                                                     QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex()) + extensionForUrl(url);
}

QString ImageDownloader::urlToFilename(const QString &url) const
{
    return cacheFileName(url);
}

bool ImageDownloader::isCached(const QString &url) const
{
    return QFile::exists(m_cacheDir + QChar('/') + urlToFilename(url));
}

QString ImageDownloader::cachedPath(const QString &url) const
{
    return m_cacheDir + QChar('/') + urlToFilename(url);
}

bool ImageDownloader::isDecodableImage(const QByteArray &data)
{
    if (data.isEmpty() || data.size() > MAX_DOWNLOAD_SIZE)
        return false;
    QBuffer buf;
    buf.setData(data);
    if (!buf.open(QIODevice::ReadOnly))
        return false;
    QImageReader reader(&buf);
    reader.setAutoTransform(true);
    const QImage img = reader.read();
    return !img.isNull() && img.width() > 0 && img.height() > 0;
}

int ImageDownloader::trimCacheDirectory(const QString &dirPath, qint64 maxBytes)
{
    if (maxBytes < 0)
        maxBytes = 0;
    QDir dir(dirPath);
    if (!dir.exists())
        return 0;

    // Oldest first so a newly written preview is the last candidate for deletion.
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoSymLinks);
    std::sort(files.begin(), files.end(), [](const QFileInfo &a, const QFileInfo &b) {
        if (a.lastModified() != b.lastModified())
            return a.lastModified() < b.lastModified();
        return a.fileName() < b.fileName();
    });
    qint64 total = 0;
    for (const QFileInfo &fi : files)
        total += fi.size();

    int removed = 0;
    for (const QFileInfo &fi : files) {
        if (total <= maxBytes)
            break;
        const qint64 sz = fi.size();
        if (QFile::remove(fi.absoluteFilePath())) {
            total -= sz;
            ++removed;
        }
    }
    return removed;
}

void ImageDownloader::enforceCacheLimit()
{
    trimCacheDirectory(m_cacheDir, MAX_CACHE_BYTES);
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

    QUrl qurl(url);
    if (!NetSafety::isSafeImageUrl(qurl)) {
        qWarning() << "[ImageDownloader] Blocked unsafe URL:" << urlForLog(qurl);
        emit downloadFailed(url);
        return;
    }

    // Already cached – emit immediately, unless the file is not an image.
    if (isCached(url)) {
        const QString path = cachedPath(url);
        QImageReader reader(path);
        const QSize sz = reader.size();
        if (reader.canRead() && sz.width() > 0 && sz.height() > 0) {
            emit imageReady(url, path, sz.width(), sz.height());
            return;
        }
        QFile::remove(path);
    }

    // A configured proxy that cannot be applied must not fall back to a
    // direct fetch (that would leak the user's address).
    const NetSafety::ProxyResolution proxy = NetSafety::proxyFromSettings();
    if (proxy.result == NetSafety::ProxyResolution::Result::Rejected) {
        qWarning() << "[ImageDownloader]" << proxy.error;
        emit downloadFailed(url);
        return;
    }
    if (proxy.result == NetSafety::ProxyResolution::Result::UseProxy)
        m_nam->setProxy(proxy.proxy);
    else
        m_nam->setProxy(QNetworkProxy(QNetworkProxy::NoProxy));

    m_pending.insert(url);
    fetchIfHostSafe(url, qurl, 0);
}

void ImageDownloader::fetchIfHostSafe(const QString &originalUrl, const QUrl &target, int hop)
{
    if (!NetSafety::isSafeImageUrl(target)) {
        qWarning() << "[ImageDownloader] Blocked unsafe URL:" << urlForLog(target);
        m_pending.remove(originalUrl);
        emit downloadFailed(originalUrl);
        return;
    }

    QHostAddress literal;
    if (NetSafety::parseHostLiteral(target.host(), &literal)) {
        if (NetSafety::isBlockedAddress(literal)) {
            qWarning() << "[ImageDownloader] Blocked literal address:" << urlForLog(target);
            m_pending.remove(originalUrl);
            emit downloadFailed(originalUrl);
            return;
        }
        beginRequest(originalUrl, target, hop);
        return;
    }

    // Hostname: resolve first and refuse private answers. QNetworkAccessManager
    // performs its own lookup afterwards, so this is not a pin — a rebinding
    // between the two lookups can still occur. Redirects are not followed
    // until the next hop passes this same check, which is the request that
    // used to be sent before the private-address test.
    QHostInfo::lookupHost(target.host(), this, [this, originalUrl, target, hop](const QHostInfo &info) {
        if (info.error() != QHostInfo::NoError ||
            NetSafety::anyAddressBlocked(info.addresses())) {
            qWarning() << "[ImageDownloader] Host resolves to a blocked address:"
                       << target.host();
            m_pending.remove(originalUrl);
            emit downloadFailed(originalUrl);
            return;
        }
        beginRequest(originalUrl, target, hop);
    });
}

void ImageDownloader::beginRequest(const QString &originalUrl, const QUrl &target, int hop)
{
    QNetworkRequest req{target};
    // Manual so a redirect to a private host is not requested before we check it.
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::ManualRedirectPolicy);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("NUchat/1.0"));

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::downloadProgress, reply,
            [reply](qint64 received, qint64) {
                if (received > MAX_DOWNLOAD_SIZE)
                    reply->abort();
            });
    connect(reply, &QNetworkReply::finished, this, [this, reply, originalUrl, hop]() {
        reply->deleteLater();

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QUrl redir = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (status >= 300 && status < 400) {
            if (!redir.isValid() || redir.isEmpty() || hop + 1 > kMaxRedirects) {
                qWarning() << "[ImageDownloader] Redirect refused:" << urlForLog(reply->url());
                m_pending.remove(originalUrl);
                emit downloadFailed(originalUrl);
                return;
            }
            const QUrl next = reply->url().resolved(redir);
            // Validate (and resolve) the next hop before any bytes are requested.
            fetchIfHostSafe(originalUrl, next, hop + 1);
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Image download failed:" << reply->errorString()
                       << urlForLog(reply->url());
            m_pending.remove(originalUrl);
            emit downloadFailed(originalUrl);
            return;
        }

        if (!NetSafety::isSafeImageUrl(reply->url())) {
            qWarning() << "[ImageDownloader] Response URL blocked:"
                       << urlForLog(reply->url());
            m_pending.remove(originalUrl);
            emit downloadFailed(originalUrl);
            return;
        }

        const QByteArray contentType =
            reply->header(QNetworkRequest::ContentTypeHeader).toByteArray().toLower();
        if (!contentType.isEmpty() && !isImageContentType(contentType)) {
            qWarning() << "[ImageDownloader] Non-image Content-Type rejected:"
                       << contentType << urlForLog(reply->url());
            m_pending.remove(originalUrl);
            emit downloadFailed(originalUrl);
            return;
        }

        const QByteArray data = reply->readAll();
        if (data.size() > MAX_DOWNLOAD_SIZE || !isDecodableImage(data)) {
            qWarning() << "[ImageDownloader] Payload is not a decodable image:"
                       << urlForLog(reply->url());
            m_pending.remove(originalUrl);
            emit downloadFailed(originalUrl);
            return;
        }

        m_pending.remove(originalUrl);

        const QString path = cachedPath(originalUrl);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            emit downloadFailed(originalUrl);
            return;
        }
        if (file.write(data) != data.size()) {
            file.close();
            file.remove();
            emit downloadFailed(originalUrl);
            return;
        }
        file.close();
        enforceCacheLimit();

        QBuffer buf;
        buf.setData(data);
        buf.open(QIODevice::ReadOnly);
        QImageReader reader(&buf);
        const QSize sz = reader.size();
        emit imageReady(originalUrl, path, sz.width(), sz.height());
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
