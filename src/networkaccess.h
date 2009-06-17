#ifndef NETWORKACCESS_H
#define NETWORKACCESS_H

#include <QtNetwork>

namespace The {
    QNetworkAccessManager* networkAccessManager();
}

class NetworkReply : public QObject {

    Q_OBJECT

public:
    NetworkReply(QNetworkReply* networkReply);

public slots:
    void finished();
    void metaDataChanged();

signals:
    void data(QByteArray);
    void finished(QNetworkReply*);

private:
    QNetworkReply *networkReply;

};


class NetworkAccess : public QObject {

    Q_OBJECT

public:
    NetworkAccess( QObject* parent=0);
    QNetworkReply* simpleGet(QUrl url);
    NetworkReply* get(QUrl url);
    QNetworkReply* syncGet(QUrl url);
    QByteArray syncGetBytes(QUrl url);
    QString syncGetString(QUrl url);

private slots:
    void error(QNetworkReply::NetworkError);
    void syncMetaDataChanged();
    void syncFinished();

private:
    QNetworkReply *networkReply;
    bool working;

};

typedef QPointer<QObject> ObjectPointer;
Q_DECLARE_METATYPE(ObjectPointer)

#endif // NETWORKACCESS_H
