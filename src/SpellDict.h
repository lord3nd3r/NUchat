#pragma once

#include <QPair>
#include <QString>
#include <QStringList>

// Directories searched for <lang>.dic / <lang>.aff, in order.
// Covers the usual Linux packages, Homebrew on macOS, a dictionary folder
// next to the executable (Windows and macOS bundles), DICPATH, and the
// per-user data directory.
QStringList spellDictionaryRoots();

// First matching dictionary under roots. The language is tried as given
// and with '_' / '-' swapped (en_US and en-US). Empty strings if none.
QPair<QString, QString> locateHunspellDictionary(const QString &lang,
                                                 const QStringList &roots);
