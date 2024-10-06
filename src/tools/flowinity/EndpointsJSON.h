//
// Created by troplo on 6/10/24.
//

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

#ifndef FLAMESHOT_ENDPOINTSJSON_H
#define FLAMESHOT_ENDPOINTSJSON_H

class EndpointsJSON : public QObject
{
    Q_OBJECT

public:
    explicit EndpointsJSON(QObject* parent = nullptr);
    ~EndpointsJSON() = default;
    void getAPIEndpoint(bool refresh);
    void getAPIFromEndpoints(bool refresh);

signals:
    void endpointOk(QString endpoint);
    void error(QString error);

private:
    QNetworkAccessManager* m_NetworkAM;
    void handleAPIEndpointResponse(QNetworkReply* reply);
};


#endif // FLAMESHOT_ENDPOINTSJSON_H
