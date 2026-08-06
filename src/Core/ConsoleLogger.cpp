#include "Core/ConsoleLogger.h"
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QTextStream>

#ifndef QT_NO_DEBUG
static void writeToFile(const QString& level, const std::string& tag, const std::string& message)
{
    QFile logFile(QStringLiteral("home_control.log"));
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        QTextStream stream(&logFile);
        QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
        stream << "[" << timestamp << "][" << level << "][" << QString::fromStdString(tag) << "] " 
               << QString::fromStdString(message) << "\n";
        logFile.close();
    }
}
#endif

void ConsoleLogger::info(const std::string& tag, const std::string& message) {
    qInfo().noquote() << "[INFO][" << QString::fromStdString(tag) << "]" << QString::fromStdString(message);
#ifndef QT_NO_DEBUG
    writeToFile(QStringLiteral("INFO"), tag, message);
#endif
}

void ConsoleLogger::debug(const std::string& tag, const std::string& message) {
    qDebug().noquote() << "[DEBUG][" << QString::fromStdString(tag) << "]" << QString::fromStdString(message);
#ifndef QT_NO_DEBUG
    writeToFile(QStringLiteral("DEBUG"), tag, message);
#endif
}

void ConsoleLogger::warn(const std::string& tag, const std::string& message) {
    qWarning().noquote() << "[WARN][" << QString::fromStdString(tag) << "]" << QString::fromStdString(message);
#ifndef QT_NO_DEBUG
    writeToFile(QStringLiteral("WARN"), tag, message);
#endif
}

void ConsoleLogger::error(const std::string& tag, const std::string& message) {
    qCritical().noquote() << "[ERROR][" << QString::fromStdString(tag) << "]" << QString::fromStdString(message);
#ifndef QT_NO_DEBUG
    writeToFile(QStringLiteral("ERROR"), tag, message);
#endif
}
