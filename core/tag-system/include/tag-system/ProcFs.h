#pragma once

#include <QString>

#include "tag-system/Metrics.h"

namespace TagOS::System {

/// Reader for the Linux `procfs` interface.
///
/// Every accessor takes its data from `root()` (default `/proc`), which makes
/// the parsers testable against fixture directories on any platform — no
/// mocking, no root, no live machine required.
class ProcFs
{
public:
    explicit ProcFs(QString root = QStringLiteral("/proc"));

    const QString &root() const { return m_root; }

    CpuTimes cpuTimes(bool *ok = nullptr) const;
    MemoryInfo memoryInfo(bool *ok = nullptr) const;
    LoadAverage loadAverage(bool *ok = nullptr) const;
    qint64 uptimeSeconds(bool *ok = nullptr) const;

private:
    QString readFile(const QString &name, bool *ok) const;

    QString m_root;
};

} // namespace TagOS::System
