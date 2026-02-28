#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QSet>

class ImageDownloader;

class MessageModel : public QAbstractListModel {
  Q_OBJECT
public:
  enum Roles {
    TypeRole = Qt::UserRole + 1,
    TextRole,
    TimestampRole,
    FormattedTextRole
  };
  Q_ENUM(Roles)

  struct Message {
    QString text;          // raw IRC text
    QString formattedText; // pre-rendered HTML for display
    QString type;          // "system", "chat", "action", "error", "embed"
    QDateTime timestamp;
  };

  explicit MessageModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void addMessage(const QString &type, const QString &text);
  Q_INVOKABLE void clear();
  Q_INVOKABLE QString allFormattedText() const;
  Q_INVOKABLE void setDarkMode(bool dark);
  Q_INVOKABLE QString formatLineFromQml(const QString &text,
                                        const QString &type,
                                        const QString &timestamp) const;
  Q_INVOKABLE void setNickname(const QString &nick);
  static QString ircToHtml(const QString &text);

signals:
  void messageAdded(const QString &formattedLine);
  void cleared();

private slots:
  void onImageReady(const QString &url, const QString &localPath, int width,
                    int height);

private:
  QString formatLine(const Message &msg) const;
  static QString linkifyUrls(const QString &html);
  static QString colorizeNicks(const QString &html);
  static QString nickColor(const QString &nick);
  QList<Message> m_messages;
  QSet<QString> m_pendingImages;
  QString m_nickname;
};
