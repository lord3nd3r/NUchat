// ── CPython embedding ──
// Python.h must be included FIRST, before any Qt headers, because Python's
// object.h uses "slots" as a struct member which conflicts with Qt's macro.
#define PY_SSIZE_T_CLEAN
#undef slots
#include <Python.h>
#define slots Q_SLOTS

#include "PythonScriptEngine.h"
#include "IRCConnectionManager.h"
#include "IrcConnection.h"
#include "MessageModel.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>

PythonScriptEngine *PythonScriptEngine::s_instance = nullptr;

// ──────────────────────────────────────────────────────────────
//  HexChat eat constants
// ──────────────────────────────────────────────────────────────
static constexpr int EAT_NONE   = 0;
static constexpr int EAT_HEXCHAT = 1;
static constexpr int EAT_PLUGIN = 2;
static constexpr int EAT_ALL    = 3;

// Priority constants
static constexpr int PRI_HIGHEST = 127;
static constexpr int PRI_HIGH    = 64;
static constexpr int PRI_NORM    = 0;
static constexpr int PRI_LOW     = -64;
static constexpr int PRI_LOWEST  = -128;

// ──────────────────────────────────────────────────────────────
//  Helper: call a Python callable safely
// ──────────────────────────────────────────────────────────────
static int callPyHook(PyObject *cb, PyObject *word, PyObject *word_eol, PyObject *userdata)
{
    if (!cb || !PyCallable_Check(cb)) return EAT_NONE;

    PyObject *args;
    if (userdata && userdata != Py_None) {
        args = PyTuple_Pack(3, word, word_eol, userdata);
    } else {
        args = PyTuple_Pack(2, word, word_eol);
    }
    PyObject *result = PyObject_CallObject(cb, args);
    Py_XDECREF(args);

    int ret = EAT_NONE;
    if (result) {
        if (PyLong_Check(result))
            ret = (int)PyLong_AsLong(result);
        Py_DECREF(result);
    } else {
        PyErr_Print();
    }
    return ret;
}

// Build word and word_eol Python lists from QStringList
static void buildWordLists(const QStringList &parts, PyObject **outWord, PyObject **outWordEol)
{
    // HexChat word arrays are 1-indexed with empty [0]
    int n = parts.size();
    PyObject *word = PyList_New(n + 1);
    PyObject *word_eol = PyList_New(n + 1);

    PyList_SetItem(word, 0, PyUnicode_FromString(""));
    PyList_SetItem(word_eol, 0, PyUnicode_FromString(""));

    for (int i = 0; i < n; ++i) {
        PyList_SetItem(word, i + 1, PyUnicode_FromString(parts[i].toUtf8().constData()));
        // word_eol[i+1] = parts[i..end].join(' ')
        QStringList tail = parts.mid(i);
        PyList_SetItem(word_eol, i + 1, PyUnicode_FromString(tail.join(' ').toUtf8().constData()));
    }

    *outWord = word;
    *outWordEol = word_eol;
}

// ──────────────────────────────────────────────────────────────
//  hexchat Python module C functions
// ──────────────────────────────────────────────────────────────

static PyObject *py_hexchat_command(PyObject * /*self*/, PyObject *args)
{
    const char *cmd;
    if (!PyArg_ParseTuple(args, "s", &cmd)) return nullptr;
    if (PythonScriptEngine::instance())
        PythonScriptEngine::instance()->command(QString::fromUtf8(cmd));
    Py_RETURN_NONE;
}

static PyObject *py_hexchat_prnt(PyObject * /*self*/, PyObject *args)
{
    const char *text;
    if (!PyArg_ParseTuple(args, "s", &text)) return nullptr;
    if (PythonScriptEngine::instance())
        PythonScriptEngine::instance()->prnt(QString::fromUtf8(text));
    Py_RETURN_NONE;
}

static PyObject *py_hexchat_emit_print(PyObject * /*self*/, PyObject *args)
{
    // emit_print(event, *args) — we just print the args for now
    Py_ssize_t n = PyTuple_Size(args);
    if (n < 1) Py_RETURN_NONE;
    PyObject *ev = PyTuple_GetItem(args, 0);
    Q_UNUSED(ev);
    // Concatenate remaining args
    QString text;
    for (Py_ssize_t i = 1; i < n; ++i) {
        PyObject *a = PyTuple_GetItem(args, i);
        const char *s = PyUnicode_AsUTF8(a);
        if (s) { if (!text.isEmpty()) text += ' '; text += QString::fromUtf8(s); }
    }
    if (PythonScriptEngine::instance())
        PythonScriptEngine::instance()->prnt(text);
    Py_RETURN_NONE;
}

static PyObject *py_hexchat_get_info(PyObject * /*self*/, PyObject *args)
{
    const char *id;
    if (!PyArg_ParseTuple(args, "s", &id)) return nullptr;
    if (PythonScriptEngine::instance()) {
        QString val = PythonScriptEngine::instance()->getInfo(QString::fromUtf8(id));
        if (!val.isNull())
            return PyUnicode_FromString(val.toUtf8().constData());
    }
    Py_RETURN_NONE;
}

static PyObject *py_hexchat_hook_command(PyObject * /*self*/, PyObject *args, PyObject *kwargs)
{
    const char *name;
    PyObject *callback;
    PyObject *userdata = Py_None;
    int priority = PRI_NORM;
    const char *help = nullptr;

    static const char *kwlist[] = {"name", "callback", "userdata", "priority", "help", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|Ois",
            const_cast<char**>(kwlist), &name, &callback, &userdata, &priority, &help))
        return nullptr;

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return nullptr;
    }

    if (PythonScriptEngine::instance()) {
        Py_INCREF(callback);
        Py_INCREF(userdata);
        int id = PythonScriptEngine::instance()->hookCommand(
            QString::fromUtf8(name).toUpper(), (void*)callback, (void*)userdata, priority);
        return PyLong_FromLong(id);
    }
    Py_RETURN_NONE;
}

static PyObject *py_hexchat_hook_server(PyObject * /*self*/, PyObject *args, PyObject *kwargs)
{
    const char *name;
    PyObject *callback;
    PyObject *userdata = Py_None;
    int priority = PRI_NORM;

    static const char *kwlist[] = {"name", "callback", "userdata", "priority", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|Oi",
            const_cast<char**>(kwlist), &name, &callback, &userdata, &priority))
        return nullptr;

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return nullptr;
    }

    if (PythonScriptEngine::instance()) {
        Py_INCREF(callback);
        Py_INCREF(userdata);
        int id = PythonScriptEngine::instance()->hookServer(
            QString::fromUtf8(name).toUpper(), (void*)callback, (void*)userdata, priority);
        return PyLong_FromLong(id);
    }
    Py_RETURN_NONE;
}

static PyObject *py_hexchat_hook_print(PyObject * /*self*/, PyObject *args, PyObject *kwargs)
{
    const char *name;
    PyObject *callback;
    PyObject *userdata = Py_None;
    int priority = PRI_NORM;

    static const char *kwlist[] = {"name", "callback", "userdata", "priority", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|Oi",
            const_cast<char**>(kwlist), &name, &callback, &userdata, &priority))
        return nullptr;

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return nullptr;
    }

    if (PythonScriptEngine::instance()) {
        Py_INCREF(callback);
        Py_INCREF(userdata);
        int id = PythonScriptEngine::instance()->hookPrint(
            QString::fromUtf8(name), (void*)callback, (void*)userdata, priority);
        return PyLong_FromLong(id);
    }
    Py_RETURN_NONE;
}

static PyObject *py_hexchat_hook_timer(PyObject * /*self*/, PyObject *args, PyObject *kwargs)
{
    int timeout;
    PyObject *callback;
    PyObject *userdata = Py_None;

    static const char *kwlist[] = {"timeout", "callback", "userdata", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iO|O",
            const_cast<char**>(kwlist), &timeout, &callback, &userdata))
        return nullptr;

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return nullptr;
    }

    if (PythonScriptEngine::instance()) {
        Py_INCREF(callback);
        Py_INCREF(userdata);
        int id = PythonScriptEngine::instance()->hookTimer(timeout, (void*)callback, (void*)userdata);
        return PyLong_FromLong(id);
    }
    Py_RETURN_NONE;
}

static PyObject *py_hexchat_unhook(PyObject * /*self*/, PyObject *args)
{
    int hookId;
    if (!PyArg_ParseTuple(args, "i", &hookId)) return nullptr;
    if (PythonScriptEngine::instance())
        PythonScriptEngine::instance()->unhook(hookId);
    Py_RETURN_NONE;
}

static PyObject *py_hexchat_nickcmp(PyObject * /*self*/, PyObject *args)
{
    const char *a, *b;
    if (!PyArg_ParseTuple(args, "ss", &a, &b)) return nullptr;
    // IRC nick comparison: case-insensitive per RFC 1459
    return PyLong_FromLong(QString::fromUtf8(a).compare(QString::fromUtf8(b), Qt::CaseInsensitive));
}

static PyObject *py_hexchat_get_list(PyObject * /*self*/, PyObject *args)
{
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name)) return nullptr;
    // Stub: return empty list for now; extend later
    return PyList_New(0);
}

static PyObject *py_hexchat_get_context(PyObject * /*self*/, PyObject * /*args*/)
{
    // Return a stub context (just an integer sentinel)
    return PyLong_FromLong(1);
}

static PyObject *py_hexchat_find_context(PyObject * /*self*/, PyObject *args, PyObject *kwargs)
{
    const char *server = nullptr, *channel = nullptr;
    static const char *kwlist[] = {"server", "channel", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|zz",
            const_cast<char**>(kwlist), &server, &channel))
        return nullptr;
    // Stub context
    return PyLong_FromLong(1);
}

static PyObject *py_hexchat_set_context(PyObject * /*self*/, PyObject *args)
{
    Q_UNUSED(args);
    // Stub — always succeed
    return PyLong_FromLong(1);
}

static PyObject *py_hexchat_get_prefs(PyObject * /*self*/, PyObject *args)
{
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name)) return nullptr;
    // Return None for unknown prefs
    Py_RETURN_NONE;
}

// ──────────────────────────────────────────────────────────────
//  hexchat module definition
// ──────────────────────────────────────────────────────────────

static PyMethodDef hexchat_methods[] = {
    {"command",       py_hexchat_command,       METH_VARARGS, "Execute an IRC command"},
    {"prnt",          py_hexchat_prnt,          METH_VARARGS, "Print text to the chat window"},
    {"emit_print",    py_hexchat_emit_print,    METH_VARARGS, "Emit a print event"},
    {"get_info",      py_hexchat_get_info,      METH_VARARGS, "Get info by name"},
    {"hook_command",  (PyCFunction)py_hexchat_hook_command,  METH_VARARGS | METH_KEYWORDS, "Hook a command"},
    {"hook_server",   (PyCFunction)py_hexchat_hook_server,   METH_VARARGS | METH_KEYWORDS, "Hook a server event"},
    {"hook_print",    (PyCFunction)py_hexchat_hook_print,    METH_VARARGS | METH_KEYWORDS, "Hook a print event"},
    {"hook_timer",    (PyCFunction)py_hexchat_hook_timer,    METH_VARARGS | METH_KEYWORDS, "Hook a timer"},
    {"unhook",        py_hexchat_unhook,        METH_VARARGS, "Remove a hook"},
    {"nickcmp",       py_hexchat_nickcmp,       METH_VARARGS, "Compare nicks (IRC-aware)"},
    {"get_list",      py_hexchat_get_list,      METH_VARARGS, "Get list"},
    {"get_context",   py_hexchat_get_context,   METH_NOARGS,  "Get current context"},
    {"find_context",  (PyCFunction)py_hexchat_find_context,  METH_VARARGS | METH_KEYWORDS, "Find a context"},
    {"set_context",   py_hexchat_set_context,   METH_VARARGS, "Set current context"},
    {"get_prefs",     py_hexchat_get_prefs,     METH_VARARGS, "Get preference value"},
    {nullptr, nullptr, 0, nullptr}
};

static struct PyModuleDef hexchat_module = {
    PyModuleDef_HEAD_INIT,
    "hexchat",
    "HexChat-compatible IRC scripting API for NUchat",
    -1,
    hexchat_methods
};

static PyObject *PyInit_hexchat(void)
{
    PyObject *m = PyModule_Create(&hexchat_module);
    if (!m) return nullptr;

    // Constants — mirroring HexChat's Python interface
    PyModule_AddIntConstant(m, "EAT_NONE",    EAT_NONE);
    PyModule_AddIntConstant(m, "EAT_HEXCHAT", EAT_HEXCHAT);
    PyModule_AddIntConstant(m, "EAT_XCHAT",   EAT_HEXCHAT);  // compat alias
    PyModule_AddIntConstant(m, "EAT_PLUGIN",  EAT_PLUGIN);
    PyModule_AddIntConstant(m, "EAT_ALL",     EAT_ALL);

    PyModule_AddIntConstant(m, "PRI_HIGHEST", PRI_HIGHEST);
    PyModule_AddIntConstant(m, "PRI_HIGH",    PRI_HIGH);
    PyModule_AddIntConstant(m, "PRI_NORM",    PRI_NORM);
    PyModule_AddIntConstant(m, "PRI_LOW",     PRI_LOW);
    PyModule_AddIntConstant(m, "PRI_LOWEST",  PRI_LOWEST);

    return m;
}

// Also register as "xchat" for legacy script compat
static struct PyModuleDef xchat_module = {
    PyModuleDef_HEAD_INIT,
    "xchat",
    "Legacy XChat-compatible scripting API (alias for hexchat)",
    -1,
    hexchat_methods
};

static PyObject *PyInit_xchat(void)
{
    PyObject *m = PyModule_Create(&xchat_module);
    if (!m) return nullptr;
    PyModule_AddIntConstant(m, "EAT_NONE",    EAT_NONE);
    PyModule_AddIntConstant(m, "EAT_HEXCHAT", EAT_HEXCHAT);
    PyModule_AddIntConstant(m, "EAT_XCHAT",   EAT_HEXCHAT);
    PyModule_AddIntConstant(m, "EAT_PLUGIN",  EAT_PLUGIN);
    PyModule_AddIntConstant(m, "EAT_ALL",     EAT_ALL);
    PyModule_AddIntConstant(m, "PRI_HIGHEST", PRI_HIGHEST);
    PyModule_AddIntConstant(m, "PRI_HIGH",    PRI_HIGH);
    PyModule_AddIntConstant(m, "PRI_NORM",    PRI_NORM);
    PyModule_AddIntConstant(m, "PRI_LOW",     PRI_LOW);
    PyModule_AddIntConstant(m, "PRI_LOWEST",  PRI_LOWEST);
    return m;
}


// ──────────────────────────────────────────────────────────────
//  PythonScriptEngine implementation
// ──────────────────────────────────────────────────────────────

PythonScriptEngine::PythonScriptEngine(IRCConnectionManager *mgr, QObject *parent)
    : QObject(parent), m_mgr(mgr)
{
    s_instance = this;
}

PythonScriptEngine::~PythonScriptEngine()
{
    shutdownPython();
    s_instance = nullptr;
}

PythonScriptEngine *PythonScriptEngine::instance()
{
    return s_instance;
}

void PythonScriptEngine::initPython()
{
    if (m_pyInited) return;

    // Register our built-in modules before Py_Initialize
    PyImport_AppendInittab("hexchat", &PyInit_hexchat);
    PyImport_AppendInittab("xchat",   &PyInit_xchat);

    Py_Initialize();
    m_pyInited = true;

    qDebug() << "[PythonScriptEngine] Python" << Py_GetVersion() << "initialized";
}

void PythonScriptEngine::shutdownPython()
{
    if (!m_pyInited) return;

    // Clean up hooks
    for (auto &hook : m_hooks) {
        if (hook.timer) {
            hook.timer->stop();
            delete hook.timer;
        }
        Py_XDECREF((PyObject*)hook.pyCallback);
        Py_XDECREF((PyObject*)hook.pyUserdata);
    }
    m_hooks.clear();

    // Don't call Py_Finalize - it can cause crashes during app shutdown
    m_pyInited = false;
}

void PythonScriptEngine::loadScripts(const QString &directory)
{
    m_directory = directory;

    // Ensure the scripts directory exists
    QDir dir(directory);
    if (!dir.exists()) {
        dir.mkpath(".");
        qDebug() << "[PythonScriptEngine] Created scripts dir:" << directory;
    }

    initPython();

    // Add scripts directory to Python's sys.path
    QString addPath = QStringLiteral(
        "import sys\n"
        "script_dir = '%1'\n"
        "if script_dir not in sys.path:\n"
        "    sys.path.insert(0, script_dir)\n"
    ).arg(directory);
    PyRun_SimpleString(addPath.toUtf8().constData());

    // Set up file watcher for hot reload
    m_watcher = new QFileSystemWatcher(this);
    m_watcher->addPath(directory);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &PythonScriptEngine::onFileChanged);

    // Load all *.py files
    int loaded = 0;
    for (const QString &file : dir.entryList({"*.py"}, QDir::Files)) {
        loadSingleScript(dir.filePath(file));
        loaded++;
    }

    qDebug() << "[PythonScriptEngine] Loaded" << loaded << "Python scripts from" << directory;
}

void PythonScriptEngine::loadSingleScript(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "[PythonScriptEngine] Cannot open:" << path;
        return;
    }

    QByteArray code = f.readAll();
    f.close();

    QString filename = QFileInfo(path).fileName();
    qDebug() << "[PythonScriptEngine] Loading:" << filename;

    // Compile and run
    PyObject *compiled = Py_CompileString(code.constData(), filename.toUtf8().constData(), Py_file_input);
    if (!compiled) {
        PyErr_Print();
        return;
    }

    PyObject *mainModule = PyImport_AddModule("__main__");
    PyObject *globalDict = PyModule_GetDict(mainModule);

    // Create script-specific namespace inheriting from __main__
    PyObject *localDict = PyDict_Copy(globalDict);
    PyDict_SetItemString(localDict, "__file__", PyUnicode_FromString(path.toUtf8().constData()));
    PyDict_SetItemString(localDict, "__name__", PyUnicode_FromString(filename.toUtf8().constData()));

    // Track hooks registered by this script
    int hooksBefore = m_nextHookId;

    PyObject *result = PyEval_EvalCode(compiled, localDict, localDict);
    if (!result) {
        PyErr_Print();
    } else {
        // Extract module info
        QString info;
        PyObject *pyName = PyDict_GetItemString(localDict, "__module_name__");
        PyObject *pyVer  = PyDict_GetItemString(localDict, "__module_version__");
        PyObject *pyDesc = PyDict_GetItemString(localDict, "__module_description__");
        if (pyName && PyUnicode_Check(pyName))
            info += QString::fromUtf8(PyUnicode_AsUTF8(pyName));
        if (pyVer && PyUnicode_Check(pyVer))
            info += " v" + QString::fromUtf8(PyUnicode_AsUTF8(pyVer));
        if (pyDesc && PyUnicode_Check(pyDesc))
            info += " — " + QString::fromUtf8(PyUnicode_AsUTF8(pyDesc));
        if (info.isEmpty()) info = filename;
        m_scriptInfo[filename] = info;
    }
    Py_XDECREF(result);
    Py_DECREF(localDict);
    Py_DECREF(compiled);

    // Track loaded script
    if (!m_loadedScripts.contains(filename)) {
        m_loadedScripts.append(filename);
        emit loadedScriptsChanged();
    }

    // Watch for changes
    if (m_watcher && !m_watcher->files().contains(path))
        m_watcher->addPath(path);
}

void PythonScriptEngine::onFileChanged(const QString &path)
{
    if (!path.endsWith(".py")) return;
    qDebug() << "[PythonScriptEngine] Reloading changed script:" << path;
    loadSingleScript(path);
    // Re-add to watcher (some editors remove+recreate files)
    if (m_watcher && !m_watcher->files().contains(path))
        m_watcher->addPath(path);
}

// ──────────────────────────────────────────────────────────────
//  Hook dispatch
// ──────────────────────────────────────────────────────────────

bool PythonScriptEngine::handleServerLine(IrcConnection * /*conn*/, const QString &rawLine)
{
    if (!m_pyInited) return false;

    // Parse the raw line into parts for hooks
    // Format: :prefix COMMAND param1 param2 ... :trailing
    QStringList parts = rawLine.split(' ');
    if (parts.isEmpty()) return false;

    // Determine the command/numeric (element 1 if prefix starts with ':', else element 0)
    QString cmd;
    if (parts[0].startsWith(':') && parts.size() > 1)
        cmd = parts[1].toUpper();
    else
        cmd = parts[0].toUpper();

    // Find matching server hooks
    bool eaten = false;
    for (const auto &hook : m_hooks) {
        if (hook.type != PyHook::Server) continue;
        if (hook.name != cmd && hook.name != "RAW LINE") continue;

        PyObject *word, *word_eol;
        buildWordLists(parts, &word, &word_eol);
        int ret = callPyHook((PyObject*)hook.pyCallback, word, word_eol, (PyObject*)hook.pyUserdata);
        Py_DECREF(word);
        Py_DECREF(word_eol);

        if (ret >= EAT_HEXCHAT) { eaten = true; break; }
    }
    return eaten;
}

bool PythonScriptEngine::handleCommand(const QString &command, const QStringList &args)
{
    if (!m_pyInited) return false;

    QString upper = command.toUpper();

    for (const auto &hook : m_hooks) {
        if (hook.type != PyHook::Command) continue;
        if (hook.name != upper) continue;

        // Build word list: [0]="" [1]=command [2..]=args
        QStringList allParts;
        allParts << command;
        allParts << args;

        PyObject *word, *word_eol;
        buildWordLists(allParts, &word, &word_eol);
        int ret = callPyHook((PyObject*)hook.pyCallback, word, word_eol, (PyObject*)hook.pyUserdata);
        Py_DECREF(word);
        Py_DECREF(word_eol);

        if (ret >= EAT_HEXCHAT) return true;
    }
    return false;
}

bool PythonScriptEngine::handlePrintEvent(const QString &event, const QStringList &args)
{
    if (!m_pyInited) return false;

    for (const auto &hook : m_hooks) {
        if (hook.type != PyHook::Print) continue;
        if (hook.name != event) continue;

        PyObject *word, *word_eol;
        buildWordLists(args, &word, &word_eol);
        int ret = callPyHook((PyObject*)hook.pyCallback, word, word_eol, (PyObject*)hook.pyUserdata);
        Py_DECREF(word);
        Py_DECREF(word_eol);

        if (ret >= EAT_HEXCHAT) return true;
    }
    return false;
}

// ──────────────────────────────────────────────────────────────
//  HexChat API implementation
// ──────────────────────────────────────────────────────────────

void PythonScriptEngine::command(const QString &cmd)
{
    if (!m_mgr) return;

    // Parse as if the user typed /cmd
    // If it starts with a known command, route through sendMessage
    QString trimmed = cmd.trimmed();
    if (trimmed.isEmpty()) return;

    // If it doesn't start with /, treat as raw command
    if (!trimmed.startsWith('/'))
        trimmed = "/" + trimmed;

    // Route through the manager's sendMessage which handles /commands
    m_mgr->sendMessage("", trimmed);
}

void PythonScriptEngine::prnt(const QString &text)
{
    if (!m_mgr) return;
    // Show text locally in the active channel
    // Access the message model through the manager
    // We emit it as a "system" message to the current view
    m_mgr->sendMessage("", "/ECHO " + text);
}

QString PythonScriptEngine::getInfo(const QString &id)
{
    if (!m_mgr) return {};

    if (id == "channel") {
        // Return active channel
        IrcConnection *conn = m_mgr->activeConnection();
        Q_UNUSED(conn);
        // We need access to activeChannel — use the Q_INVOKABLE or property
        // For now return what we can
        return {};
    }
    if (id == "network" || id == "server" || id == "host") {
        IrcConnection *conn = m_mgr->activeConnection();
        if (conn) return conn->serverHost();
        return {};
    }
    if (id == "nick" || id == "nickname") {
        return m_mgr->currentNick();
    }
    if (id == "topic") {
        return m_mgr->channelTopic();
    }
    if (id == "version") {
        return QCoreApplication::applicationVersion();
    }
    if (id == "away") {
        return {};  // TODO: track away state
    }
    if (id == "event_text") {
        return {};
    }
    return {};
}

int PythonScriptEngine::hookCommand(const QString &name, void *callback, void *userdata, int priority)
{
    PyHook hook;
    hook.type = PyHook::Command;
    hook.name = name;
    hook.pyCallback = callback;
    hook.pyUserdata = userdata;
    hook.priority = priority;
    hook.timer = nullptr;
    hook.id = m_nextHookId++;
    m_hooks.append(hook);
    qDebug() << "[PythonScriptEngine] Hooked command:" << name << "id:" << hook.id;
    return hook.id;
}

int PythonScriptEngine::hookServer(const QString &name, void *callback, void *userdata, int priority)
{
    PyHook hook;
    hook.type = PyHook::Server;
    hook.name = name;
    hook.pyCallback = callback;
    hook.pyUserdata = userdata;
    hook.priority = priority;
    hook.timer = nullptr;
    hook.id = m_nextHookId++;
    m_hooks.append(hook);
    qDebug() << "[PythonScriptEngine] Hooked server event:" << name << "id:" << hook.id;
    return hook.id;
}

int PythonScriptEngine::hookPrint(const QString &name, void *callback, void *userdata, int priority)
{
    PyHook hook;
    hook.type = PyHook::Print;
    hook.name = name;
    hook.pyCallback = callback;
    hook.pyUserdata = userdata;
    hook.priority = priority;
    hook.timer = nullptr;
    hook.id = m_nextHookId++;
    m_hooks.append(hook);
    qDebug() << "[PythonScriptEngine] Hooked print event:" << name << "id:" << hook.id;
    return hook.id;
}

int PythonScriptEngine::hookTimer(int timeout, void *callback, void *userdata)
{
    PyHook hook;
    hook.type = PyHook::Timer;
    hook.name = "TIMER";
    hook.pyCallback = callback;
    hook.pyUserdata = userdata;
    hook.priority = PRI_NORM;
    hook.id = m_nextHookId++;

    QTimer *timer = new QTimer(this);
    timer->setInterval(timeout);
    int hookId = hook.id;
    connect(timer, &QTimer::timeout, this, [this, hookId]() {
        // Find the hook
        for (int i = 0; i < m_hooks.size(); ++i) {
            if (m_hooks[i].id == hookId) {
                PyObject *args = PyTuple_Pack(1, m_hooks[i].pyUserdata ? (PyObject*)m_hooks[i].pyUserdata : Py_None);
                PyObject *result = PyObject_CallObject((PyObject*)m_hooks[i].pyCallback, args);
                Py_XDECREF(args);

                bool keepGoing = true;
                if (result) {
                    if (PyLong_Check(result) && PyLong_AsLong(result) == 0)
                        keepGoing = false;
                    else if (result == Py_False)
                        keepGoing = false;
                    Py_DECREF(result);
                } else {
                    PyErr_Print();
                    keepGoing = false;
                }

                if (!keepGoing) {
                    // Remove the hook
                    m_hooks[i].timer->stop();
                    delete m_hooks[i].timer;
                    Py_XDECREF((PyObject*)m_hooks[i].pyCallback);
                    Py_XDECREF((PyObject*)m_hooks[i].pyUserdata);
                    m_hooks.removeAt(i);
                }
                break;
            }
        }
    });
    timer->start();
    hook.timer = timer;

    m_hooks.append(hook);
    qDebug() << "[PythonScriptEngine] Hooked timer:" << timeout << "ms, id:" << hook.id;
    return hook.id;
}

void PythonScriptEngine::unhook(int hookId)
{
    for (int i = 0; i < m_hooks.size(); ++i) {
        if (m_hooks[i].id == hookId) {
            if (m_hooks[i].timer) {
                m_hooks[i].timer->stop();
                delete m_hooks[i].timer;
            }
            Py_XDECREF((PyObject*)m_hooks[i].pyCallback);
            Py_XDECREF((PyObject*)m_hooks[i].pyUserdata);
            m_hooks.removeAt(i);
            qDebug() << "[PythonScriptEngine] Unhooked id:" << hookId;
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────
//  QML-accessible script management
// ──────────────────────────────────────────────────────────────

void PythonScriptEngine::loadScript(const QString &path)
{
    if (!m_pyInited) initPython();

    QString fullPath = path;
    // If just a filename, prepend scripts directory
    if (!path.contains('/') && !path.contains('\\'))
        fullPath = m_directory + "/" + path;

    QFileInfo fi(fullPath);
    if (!fi.exists()) {
        emit scriptMessage("File not found: " + fullPath);
        return;
    }

    loadSingleScript(fullPath);
    emit scriptMessage("Loaded: " + fi.fileName());
}

void PythonScriptEngine::unloadScript(const QString &filename)
{
    if (!m_loadedScripts.contains(filename)) {
        emit scriptMessage("Script not loaded: " + filename);
        return;
    }

    // Remove all hooks whose callback was defined in this script file
    for (int i = m_hooks.size() - 1; i >= 0; --i) {
        PyObject *cb = (PyObject*)m_hooks[i].pyCallback;
        if (cb && PyFunction_Check(cb)) {
            PyObject *code = PyFunction_GetCode(cb);
            if (code) {
                PyObject *pyFilename = PyObject_GetAttrString(code, "co_filename");
                if (pyFilename && PyUnicode_Check(pyFilename)) {
                    const char *fn = PyUnicode_AsUTF8(pyFilename);
                    if (fn && QString::fromUtf8(fn) == filename) {
                        if (m_hooks[i].timer) {
                            m_hooks[i].timer->stop();
                            delete m_hooks[i].timer;
                        }
                        Py_XDECREF((PyObject*)m_hooks[i].pyCallback);
                        Py_XDECREF((PyObject*)m_hooks[i].pyUserdata);
                        m_hooks.removeAt(i);
                    }
                }
                Py_XDECREF(pyFilename);
            }
        }
    }

    m_loadedScripts.removeAll(filename);
    m_scriptInfo.remove(filename);
    emit loadedScriptsChanged();
    emit scriptMessage("Unloaded: " + filename);
    qDebug() << "[PythonScriptEngine] Unloaded:" << filename;
}

void PythonScriptEngine::reloadScript(const QString &filename)
{
    QString fullPath = m_directory + "/" + filename;
    if (!QFileInfo::exists(fullPath)) {
        emit scriptMessage("File not found: " + fullPath);
        return;
    }

    // Unload hooks then reload
    unloadScript(filename);
    loadSingleScript(fullPath);
    emit scriptMessage("Reloaded: " + filename);
}

void PythonScriptEngine::reloadAll()
{
    QStringList scripts = m_loadedScripts;
    for (const QString &s : scripts) {
        reloadScript(s);
    }
    emit scriptMessage("Reloaded all scripts");
}

void PythonScriptEngine::openScriptsFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_directory));
}

QString PythonScriptEngine::scriptInfo(const QString &filename) const
{
    return m_scriptInfo.value(filename, filename);
}
