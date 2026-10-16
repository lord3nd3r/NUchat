#pragma once

#include <QString>

// ASCII case-fold used for channel and nick identity. This matches the
// existing ChannelKey ordering (Qt::CaseInsensitive), not RFC 1459's
// extra {}|~ ↔ []\^ mapping — servers differ, and CASEMAPPING is not parsed.
inline bool ircSame(const QString &a, const QString &b) {
  return a.compare(b, Qt::CaseInsensitive) == 0;
}

inline QString ircFold(const QString &s) { return s.toLower(); }
