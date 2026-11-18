#include "tag-control/ControlParsers.h"

#include <QRegularExpression>

namespace TagOS::Control {
namespace {

QStringList lines(const QString &text)
{
    QStringList out;
    for (const QString &line : text.split(QLatin1Char('\n'))) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty())
            out.append(trimmed);
    }
    return out;
}

} // namespace

WifiState parseNmcliWifiList(const QString &text)
{
    WifiState state;
    for (const QString &line : lines(text)) {
        // nmcli terse mode separates fields with ':' and escapes ':' inside them.
        const QStringList fields = line.split(QLatin1Char(':'));
        if (fields.size() < 2)
            continue;
        const QString inUse = fields.at(0).trimmed();
        if (inUse != QLatin1String("*") && inUse.compare(QLatin1String("yes"), Qt::CaseInsensitive) != 0)
            continue;

        state.valid = true;
        state.connected = true;
        state.ssid = fields.value(1).trimmed();
        state.strength = fields.value(2).trimmed().toInt();
        state.security = fields.value(3).trimmed();
        return state;
    }
    // Terse output with no active row still proves the parser ran.
    state.valid = !text.trimmed().isEmpty() || text.isEmpty();
    return state;
}

bool parseNmcliRadio(const QString &text, bool *ok)
{
    const QString value = text.trimmed().toLower();
    if (ok)
        *ok = value == QLatin1String("enabled") || value == QLatin1String("disabled");
    return value == QLatin1String("enabled");
}

AudioState parseWpctlVolume(const QString &text)
{
    AudioState state;
    static const QRegularExpression re(
        QStringLiteral(R"(Volume:\s*([0-9]*\.?[0-9]+))"));
    const auto match = re.match(text);
    if (!match.hasMatch())
        return state;

    state.valid = true;
    state.volume = qRound(match.captured(1).toDouble() * 100.0);
    state.volume = qBound(0, state.volume, 150);
    state.muted = text.contains(QLatin1String("MUTED"), Qt::CaseInsensitive);
    return state;
}

bool parseBluetoothPowered(const QString &text, bool *ok)
{
    static const QRegularExpression powered(QStringLiteral(R"(Powered:\s*(yes|no))"),
                                            QRegularExpression::CaseInsensitiveOption);
    auto match = powered.match(text);
    if (match.hasMatch()) {
        if (ok)
            *ok = true;
        return match.captured(1).compare(QLatin1String("yes"), Qt::CaseInsensitive) == 0;
    }
    static const QRegularExpression powerState(QStringLiteral(R"(PowerState:\s*(on|off))"),
                                               QRegularExpression::CaseInsensitiveOption);
    match = powerState.match(text);
    if (ok)
        *ok = match.hasMatch();
    return match.hasMatch() && match.captured(1).compare(QLatin1String("on"), Qt::CaseInsensitive) == 0;
}

BatteryState parseUpowerBattery(const QString &text)
{
    BatteryState state;
    static const QRegularExpression percent(QStringLiteral(R"((percentage|capacity):\s*([0-9]+)%)"),
                                            QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression stateRe(QStringLiteral(R"(^\s*state:\s*(\w+))"),
                                            QRegularExpression::CaseInsensitiveOption
                                                | QRegularExpression::MultilineOption);
    static const QRegularExpression present(QStringLiteral(R"(present:\s*(yes|no))"),
                                            QRegularExpression::CaseInsensitiveOption);

    const auto presentMatch = present.match(text);
    if (presentMatch.hasMatch() && presentMatch.captured(1).compare(QLatin1String("no"), Qt::CaseInsensitive) == 0)
        return state;

    const auto percentMatch = percent.match(text);
    const auto stateMatch = stateRe.match(text);
    if (!percentMatch.hasMatch())
        return state;

    state.valid = true;
    state.present = true;
    state.percent = qBound(0, percentMatch.captured(2).toInt(), 100);
    if (stateMatch.hasMatch())
        state.state = stateMatch.captured(1);
    return state;
}

BrightnessState parseBrightnessctlInfo(const QString &text)
{
    BrightnessState state;
    // `brightnessctl -m info` -> device,class,current,percentage,max
    const QStringList fields = text.trimmed().split(QLatin1Char(','));
    if (fields.size() >= 4) {
        bool currentOk = false;
        bool maxOk = false;
        const int current = fields.at(fields.size() >= 5 ? 2 : 1).toInt(&currentOk);
        const int maximum = fields.last().toInt(&maxOk);
        if (currentOk && maxOk && maximum > 0) {
            state.valid = true;
            state.current = current;
            state.maximum = maximum;
            return state;
        }
    }
    return state;
}

ProfileState parsePowerProfiles(const QString &text)
{
    ProfileState state;
    static const QRegularExpression item(QStringLiteral(R"(^([a-z-]+):$)"));
    static const QStringList known = {QStringLiteral("performance"), QStringLiteral("balanced"),
                                      QStringLiteral("power-saver")};

    for (const QString &raw : text.split(QLatin1Char('\n'))) {
        QString line = raw.trimmed();
        if (line.isEmpty())
            continue;
        const bool active = line.startsWith(QLatin1Char('*'));
        if (active)
            line = line.mid(1).trimmed();

        const auto match = item.match(line);
        if (!match.hasMatch())
            continue;
        const QString name = match.captured(1);
        if (!known.contains(name))
            continue;
        if (!state.available.contains(name))
            state.available.append(name);
        if (active)
            state.active = name;
    }

    state.valid = !state.available.isEmpty();
    return state;
}

bool parseRfkillSoftBlocked(const QString &text, const QString &type, bool *ok)
{
    // rfkill list output:
    //   0: phy0: Wireless LAN
    //           Soft blocked: no
    //           Hard blocked: no
    const QStringList blocks = text.split(QRegularExpression(QStringLiteral(R"(\n\s*\n)")),
                                          Qt::SkipEmptyParts);
    for (const QString &block : blocks) {
        const QString lower = block.toLower();
        const bool isWifi = lower.contains(QLatin1String("wireless lan")) || lower.contains(QLatin1String("wlan"));
        const bool isBt = lower.contains(QLatin1String("bluetooth"));
        const bool matchesType = (type == QLatin1String("wifi") && isWifi) || (type == QLatin1String("bluetooth") && isBt);
        if (!matchesType)
            continue;
        if (ok)
            *ok = true;
        return lower.contains(QLatin1String("soft blocked: yes"));
    }
    if (ok)
        *ok = false;
    return false;
}

bool parseNmcliHasWifiDevice(const QString &text)
{
    for (const QString &line : lines(text)) {
        const QStringList fields = line.split(QLatin1Char(':'));
        if (fields.size() >= 2 && fields.at(1).trimmed() == QLatin1String("wifi"))
            return true;
    }
    return false;
}

} // namespace TagOS::Control
