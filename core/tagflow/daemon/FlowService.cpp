#include "FlowService.h"

#include "tag-system/SystemProbe.h"
#include "tagflow/TagFlowTypes.h"

namespace TagOS::Flow {

FlowService::FlowService(QObject *parent)
    : QObject(parent)
{
    // Prime the CPU delta so the first D-Bus client sees a real number.
    TagOS::System::sampleSystem();
    m_snapshot = TagOS::System::sampleSystem();
}

void FlowService::poll()
{
    m_snapshot = TagOS::System::sampleSystem();
    emit SnapshotChanged(toVariantMap(m_snapshot));
}

QVariantMap FlowService::Snapshot() const
{
    return toVariantMap(m_snapshot);
}

QString FlowService::Version() const
{
    return TagOS::System::tagosVersion();
}

} // namespace TagOS::Flow
