#pragma once

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

// Named secrets (server PASS, SASL, NickServ, ZNC, proxy).
//
// When QtKeychain is compiled in and the OS keychain is reachable, secrets
// live there. Otherwise write() fails and callers keep the previous plaintext
// so an existing config is never discarded.
class SecretStore
{
public:
    enum class Mode {
        Auto,         // OS keychain if built and available, else unavailable
        InMemory,     // process-local map (unit tests)
        Unavailable   // force the plaintext fallback (unit tests)
    };

    enum class ReadStatus { Found, Missing, Failed };

    static SecretStore &instance();

    // Test hooks. Production code leaves the mode at Auto.
    static void setModeForTests(Mode mode);
    static void resetForTests();

    // True when a write is expected to land in a secure backend.
    bool isSecure() const;

    ReadStatus read(const QString &key, QString *value);
    // False means the secret was NOT persisted securely. Callers must keep
    // their own copy.
    bool write(const QString &key, const QString &secret);
    // True if the secret is gone or was never there. False on a hard failure
    // (the caller should not assume it was deleted).
    bool remove(const QString &key);

    // A previous read() of this key returned Failed and nothing has succeeded
    // since. Callers must not treat an empty UI field as "user cleared it".
    bool readFailed(const QString &key) const;

    QStringList keysForTests() const;

private:
    SecretStore() = default;

    Mode m_mode = Mode::Auto;
    QHash<QString, QString> m_memory;
    QHash<QString, QString> m_cache;
    QSet<QString> m_missing;
    QSet<QString> m_failedReads;

    bool memoryMode() const;
    bool systemAvailable() const;
    ReadStatus systemRead(const QString &key, QString *value);
    bool systemWrite(const QString &key, const QString &secret);
    bool systemRemove(const QString &key);
    void noteSuccess(const QString &key, const QString &secret);
    void noteMissing(const QString &key);
    void noteFailed(const QString &key);
};
