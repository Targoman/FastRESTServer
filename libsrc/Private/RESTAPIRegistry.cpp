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

#include <QMap>
#include <QMutex>

#include "Private/RESTAPIRegistry.h"

namespace QHttp {
namespace Private {

/***********************************************************************************************/

#define DO_ON_TYPE(_complexity, _typeName, _baseType) DO_ON_TYPE_PROXY(_baseType, IGNORE_TYPE_##_typeName)
#define DO_ON_TYPE_SELECTOR(_1,_2,_3,N,...) N
#define DO_ON_TYPE_PROXY(_complexity, _type, ...) DO_ON_TYPE_SELECTOR(__VA_ARGS__, DO_ON_TYPE_IGNORED, DO_ON_TYPE_VALID)(_complexity, _type)
#define DO_ON_TYPE_IGNORED(_baseType) nullptr

#define DO_ON_TYPE_VALID(_complexity, _baseType)  new tmplAPIArg<_baseType, _complexity>(TARGOMAN_M2STR(_baseType))
#define MAKE_INFO_FOR_VALID_INTEGRAL_METATYPE(_typeName, _id, _baseType) { _id, { DO_ON_TYPE(Integral, _typeName, _baseType) }},
#define MAKE_INFO_FOR_VALID_COMPLEX_METATYPE(_typeName, _id, _baseType) { _id, { DO_ON_TYPE(Complex, _typeName, _baseType, Complex) }},
#define MAKE_INVALID_METATYPE(_typeName, _id, _baseType) { _id, { nullptr }},

#define IGNORE_TYPE_Void ,
#define IGNORE_TYPE_QByteArray ,
#define IGNORE_TYPE_QBitArray ,
#define IGNORE_TYPE_QLocale ,
#define IGNORE_TYPE_QRect ,
#define IGNORE_TYPE_QRectF ,
#define IGNORE_TYPE_QSize ,
#define IGNORE_TYPE_QSizeF ,
#define IGNORE_TYPE_QLine ,
#define IGNORE_TYPE_QLineF ,
#define IGNORE_TYPE_QPoint ,
#define IGNORE_TYPE_QPointF ,
#define IGNORE_TYPE_QEasingCurve ,
#define IGNORE_TYPE_QModelIndex ,
#define IGNORE_TYPE_QJsonValue ,
#define IGNORE_TYPE_QJsonObject ,
#define IGNORE_TYPE_QJsonArray ,
#define IGNORE_TYPE_QJsonDocument ,
#define IGNORE_TYPE_QPersistentModelIndex ,
#define IGNORE_TYPE_QPersistentModelIndex ,
#define IGNORE_TYPE_Nullptr ,
#define IGNORE_TYPE_QVariantHash ,
#define IGNORE_TYPE_QByteArrayList ,

const QMap<int, intfAPIArgManipulator*> MetaTypeInfoMap = {
    QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(MAKE_INFO_FOR_VALID_INTEGRAL_METATYPE)
    QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(MAKE_INVALID_METATYPE)
    QT_FOR_EACH_STATIC_CORE_CLASS(MAKE_INFO_FOR_VALID_COMPLEX_METATYPE)
    QT_FOR_EACH_STATIC_CORE_POINTER(MAKE_INVALID_METATYPE)
    QT_FOR_EACH_STATIC_CORE_TEMPLATE(MAKE_INFO_FOR_VALID_COMPLEX_METATYPE)
    QT_FOR_EACH_STATIC_GUI_CLASS(MAKE_INVALID_METATYPE)
    QT_FOR_EACH_STATIC_WIDGETS_CLASS(MAKE_INVALID_METATYPE)
};

QList<intfAPIArgManipulator*> gOrderedMetaTypeInfo;
QList<intfAPIArgManipulator*> gUserDefinedTypesInfo;

/***********************************************************************************************/
void RESTAPIRegistry::registerRESTAPI(intfRESTAPIHolder* _module, const QMetaMethod& _method){
    if(gOrderedMetaTypeInfo.isEmpty()){
        gOrderedMetaTypeInfo.reserve(MetaTypeInfoMap.lastKey());

        for(auto MetaTypeInfoMapIter = MetaTypeInfoMap.begin();
            MetaTypeInfoMapIter != MetaTypeInfoMap.end();
            ++MetaTypeInfoMapIter){
            for(int i = 0; i< MetaTypeInfoMapIter.key() - gOrderedMetaTypeInfo.size(); ++i)
                gOrderedMetaTypeInfo.append(nullptr);
            gOrderedMetaTypeInfo.append(MetaTypeInfoMapIter.value());
        }
    }
    if ((_method.name().startsWith("api") == false &&
         _method.name().startsWith("asyncApi") == false)||
        _method.typeName() == nullptr)
        return;

    try{
        RESTAPIRegistry::validateMethodInputAndOutput(_method);
        QString MethodName = _method.name().constData();
        MethodName = MethodName.mid(MethodName.indexOf("api",0,Qt::CaseInsensitive) + 3);

        QString MethodSignature, MethodDoc;
        bool Found = false;
        for (int i=0; i<_module->metaObject()->methodCount(); ++i)
            if(_module->metaObject()->method(i).name() == "signOf" + MethodName){
                _module->metaObject()->method(i).invoke(_module,
                                                        Qt::DirectConnection,
                                                        Q_RETURN_ARG(QString, MethodSignature)
                                                        );
                Found = true;
                break;
            }

        if(!Found)
            throw exRESTRegistry("Seems that you have not use API macro to define your API");

        for (int i=0; i<_module->metaObject()->methodCount(); ++i)
            if(_module->metaObject()->method(i).name() == "docOf" + MethodName){
                _module->metaObject()->method(i).invoke(_module,
                                                        Qt::DirectConnection,
                                                        Q_RETURN_ARG(QString, MethodDoc)
                                                        );
                Found = true;
                break;
            }

        if(!Found)
            throw exRESTRegistry("Seems that you have not use API macro to define your API");


        auto makeMethodName = [MethodName](int _start) -> QString{
            if(MethodName.size() >= _start)
                return MethodName.at(_start-1).toLower() + MethodName.mid(_start);
            return "";
        };

        constexpr char COMMA_SIGN[] = "$_COMMA_$";

        MethodSignature = MethodSignature.trimmed().replace("','", QString("'%1'").arg(COMMA_SIGN));
        bool DQuoteStarted = false, BackslashFound = false;
        QString CommaReplacedMethodSignature;
        for(int i=0; i< MethodSignature.size(); ++i){
            if(DQuoteStarted){
                if(MethodSignature.at(i).toLatin1() == ',')
                    CommaReplacedMethodSignature += COMMA_SIGN;
                else if (MethodSignature.at(i).toLatin1() == '\\') {
                    BackslashFound = true;
                }else if (MethodSignature.at(i).toLatin1() == '"') {
                    CommaReplacedMethodSignature += '"';
                    if(BackslashFound)
                        BackslashFound = false;
                    else
                        DQuoteStarted = false;
                }else
                    CommaReplacedMethodSignature += MethodSignature.at(i);
            }else if (MethodSignature.at(i).toLatin1() == '"'){
                CommaReplacedMethodSignature += '"';
                DQuoteStarted = true;
            }else
                CommaReplacedMethodSignature += MethodSignature.at(i);
        }


        QVariantList DefaultValues;
        quint8 ParamIndex = 0;
        foreach(auto Param, CommaReplacedMethodSignature.split(',')){
            if(Param.contains('=')){
                QString Value = Param.split('=').last().replace(COMMA_SIGN, ",").trimmed();
                if(Value.startsWith("(")) Value = Value.mid(1);
                if(Value.endsWith(")")) Value = Value.mid(0, Value.size() - 1);
                if(Value.startsWith("'")) Value = Value.mid(1);
                if(Value.endsWith("'")) Value = Value.mid(0, Value.size() - 1);
                if(Value.startsWith("\"")) Value = Value.mid(1);
                if(Value.endsWith("\"")) Value = Value.mid(0, Value.size() - 1);
                Value.replace("\\\"", "\"");
                Value.replace("\\'", "'");

                if(Value == "{}" || Value.contains("()"))
                    DefaultValues.append(QVariant());
                else
                    DefaultValues.append(Value);
            }else
                DefaultValues.append(QVariant());
            ParamIndex++;
        }

        QMetaMethodExtended Method(_method, DefaultValues, MethodDoc);

        if(MethodName.startsWith("GET"))
            RESTAPIRegistry::addRegistryEntry(RESTAPIRegistry::Registry, _module, Method, "GET", makeMethodName(sizeof("GET")));
        else if(MethodName.startsWith("POST"))
            RESTAPIRegistry::addRegistryEntry(RESTAPIRegistry::Registry, _module, Method, "POST", makeMethodName(sizeof("POST")));
        else if(MethodName.startsWith("PUT"))
            RESTAPIRegistry::addRegistryEntry(RESTAPIRegistry::Registry, _module, Method, "PUT", makeMethodName(sizeof("PUT")));
        else if(MethodName.startsWith("PATCH"))
            RESTAPIRegistry::addRegistryEntry(RESTAPIRegistry::Registry, _module, Method, "PATCH", makeMethodName(sizeof("PATCH")));
        else if (MethodName.startsWith("DELETE"))
            RESTAPIRegistry::addRegistryEntry(RESTAPIRegistry::Registry, _module, Method, "DELETE", makeMethodName(sizeof("DELETE")));
        else if (MethodName.startsWith("UPDATE"))
            RESTAPIRegistry::addRegistryEntry(RESTAPIRegistry::Registry, _module, Method, "UPDATE", makeMethodName(sizeof("UPDATE")));
        else if (MethodName.startsWith("WS")){
#ifdef QHTTP_ENABLE_WEBSOCKET
            RESTAPIRegistry::addRegistryEntry(RESTAPIRegistry::WSRegistry, _module, Method, "WS", makeMethodName(sizeof("WS")));
#else
            throw exRESTRegistry("Websockets are not enabled in this QRestServer please compile with websockets support");
#endif
        }else{
            RESTAPIRegistry::addRegistryEntry(RESTAPIRegistry::Registry, _module, Method, "GET", MethodName.at(0).toLower() + MethodName.mid(1));
            RESTAPIRegistry::addRegistryEntry(RESTAPIRegistry::Registry, _module, Method, "POST", MethodName.at(0).toLower() + MethodName.mid(1));
        }
    }catch(Targoman::Common::exTargomanBase& ex){
        TargomanError("Fatal Error: "<<ex.what());
        throw;
    }
}

QMap<QString, QString> RESTAPIRegistry::extractMethods(QHash<QString, clsAPIObject*>& _registry, const QString& _module, bool _showTypes, bool _prettifyTypes)
{
    static auto type2Str = [_prettifyTypes](int _typeID) {
        if(_prettifyTypes == false || _typeID > 1023)
            return QString(QMetaType::typeName(_typeID));

        return gOrderedMetaTypeInfo.at(_typeID)->PrettyTypeName;
    };


    QMap<QString, QString> Methods;
    foreach(const QString& Key, _registry.keys()){
        QString Path = Key.split(" ").last();
        if(Path.startsWith(_module) == false)
            continue;

        clsAPIObject* APIObject = _registry.value(Key);
        QStringList Parameters;
        for(quint8 i=0; i< APIObject->BaseMethod.parameterCount(); ++i){

            static auto defaultValue = [APIObject](quint8 i) -> QString{
                if(APIObject->RequiredParamsCount > i)
                    return "";
                intfAPIArgManipulator* ArgManipulator;
                if(APIObject->BaseMethod.parameterType(i) < QHTTP_BASE_USER_DEFINED_TYPEID)
                    ArgManipulator = gOrderedMetaTypeInfo.at(APIObject->BaseMethod.parameterType(i));
                else
                    ArgManipulator = gUserDefinedTypesInfo.at(APIObject->BaseMethod.parameterType(i) - QHTTP_BASE_USER_DEFINED_TYPEID);
                return " = " + (ArgManipulator->isIntegralType() ? QString("%1") : QString("\"%1\"")).arg(APIObject->defaultValue(i).toString());
            };

            Parameters.append((_showTypes ? type2Str(APIObject->BaseMethod.parameterType(i)) + " " : "" ) + (
                                  APIObject->BaseMethod.parameterNames().at(i).startsWith('_') ?
                                      APIObject->BaseMethod.parameterNames().at(i).mid(1) :
                                      APIObject->BaseMethod.parameterNames().at(i)) +
                                      defaultValue(i)
                              );
        }

        Methods.insert(Path + Key.split(" ").first(),
                       QString("%1(%2) %3 %4").arg(
                           Key).arg(
                           Parameters.join(", ")).arg(
                           _showTypes ? QString("-> %1").arg(
                                            type2Str(APIObject->BaseMethod.returnType())) : "").arg(
                           APIObject->IsAsync ? "[async]" : ""
                                                )
                       );
    }

    return Methods;
}

QJsonObject RESTAPIRegistry::retriveOpenAPIJson(){

    QJsonObject OpenAPI = gConfigs.Public.BaseOpenAPIObject;
    QJsonObject DefaultResponses = QJsonObject({
                                                   {"200", QJsonObject({{"description", "Success"}})},
                                                   {"400", QJsonObject({{"description", "Bad request."}})},
                                                   {"401", QJsonObject({{"description", "Authorization information is missing or invalid"}})},
                                                   {"404", QJsonObject({{"description", "Not found"}})},
                                                   {"5XX", QJsonObject({{"description", "Unexpected error"}})},
                                               });

    if(OpenAPI.isEmpty())
        OpenAPI = QJsonObject({
                                  {"swagger","2.0"},
                                  {"info",QJsonObject({
                                       {"version", gConfigs.Public.Version},
                                       {"title", "NOT_SET"},
                                       {"description", ""},
                                       {"contact", QJsonObject({{"email", ""}})}
                                   })},
                                  {"host", QString("127.0.0.1:%1").arg(gConfigs.Public.ListenPort)},
                                  {"components", QJsonObject({
                                       {"securitySchemes", QJsonObject({
                                            {"BearerAuth", QJsonObject({
                                                 {"type", "http"},
                                                 {"scheme", "bearer"},
                                                 {"bearerFormat", "JWT"}
                                             })}
                                        })},
                                       {"responses", QJsonObject({
                                            {"UnauthorizedError", QJsonObject({
                                                 {"description", "Access token is missing or invalid"},
                                             })}
                                        })}
                                   })},
                                  {"securityDefinitions", QJsonObject({
                                       {"Bearer", QJsonObject({
                                            {"type", "apiKey"},
                                            {"name", "Authorization"},
                                            {"in", "header"},
                                            {"description", "For accessing the API a valid JWT token must be passed in all the queries in the 'Authorization' header.\n\n A valid JWT token is generated by the API and retourned as answer of a call     to the route /login giving a valid user & password.\n\nThe following syntax must be used in the 'Authorization' header :\n\nBearer xxxxxx.yyyyyyy.zzzzzz"},
                                        })}
                                   })},
                                  {"security", QJsonArray({
                                       QJsonObject({
                                           {"Bearer", QJsonArray()}
                                       })
                                   })
                                  },
                                  {"basePath", gConfigs.Private.BasePathWithVersion},
                                  {"schemes", QJsonArray({"http", "https"})},
                              });

    if(OpenAPI.value("info").isObject() == false)
        throw exHTTPInternalServerError("Invalid OpenAPI base json");

    QJsonObject PathsObject;
    foreach(const QString& Key, RESTAPIRegistry::Registry.keys()){
        QString PathString = Key.split(" ").last();
        QString HTTPMethod = Key.split(" ").first().toLower();

        clsAPIObject* APIObject = RESTAPIRegistry::Registry.value(Key);
        QJsonArray Parameters;
        for(quint8 i=0; i< APIObject->BaseMethod.parameterCount(); ++i){
            QJsonObject Schema({
                                   {"type",QMetaType::typeName(APIObject->BaseMethod.parameterType(i))}
                               });

            QString ParamName = APIObject->BaseMethod.parameterNames().at(i).startsWith('_') ?
                                    APIObject->BaseMethod.parameterNames().at(i).mid(1) :
                                    APIObject->BaseMethod.parameterNames().at(i);

            if(ParamName == PARAM_COOKIES ||
               ParamName == PARAM_JWT ||
               ParamName == PARAM_HEADERS ||
               ParamName == PARAM_REMOTE_IP ||
               ParamName == PARAM_EXTRAPATH)
                continue;
            Parameters.append(
                        QJsonObject({
                                        {"in", "path"},
                                        {"name", ParamName},
                                        {"required", APIObject->isRequiredParam(i)},
                                        {"description", ""},
//                                        {"type", },
                                        {"schema", QJsonObject({
                                             {"type", QMetaType::typeName(APIObject->BaseMethod.parameterType(i))},
                                         })}
                                    })
                        );
        }
        //TODO add default, enum, nullable, example

        QJsonObject CurrPathMethodInfo = QJsonObject({{"parameters", Parameters}});
        QStringList PathStringParts = PathString.split("/", QString::SkipEmptyParts);
        PathStringParts.pop_back();
        CurrPathMethodInfo["tags"] = QJsonArray({PathStringParts.join("_")});
        CurrPathMethodInfo["produces"] = QJsonArray({"application/json"});
        CurrPathMethodInfo["consumes"] = QJsonArray({"application/json"});
        CurrPathMethodInfo["responses"] = DefaultResponses;


        if(PathsObject.contains(PathString)){
            QJsonObject CurrPathObject = PathsObject.value(PathString).toObject();
            CurrPathObject[HTTPMethod] = CurrPathMethodInfo;
            PathsObject[PathString] = CurrPathObject;
        }else
            PathsObject[PathString] = QJsonObject({
                                                      {HTTPMethod, CurrPathMethodInfo}
                                                  });
    }

    OpenAPI["paths"] = PathsObject;

    return OpenAPI;
}

QStringList RESTAPIRegistry::registeredAPIs(const QString& _module, bool _showParams, bool _showTypes, bool _prettifyTypes){
    if(_showParams == false){
#ifdef QHTTP_ENABLE_WEBSOCKET
        QStringList Methods = RESTAPIRegistry::Registry.keys();
        Methods.append(RESTAPIRegistry::WSRegistry.keys());
        return  Methods;
#else
        return  RESTAPIRegistry::Registry.keys();
#endif
    }

    QMap<QString, QString> Methods = RESTAPIRegistry::extractMethods(RESTAPIRegistry::Registry, _module, _showTypes, _prettifyTypes);
#ifdef QHTTP_ENABLE_WEBSOCKET
    Methods.unite(RESTAPIRegistry::extractMethods(RESTAPIRegistry::WSRegistry, _module, _showTypes, _prettifyTypes));
#endif
    return Methods.values();
}

QString RESTAPIRegistry::isValidType(int _typeID, bool _validate4Input){
    if(_typeID == 0 || _typeID == QMetaType::User || _typeID == 1025)
        return  "is not registered with Qt MetaTypes";
    if(_typeID < QHTTP_BASE_USER_DEFINED_TYPEID && (_typeID >= gOrderedMetaTypeInfo.size() || gOrderedMetaTypeInfo.at(_typeID) == nullptr))
        return "is complex type and not supported";

    if(_typeID >= QHTTP_BASE_USER_DEFINED_TYPEID){
        if((_typeID - QHTTP_BASE_USER_DEFINED_TYPEID >= gUserDefinedTypesInfo.size() ||
            gUserDefinedTypesInfo.at(_typeID - QHTTP_BASE_USER_DEFINED_TYPEID) == nullptr))
            return "is user defined but not registered";

        if(strcmp(gUserDefinedTypesInfo.at(_typeID - QHTTP_BASE_USER_DEFINED_TYPEID)->RealTypeName, QMetaType::typeName(_typeID)))
            return QString("Seems that registration table is corrupted: %1:%2 <-> %3:%4").arg(
                        _typeID).arg(
                        gUserDefinedTypesInfo.at(_typeID - QHTTP_BASE_USER_DEFINED_TYPEID)->RealTypeName).arg(
                        _typeID).arg(
                        QMetaType::typeName(_typeID));

        if(_validate4Input){
            if(!gUserDefinedTypesInfo.at(_typeID - QHTTP_BASE_USER_DEFINED_TYPEID)->hasFromVariantMethod())
                return "has no fromVariant lambda so can not be used as input";
        }else{
            if(!gUserDefinedTypesInfo.at(_typeID - QHTTP_BASE_USER_DEFINED_TYPEID)->hasToVariantMethod())
                return "has no toVariant lambda so can not be used as output";
        }
    }
    return "";
}

void RESTAPIRegistry::validateMethodInputAndOutput(const QMetaMethod& _method){
    if(_method.parameterCount() > 9)
        throw exRESTRegistry("Unable to register methods with more than 9 input args");

    QString ErrMessage;
    if ((ErrMessage = RESTAPIRegistry::isValidType(_method.returnType(), false)).size())
        throw exRESTRegistry(QString("Invalid return type(%1): %2").arg(_method.typeName()).arg(ErrMessage));

    ErrMessage.clear();

    for(int i=0; i<_method.parameterCount(); ++i){
        if ((ErrMessage = RESTAPIRegistry::isValidType(_method.parameterType(i), true)).size())
            throw exRESTRegistry(QString("Invalid parameter (%1 %2): %3").arg(
                                     _method.parameterTypes().at(i).constData()).arg(
                                     _method.parameterNames().at(i).constData()).arg(
                                     ErrMessage));

        for(int j=i+1; j<_method.parameterCount(); ++j){
            if(_method.parameterNames().at(j) == _method.parameterNames().at(i))
                throw exRESTRegistry(QString("Invalid duplicate parameter name %1 at %2 and %3").arg(
                                         _method.parameterNames().at(i).constData()).arg(
                                         i).arg(
                                         j
                                         ));
        }
    }
}

constexpr char CACHE_INTERNAL[] = "CACHEABLE_";
constexpr char CACHE_CENTRAL[]  = "CENTRALCACHE_";

void RESTAPIRegistry::addRegistryEntry(QHash<QString, QHttp::Private::clsAPIObject *>& _registry,
                                       intfRESTAPIHolder* _module,
                                       const QMetaMethodExtended& _method,
                                       const QString& _httpMethod,
                                       const QString& _methodName){
    QString MethodKey = RESTAPIRegistry::makeRESTAPIKey(_httpMethod, "/" + _module->moduleBaseName().replace("::", "/")+ '/' + _methodName);

    if(_registry.contains(MethodKey)){
        if(RESTAPIRegistry::Registry.value(MethodKey)->isPolymorphic(_method))
            throw exRESTRegistry(QString("Polymorphism is not supported: %1").arg(_method.methodSignature().constData()));
        _registry.value(MethodKey)->updateDefaultValues(_method);
    }else{
        if(RESTAPIRegistry::getCacheSeconds(_method, CACHE_INTERNAL) > 0 && RESTAPIRegistry::getCacheSeconds(_method, CACHE_CENTRAL) >0)
            throw exRESTRegistry("Both internal and central cache can not be defined on an API");

        _registry.insert(MethodKey,
                         new clsAPIObject(_module,
                                          _method,
                                          QString(_method.name()).startsWith("async"),
                                          RESTAPIRegistry::getCacheSeconds(_method, CACHE_INTERNAL),
                                          RESTAPIRegistry::getCacheSeconds(_method, CACHE_CENTRAL)
                                          ));
    }
}

int RESTAPIRegistry::getCacheSeconds(const QMetaMethod& _method, const char* _type){
    if(_method.tag() == nullptr || _method.tag()[0] == '\0')
        return 0;
    QString Tag = _method.tag();
    if(Tag.startsWith(_type) && Tag.size () > 12){
        Tag = Tag.mid(10);
        if(_type == CACHE_INTERNAL && Tag == "INF")
            return -1;
        char Type = Tag.rbegin()->toLatin1();
        int  Number = Tag.mid(0,Tag.size() -1).toUShort();
        switch(Type){
        case 'S': return Number;
        case 'M': return Number * 60;
        case 'H': return Number * 3600;
        default:
            throw exRESTRegistry("Invalid CACHE numer or type defined for api: " + _method.methodSignature());
        }
    }
    return 0;
}


Cache_t InternalCache::Cache;
QMutex InternalCache::Lock;

QScopedPointer<intfCacheConnector> CentralCache::Connector;
QHash<QString, clsAPIObject*>  RESTAPIRegistry::Registry;
#ifdef QHTTP_ENABLE_WEBSOCKET
QHash<QString, clsAPIObject*>  RESTAPIRegistry::WSRegistry;
#endif

/****************************************************/
intfCacheConnector::~intfCacheConnector()
{;}

/****************************************************/
clsAPIObject::~clsAPIObject()
{;}

}
}
