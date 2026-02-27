#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QSet>

class ImageDownloader;

class MessageModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles { TypeRole = Qt::UserRole + 1, TextRole, TimestampRole };
    Q_ENUM(Roles)

    struct Message {
        QString text;
        QString type; // "system", "chat", "action", "error", "embed"
        QDateTime timestamp;
    };

    explicit MessageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addMessage(const QString &type, const QString &text);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QString allFormattedText() const;
    static QString ircToHtml(const QString &text);

signals:
    void messageAdded(const QString &formattedLine);
    void cleared();

private slots:
    void onImageReady(const QString &url, const QString &localPath, int width, int height);

private:
    static QString formatLine(const Message &msg);
    static QString linkifyUrls(const QString &html);
    QList<Message> m_messages;
    QSet<QString> m_pendingImages;
};
