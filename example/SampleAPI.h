/*******************************************************************************
 * QRESTServer a lean and mean Qt/C++ based REST server                        *
 *                                                                             *
 * Copyright 2018 by Targoman Intelligent Processing Co Pjc.<http://tip.co.ir> *
 *                                                                             *
 *                                                                             *
 * QRESTServer is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU AFFERO GENERAL PUBLIC LICENSE as published by *
 * the Free Software Foundation, either version 3 of the License, or           *
 * (at your option) any later version.                                         *
 *                                                                             *
 * QRESTServer is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU AFFERO GENERAL PUBLIC LICENSE for more details.                         *
 * You should have received a copy of the GNU AFFERO GENERAL PUBLIC LICENSE    *
 * along with QRESTServer. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                             *
 *******************************************************************************/
/**
 * @author S.Mehran M.Ziabary <ziabary@targoman.com>
 */

#ifndef SAMPLEAPI_H
#define SAMPLEAPI_H

#include "QHttp/intfRESTAPIHolder.h"


namespace ns {
class SampleAPI : public QHttp::intfRESTAPIHolder
{
    Q_OBJECT
public:
    void init();

private slots:
    int API(GET, SampleData,(),
            "Sample API")
    CACHEABLE_1H QHttp::COOKIES_t API(GET, SampleDataWithCookie, (QHttp::COOKIES_t _COOKIES),
                                      "Sample cacheable API for 1 hour")
    QHttp::HEADERS_t API(GET, SampleDataWithHeaders, (QHttp::HEADERS_t _HEADERS),
                         "Sample API with header")
    QString API(GET, SampleDataReturningJWT, (),
                "Sample API with returning string")
    QHttp::EncodedJWT_t ASYNC_API(GET, SampleDataWithJWT, (QHttp::JWT_t _JWT),
                                  "Sample AsyncAPI returning JWT as encoded")
    int API(PUT,SampleData, (quint64 _id, const QString& _info = "defaultValue"),
            "Sample API with data")
    int API(DELETE, SampleData, (quint64 _id = 5),
            "Sample API for delete")
    QVariantList API(UPDATE,SampleData, (quint64 _id = ',', const QString& _info = "df\",dsf"),
                     "Sample API for Update")


    QHttp::stuTable API(, Translate, (const QString& _text="dfdfjk,", QString _info = ","),
                        "Sample complex API")
    QHttp::stuTable API(, SampleList, (const QVariantList& _list),
                        "Sample list returning API")
    QString API(WS,Sample, (const QString _value),
                "Sample web socket API")

private:
    SampleAPI();
    TARGOMAN_DEFINE_SINGLETON_MODULE(SampleAPI);
};


class SampleSubModule : public QHttp::intfRESTAPIHolder{
    Q_OBJECT
public:
    void init();

private slots:
    int API(GET, ,(),
            "Sample API without name")

private:
    SampleSubModule();
    TARGOMAN_DEFINE_SINGLETON_SUBMODULE(SampleAPI, SampleSubModule);
};
}

#endif // SAMPLEAPI_H
