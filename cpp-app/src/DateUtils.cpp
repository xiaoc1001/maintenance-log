#include "DateUtils.h"

#include <QRegularExpression>

namespace DateUtils {

bool isYmd(const QString &text) {
    static const QRegularExpression re(R"(\d{4}-\d{2}-\d{2})");
    return re.match(text.trimmed()).hasMatch();
}

QDate parseYmd(const QString &text) {
    if (!isYmd(text)) {
        return {};
    }
    return QDate::fromString(text.trimmed(), "yyyy-MM-dd");
}

QString dateToIso(const QDate &date) {
    if (!date.isValid()) {
        return {};
    }
    return date.toString("yyyy-MM-dd");
}

QString dateToRoc(const QDate &date) {
    if (!date.isValid()) {
        return {};
    }
    int rocYear = date.year() - 1911;
    return QString("%1.%2.%3")
        .arg(rocYear, 3, 10, QChar('0'))
        .arg(date.month(), 2, 10, QChar('0'))
        .arg(date.day(), 2, 10, QChar('0'));
}

QDate addMonths(const QDate &date, int months) {
    if (!date.isValid()) {
        return {};
    }
    int totalMonths = (date.year() * 12 + (date.month() - 1)) + months;
    int year = totalMonths / 12;
    int month = (totalMonths % 12) + 1;
    int lastDay = QDate(year, month, 1).addMonths(1).addDays(-1).day();
    int day = qMin(date.day(), lastDay);
    return QDate(year, month, day);
}

QDate addOneYear(const QDate &date) {
    return addMonths(date, 12);
}

QString normalizeRocStr(const QString &value) {
    QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    static const QRegularExpression re(R"(^\s*(\d{1,3})[./-](\d{1,2})[./-](\d{1,2})\s*$)");
    auto match = re.match(trimmed);
    if (!match.hasMatch()) {
        return {};
    }
    int year = match.captured(1).toInt();
    int month = match.captured(2).toInt();
    int day = match.captured(3).toInt();
    return QString("%1.%2.%3")
        .arg(year, 3, 10, QChar('0'))
        .arg(month, 2, 10, QChar('0'))
        .arg(day, 2, 10, QChar('0'));
}

QDate rocToAdDate(const QString &rocStr) {
    QString normalized = normalizeRocStr(rocStr);
    if (normalized.isEmpty()) {
        return {};
    }
    const QStringList parts = normalized.split('.');
    if (parts.size() != 3) {
        return {};
    }
    bool okYear = false;
    bool okMonth = false;
    bool okDay = false;
    int year = parts[0].toInt(&okYear);
    int month = parts[1].toInt(&okMonth);
    int day = parts[2].toInt(&okDay);
    if (!okYear || !okMonth || !okDay) {
        return {};
    }
    return QDate(year + 1911, month, day);
}

} // namespace DateUtils
