#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QSet>
#include <QHash>

class ImageDownloader : public QObject
{
    Q_OBJECT
public:
    static ImageDownloader *instance();

    Q_INVOKABLE void download(const QString &url);
    Q_INVOKABLE bool isCached(const QString &url) const;
    Q_INVOKABLE QString cachedPath(const QString &url) const;
    Q_INVOKABLE bool saveImageToDownloads(const QString &url);
    Q_INVOKABLE void copyToClipboard(const QString &text);

    static bool isImageUrl(const QString &url);
    static bool isVideoUrl(const QString &url);

    // Cache key is the full SHA-256 of the URL (not a 64-bit prefix).
    static QString cacheFileName(const QString &url);
    // True when Qt can decode the bytes as an image. Callers must not cache otherwise.
    static bool isDecodableImage(const QByteArray &data);
    // Deletes oldest files in dir until the directory is within maxBytes.
    // Returns how many files were removed.
    static int trimCacheDirectory(const QString &dir, qint64 maxBytes);

signals:
    void imageReady(const QString &url, const QString &localPath, int width, int height);
    void downloadFailed(const QString &url);

private:
    explicit ImageDownloader(QObject *parent = nullptr);
    QString urlToFilename(const QString &url) const;
    void fetchIfHostSafe(const QString &originalUrl, const QUrl &target, int hop);
    void beginRequest(const QString &originalUrl, const QUrl &target, int hop);
    void enforceCacheLimit();

    QNetworkAccessManager *m_nam;
    QString m_cacheDir;
    QSet<QString> m_pending;

    static constexpr qint64 MAX_DOWNLOAD_SIZE = 15 * 1024 * 1024; // 15MB
    static constexpr qint64 MAX_CACHE_BYTES = 100 * 1024 * 1024;  // 100MB
    static constexpr int kMaxRedirects = 5;
};
