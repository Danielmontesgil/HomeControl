#pragma once
#include <QQuickImageProvider>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QImage>
#include <QUrl>
#include <QString>

class HaImageProvider : public QQuickImageProvider
{
public:
    HaImageProvider(const QString& haUrl, const QString& haToken)
        : QQuickImageProvider(QQuickImageProvider::Image), m_haUrl(haUrl), m_haToken(haToken)
    {
        m_haUrl.replace("ws://", "http://");
        m_haUrl.replace("wss://", "https://");
        if (m_haUrl.endsWith("/api/websocket"))
        {
            m_haUrl.replace("/api/websocket", "");
        }
    }

    ~HaImageProvider() override = default;

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override
    {
        QNetworkAccessManager manager;
        QString targetUrl = m_haUrl + "/api/camera_proxy/" + id;
        
        QNetworkRequest request((QUrl(targetUrl)));
        request.setRawHeader("Authorization", "Bearer " + m_haToken.toUtf8());
        
        QNetworkReply* reply = manager.get(request);
        
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        
        QImage image;
        if (reply->error() == QNetworkReply::NoError)
        {
            image.loadFromData(reply->readAll());
        }
        else
        {
            image = QImage(200, 200, QImage::Format_RGB32);
            image.fill(QColor("#ECEFF1"));
        }
        
        if (size)
        {
            *size = image.size();
        }
        
        reply->deleteLater();
        return image;
    }

private:
    QString m_haUrl;
    QString m_haToken;
};
