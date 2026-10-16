#pragma once

#include <QString>

class Settings;

// Expand a leading "~" or "~/..." (and "~user" on Unix) to an absolute path.
// Other strings are returned unchanged.
QString expandHomePath(const QString &path);

// Preference dcc/maxSize is megabytes (0 = unlimited). The legacy key
// dcc/maxFileSize is bytes. Missing both keys keeps the 100 MB default.
qint64 dccMaxFileBytes(const Settings *settings);

// dcc/downloadDir with tilde expansion, or ~/Downloads when unset.
QString dccDownloadDirectory(const Settings *settings);
