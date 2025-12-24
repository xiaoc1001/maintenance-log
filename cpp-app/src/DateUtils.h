#pragma once

#include <QDate>
#include <QString>

namespace DateUtils {
bool isYmd(const QString &text);
QDate parseYmd(const QString &text);
QString dateToIso(const QDate &date);
QString dateToRoc(const QDate &date);
QDate addMonths(const QDate &date, int months);
QDate addOneYear(const QDate &date);
QString normalizeRocStr(const QString &value);
QDate rocToAdDate(const QString &rocStr);
} // namespace DateUtils
