#include "SecretStore.h"

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <memory>

#ifdef HAVE_QTKEYCHAIN
#if defined(__has_include)
#if __has_include(<qt6keychain/keychain.h>)
#include <qt6keychain/keychain.h>
#elif __has_include(<qtkeychain/keychain.h>)
#include <qtkeychain/keychain.h>
#else
#error "HAVE_QTKEYCHAIN is set but no qtkeychain header was found"
#endif
#else
#include <qtkeychain/keychain.h>
#endif
#endif

namespace {

const QString kService = QStringLiteral("NUchat");
constexpr int kKeychainTimeoutMs = 15000;

#ifdef HAVE_QTKEYCHAIN
struct JobState {
    bool done = false;
    QKeychain::Error error = QKeychain::OtherError;
    QString text;
};

// Blocks until the job finishes or the timeout elapses. On timeout the job
// is left to delete itself; the caller treats that as failure and must not
// drop its plaintext copy.
JobState waitForJob(QKeychain::Job *job) {
    auto state = std::make_shared<JobState>();
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    job->setAutoDelete(false);
    QMetaObject::Connection doneConn = QObject::connect(
        job, &QKeychain::Job::finished, &loop,
        [&loop, state](QKeychain::Job *finishedJob) {
            state->done = true;
            state->error = finishedJob->error();
            if (auto *readJob = qobject_cast<QKeychain::ReadPasswordJob *>(finishedJob))
                state->text = readJob->textData();
            loop.quit();
        });
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(kKeychainTimeoutMs);
    job->start();
    loop.exec();
    QObject::disconnect(doneConn);
    timer.stop();

    if (!state->done) {
        // Still running. Detach it so destroying the stack frame is safe.
        QObject::disconnect(job, nullptr, nullptr, nullptr);
        job->setAutoDelete(true);
        if (QCoreApplication::instance() && !job->parent())
            job->setParent(QCoreApplication::instance());
        return *state;
    }
    delete job;
    return *state;
}
#endif

void warnKeychainUnavailable() {
    static bool warned = false;
    if (warned)
        return;
    warned = true;
    qWarning().noquote()
        << "NUchat: the OS keychain is unavailable, so passwords stay in "
           "NUchat.conf. Install QtKeychain and rebuild to move them; "
           "see the README.";
}

} // namespace

SecretStore &SecretStore::instance() {
    static SecretStore store;
    return store;
}

void SecretStore::setModeForTests(Mode mode) {
    instance().m_mode = mode;
    resetForTests();
}

void SecretStore::resetForTests() {
    SecretStore &s = instance();
    s.m_memory.clear();
    s.m_cache.clear();
    s.m_missing.clear();
    s.m_failedReads.clear();
}

bool SecretStore::memoryMode() const {
    return m_mode == Mode::InMemory;
}

bool SecretStore::isSecure() const {
    if (m_mode == Mode::InMemory)
        return true;
    if (m_mode == Mode::Unavailable)
        return false;
#ifdef HAVE_QTKEYCHAIN
    return systemAvailable();
#else
    return false;
#endif
}

bool SecretStore::systemAvailable() const {
#ifdef HAVE_QTKEYCHAIN
    static int cached = -1;
    if (cached < 0)
        cached = QKeychain::isAvailable() ? 1 : 0;
    return cached == 1;
#else
    return false;
#endif
}

void SecretStore::noteSuccess(const QString &key, const QString &secret) {
    m_cache.insert(key, secret);
    m_missing.remove(key);
    m_failedReads.remove(key);
}

void SecretStore::noteMissing(const QString &key) {
    m_cache.remove(key);
    m_missing.insert(key);
    m_failedReads.remove(key);
}

void SecretStore::noteFailed(const QString &key) {
    m_cache.remove(key);
    m_missing.remove(key);
    m_failedReads.insert(key);
}

bool SecretStore::readFailed(const QString &key) const {
    return m_failedReads.contains(key);
}

SecretStore::ReadStatus SecretStore::read(const QString &key, QString *value) {
    if (value)
        value->clear();
    if (key.isEmpty())
        return ReadStatus::Missing;

    if (memoryMode()) {
        auto it = m_memory.constFind(key);
        if (it == m_memory.constEnd())
            return ReadStatus::Missing;
        if (value)
            *value = it.value();
        return ReadStatus::Found;
    }
    if (m_mode == Mode::Unavailable)
        return ReadStatus::Missing;

    if (m_cache.contains(key)) {
        if (value)
            *value = m_cache.value(key);
        return ReadStatus::Found;
    }
    if (m_missing.contains(key))
        return ReadStatus::Missing;
    if (m_failedReads.contains(key))
        return ReadStatus::Failed;

#ifdef HAVE_QTKEYCHAIN
    if (!systemAvailable()) {
        noteFailed(key);
        return ReadStatus::Failed;
    }
    QString text;
    ReadStatus status = systemRead(key, &text);
    if (status == ReadStatus::Found) {
        noteSuccess(key, text);
        if (value)
            *value = text;
    } else if (status == ReadStatus::Missing) {
        noteMissing(key);
    } else {
        noteFailed(key);
    }
    return status;
#else
    noteFailed(key);
    return ReadStatus::Failed;
#endif
}

bool SecretStore::write(const QString &key, const QString &secret) {
    if (key.isEmpty())
        return false;
    if (memoryMode()) {
        m_memory.insert(key, secret);
        noteSuccess(key, secret);
        return true;
    }
    if (m_mode == Mode::Unavailable) {
        return false;
    }
#ifdef HAVE_QTKEYCHAIN
    if (!systemAvailable() || !systemWrite(key, secret)) {
        warnKeychainUnavailable();
        noteFailed(key);
        return false;
    }
    noteSuccess(key, secret);
    return true;
#else
    warnKeychainUnavailable();
    noteFailed(key);
    return false;
#endif
}

bool SecretStore::remove(const QString &key) {
    if (key.isEmpty())
        return true;
    if (memoryMode()) {
        m_memory.remove(key);
        noteMissing(key);
        return true;
    }
    if (m_mode == Mode::Unavailable) {
        noteMissing(key);
        return true;
    }
#ifdef HAVE_QTKEYCHAIN
    if (!systemAvailable()) {
        noteFailed(key);
        return false;
    }
    if (!systemRemove(key)) {
        noteFailed(key);
        return false;
    }
    noteMissing(key);
    return true;
#else
    noteFailed(key);
    return false;
#endif
}

QStringList SecretStore::keysForTests() const {
    if (memoryMode())
        return m_memory.keys();
    return m_cache.keys();
}

#ifdef HAVE_QTKEYCHAIN
SecretStore::ReadStatus SecretStore::systemRead(const QString &key, QString *value) {
    auto *job = new QKeychain::ReadPasswordJob(kService);
    job->setKey(key);
    // Never let QtKeychain invent its own plaintext file. Fallback is the
    // caller's job, so a failed write leaves the original config untouched.
    job->setInsecureFallback(false);
    JobState state = waitForJob(job);
    if (!state.done)
        return ReadStatus::Failed;
    if (state.error == QKeychain::EntryNotFound)
        return ReadStatus::Missing;
    if (state.error != QKeychain::NoError)
        return ReadStatus::Failed;
    if (value)
        *value = state.text;
    return ReadStatus::Found;
}

bool SecretStore::systemWrite(const QString &key, const QString &secret) {
    auto *job = new QKeychain::WritePasswordJob(kService);
    job->setKey(key);
    job->setTextData(secret);
    job->setInsecureFallback(false);
    JobState state = waitForJob(job);
    return state.done && state.error == QKeychain::NoError;
}

bool SecretStore::systemRemove(const QString &key) {
    auto *job = new QKeychain::DeletePasswordJob(kService);
    job->setKey(key);
    job->setInsecureFallback(false);
    JobState state = waitForJob(job);
    if (!state.done)
        return false;
    return state.error == QKeychain::NoError
        || state.error == QKeychain::EntryNotFound;
}
#else
SecretStore::ReadStatus SecretStore::systemRead(const QString &, QString *) {
    return ReadStatus::Failed;
}
bool SecretStore::systemWrite(const QString &, const QString &) { return false; }
bool SecretStore::systemRemove(const QString &) { return false; }
#endif
