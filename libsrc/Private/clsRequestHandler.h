/*******************************************************************************
 * QRESTServer a lean and mean Qt/C++ based REST server                     *
 *                                                                             *
 * Copyright 2018 by Targoman Intelligent Processing Co Pjc.<http://tip.co.ir> *
 *                                                                             *
 *                                                                             *
 * QRESTServer is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU AFFERO GENERAL PUBLIC LICENSE as published by *
 * the Free Software Foundation, either version 3 of the License, or           *
 * (at your option) any later version.                                         *
 *                                                                             *
 * QRESTServer is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU AFFERO GENERAL PUBLIC LICENSE for more details.                         *
 * You should have received a copy of the GNU AFFERO GENERAL PUBLIC LICENSE    *
 * along with QRESTServer. If not, see <http://www.gnu.org/licenses/>.      *
 *                                                                             *
 *******************************************************************************/
/**
 * @author S.Mehran M.Ziabary <ziabary@targoman.com>
 */

#ifndef QHTTP_PRIVATE_CLSREQUESTHANDLER_H
#define QHTTP_PRIVATE_CLSREQUESTHANDLER_H

#include <QTemporaryFile>
#include "QHttp/QHttpServer"
#include "RESTAPIRegistry.h"
#include "Private/Configs.hpp"
#include "3rdParty/multipart-parser/MultipartReader.h"

namespace QHttp {
namespace Private {

class clsRequestHandler;

class clsStatisticsUpdateThread : public QThread{
    Q_OBJECT
private:
    void run() Q_DECL_FINAL {
        QTimer Timer;
        QObject::connect(&Timer, &QTimer::timeout, [](){
            gServerStats.Connections.snapshot(gConfigs.Public.StatisticsInterval);
            gServerStats.Errors.snapshot(gConfigs.Public.StatisticsInterval);
            gServerStats.Success.snapshot(gConfigs.Public.StatisticsInterval);

            for (auto ListIter = gServerStats.APICallsStats.begin ();
                 ListIter != gServerStats.APICallsStats.end ();
                 ++ListIter)
                ListIter->snapshot(gConfigs.Public.StatisticsInterval);
            for (auto ListIter = gServerStats.APICacheStats.begin ();
                 ListIter != gServerStats.APICacheStats.end ();
                 ++ListIter)
                ListIter->snapshot(gConfigs.Public.StatisticsInterval);
        });
        Timer.start(gConfigs.Public.StatisticsInterval * 1000);
        this->exec();
    }
};

class clsMultipartFormDataRequestHandler : public MultipartReader{
public:
    clsMultipartFormDataRequestHandler(clsRequestHandler* _parent, const QByteArray& _marker) :
        pParent(_parent),
        LastWrittenBytes(0){
        this->onPartBegin = clsMultipartFormDataRequestHandler::onPartBegin;
        this->onPartData = clsMultipartFormDataRequestHandler::onPartData;
        this->onPartEnd = clsMultipartFormDataRequestHandler::onPartEnd;
        this->onEnd = clsMultipartFormDataRequestHandler::onDataEnd;

        this->setBoundary(_marker.toStdString());
    }

    size_t feed(const char *_buffer, size_t _len){
        return MultipartReader::feed(_buffer, _len);
    }

private:
    static void onMultiPartBegin(const MultipartHeaders &_headers, void *_userData);
    static void onMultiPartData(const char *_buffer, size_t _size, void *_userData);
    static void onMultiPartEnd(void *_userData);
    static void onDataEnd(void *_userData);

private:
    clsRequestHandler* pParent;
    QScopedPointer<QTemporaryFile> LastTempFile;
    std::string LastMime;
    std::string LastFileName;
    std::string LastStoredItemName;
    std::string LastItemName;
    int         LastWrittenBytes;
    QStringList UploadedFilesInfo;

    friend class clsRequestHandler;
};

class clsRequestHandler :QObject
{
public:
    clsRequestHandler(qhttp::server::QHttpRequest* _req, qhttp::server::QHttpResponse* _res, QObject *_parent = nullptr);
    void process(const QString &_api);
    void findAndCallAPI(const QString &_api);
    void sendError(qhttp::TStatusCode _code, const QString& _message, bool _closeConnection = false);
    void sendResponse(qhttp::TStatusCode _code, QVariant _response);

private:
    QByteArray      RemainingData;
    qhttp::server::QHttpRequest* Request;
    qhttp::server::QHttpResponse *Response;
    QScopedPointer<clsMultipartFormDataRequestHandler> MultipartFormDataHandler;

    friend class clsMultipartFormDataRequestHandler;
};

}
}

#endif // QHTTP_PRIVATE_CLSREQUESTHANDLER_H


