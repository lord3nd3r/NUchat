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

signals:
    void imageReady(const QString &url, const QString &localPath, int width, int height);
    void downloadFailed(const QString &url);

private:
    explicit ImageDownloader(QObject *parent = nullptr);
    QString urlToFilename(const QString &url) const;

    QNetworkAccessManager *m_nam;
    QString m_cacheDir;
    QSet<QString> m_pending;

    static constexpr qint64 MAX_DOWNLOAD_SIZE = 15 * 1024 * 1024; // 15MB
};
