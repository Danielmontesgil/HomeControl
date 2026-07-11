#pragma once
#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QRunnable>
#include <QThreadPool>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QImage>
#include <QUrl>
#include <QString>
#include <QQuickTextureFactory>

/**
 * @brief Represents an asynchronous image load response running on a worker thread.
 * Performs the HTTP fetch from HA and signals completion to QtQuick.
 */
class HaImageResponse : public QQuickImageResponse, public QRunnable
{
public:
    HaImageResponse(const QString& url, const QString& token)
        : m_url(url), m_token(token)
    {
        setAutoDelete(false); // Managed by QtQuick framework wrapper
    }

    ~HaImageResponse() override = default;

    void run() override
    {
        QNetworkAccessManager manager;
        QNetworkRequest request((QUrl(m_url)));
        if (!m_token.isEmpty())
        {
            request.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
        }

        QNetworkReply* reply = manager.get(request);

        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec(); // Safely blocks the background worker thread, NOT the main GUI event loop

        if (reply->error() == QNetworkReply::NoError)
        {
            m_image.loadFromData(reply->readAll());
        }
        else
        {
            m_image = QImage(200, 200, QImage::Format_RGB32);
            m_image.fill(QColor("#ECEFF1"));
        }

        reply->deleteLater();
        emit finished(); // Notify QtQuick that the texture is ready to be bound
    }

    QQuickTextureFactory* textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

private:
    QString m_url;
    QString m_token;
    QImage m_image;
};

/**
 * @brief Async Image Provider for protected resources like map proxies in QML.
 */
class HaImageProvider : public QQuickAsyncImageProvider
{
public:
    HaImageProvider(const QString& haUrl, const QString& haToken)
        : m_haUrl(haUrl), m_haToken(haToken)
    {
        m_haUrl.replace("ws://", "http://");
        m_haUrl.replace("wss://", "https://");
        if (m_haUrl.endsWith("/api/websocket"))
        {
            m_haUrl.replace("/api/websocket", "");
        }
    }

    ~HaImageProvider() override = default;

    QQuickImageResponse* requestImageResponse(const QString& id, const QSize& requestedSize) override
    {
        Q_UNUSED(requestedSize);
        QString targetUrl = m_haUrl + "/api/camera_proxy/" + id;
        auto* response = new HaImageResponse(targetUrl, m_haToken);
        QThreadPool::globalInstance()->start(response); // Dispatches run() asynchronously
        return response;
    }

private:
    QString m_haUrl;
    QString m_haToken;
};
