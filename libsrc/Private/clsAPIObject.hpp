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
 * @author S. Mohammad M. Ziabary <ziabary@targoman.com>
 */

#ifndef QHTTP_PRIVATE_CLSAPIOBJECT_HPP
#define QHTTP_PRIVATE_CLSAPIOBJECT_HPP

#include <QGenericArgument>
#include <QMetaMethod>

#include "QHttp/intfRESTAPIHolder.h"

#include "Private/Configs.hpp"
#include "Private/RESTAPIRegistry.h"

namespace QHttp {
namespace Private {

class clsAPIObject : public QObject
{
public:
    clsAPIObject(intfRESTAPIHolder* _module, QMetaMethod _method, bool _async, qint32 _cache4) :
        QObject(_module),
        BaseMethod(_method),
        IsAsync(_async),
        Cache4Secs(_cache4),
        RequiredParamsCount(_method.parameterCount())
    {
        foreach(const QByteArray& ParamName, _method.parameterNames())
            this->ParamNames.append(ParamName.startsWith('_') ? ParamName.mid(1) : ParamName);
    }

    inline QVariant invoke(const QStringList& _args) const{
        Q_ASSERT_X(this->parent(), "parent module", "Parent module not found to invoke method");

        if(_args.size() < this->RequiredParamsCount)
            throw exHTTPBadRequest("Not enough arguments");

        QList<QGenericArgument> Arguments;

        qint8 FirstArgumentWithValue = -1;
        qint8 LastArgumentWithValue = -1;

        for(int i=0; i< this->ParamNames.count(); ++i ){
            bool ParamNotFound = true;
            QVariant ArgumentValue;
            foreach (const QString& Arg, _args){
                if(Arg.startsWith(this->ParamNames.at(i)+ '=')){
                    ParamNotFound = false;
                    QString ArgValStr = Arg.mid(Arg.indexOf('='));
                    if((ArgValStr.startsWith('[') && ArgValStr.endsWith(']')) ||
                        (ArgValStr.startsWith('{') && ArgValStr.endsWith('}'))){
                        QJsonParseError Error;
                        QJsonDocument JSON = QJsonDocument::fromJson(ArgValStr.toUtf8(), &Error);
                        if(JSON.isNull())
                            throw exHTTPBadRequest(QString("Invalid value for %1: %2").arg(Arg).arg(Error.errorString()));
                        ArgumentValue = JSON.toVariant();
                    }else{
                        ArgumentValue = ArgValStr;
                    }
                    if(ArgumentValue.isValid() == false)
                        throw exHTTPBadRequest(QString("Invalid value for %1: no conversion to QVariant defined").arg(Arg));
                    break;
                }
            }

            if(ParamNotFound){
                if(i < this->RequiredParamsCount)
                    throw exHTTPBadRequest(QString("Required parameter <%1> not specified").arg(this->ParamNames.at(i).constData()));
                else
                    Arguments.append(QGenericArgument());
                continue;
            }
            if(FirstArgumentWithValue < 0)
                FirstArgumentWithValue = i;
            LastArgumentWithValue = i;

            if(this->BaseMethod.parameterType(i) > 1024){
                //TODO handle user defined types
            }else{
                Q_ASSERT(this->BaseMethod.parameterType(i) < OrderedMetaTypeInfo.size());
                Q_ASSERT(OrderedMetaTypeInfo.at(this->BaseMethod.parameterType(i)).makeGenericArgument != nullptr);

                Arguments.append(OrderedMetaTypeInfo.at(this->BaseMethod.parameterType(i)).makeGenericArgument(
                                     ArgumentValue,
                                     this->BaseMethod.parameterTypes().at(i)));
            }
        }

        if(FirstArgumentWithValue < 0)
            Arguments.clear();
        else if (LastArgumentWithValue < Arguments.size() - 1)
            Arguments = Arguments.mid(0, LastArgumentWithValue + 1);

        if(this->BaseMethod.returnType() > 1024){
            //TODO handle user defined types
            throw Targoman::Common::exTargomanMustBeImplemented();
        }else{
            Q_ASSERT(this->BaseMethod.returnType() < OrderedMetaTypeInfo.size());
            Q_ASSERT(OrderedMetaTypeInfo.at(this->BaseMethod.returnType()).invokeMethod == nullptr);

            return OrderedMetaTypeInfo.at(this->BaseMethod.returnType()).invokeMethod(this, Arguments);
        }
    }

    void invokeMethod(const QList<QGenericArgument>& _arguments, QGenericReturnArgument _returnArg) const{
        bool InvocationResult= true;
        QMetaMethod InvokableMethod;
        if(_arguments.size() == this->ParamNames.size())
            InvokableMethod = this->BaseMethod;
        else
            InvokableMethod = this->LessArgumentMethods.at(this->ParamNames.size() - _arguments.size() - 1);

        switch(_arguments.size()){
        case 0: InvocationResult = InvokableMethod.invoke(this->parent(), this->IsAsync ? Qt::QueuedConnection : Qt::DirectConnection,
                                                       _returnArg);break;
        case 1: InvocationResult = InvokableMethod.invoke(this->parent(), this->IsAsync ? Qt::QueuedConnection : Qt::DirectConnection,
                                                       _returnArg, _arguments.at(0));break;
        case 2: InvocationResult = InvokableMethod.invoke(this->parent(), this->IsAsync ? Qt::QueuedConnection : Qt::DirectConnection,
                                                       _returnArg, _arguments.at(0),_arguments.at(1));break;
        case 3: InvocationResult = InvokableMethod.invoke(this->parent(), this->IsAsync ? Qt::QueuedConnection : Qt::DirectConnection,
                                                       _returnArg, _arguments.at(0),_arguments.at(1),_arguments.at(2));break;
        case 4: InvocationResult = InvokableMethod.invoke(this->parent(), this->IsAsync ? Qt::QueuedConnection : Qt::DirectConnection,
                                                       _returnArg, _arguments.at(0),_arguments.at(1),_arguments.at(2),_arguments.at(3));break;
        case 5: InvocationResult = InvokableMethod.invoke(this->parent(), this->IsAsync ? Qt::QueuedConnection : Qt::DirectConnection,
                                                       _returnArg, _arguments.at(0),_arguments.at(1),_arguments.at(2),_arguments.at(3),_arguments.at(4));break;
        case 6: InvocationResult = InvokableMethod.invoke(this->parent(), this->IsAsync ? Qt::QueuedConnection : Qt::DirectConnection,
                                                       _returnArg,_arguments.at(0),_arguments.at(1),_arguments.at(2),_arguments.at(3),_arguments.at(4),
                                                       _arguments.at(5));break;
        case 7: InvocationResult = InvokableMethod.invoke(this->parent(), this->IsAsync ? Qt::QueuedConnection : Qt::DirectConnection,
                                                       _returnArg, _arguments.at(0),_arguments.at(1),_arguments.at(2),_arguments.at(3),_arguments.at(4),
                                                       _arguments.at(5),_arguments.at(6));break;
        case 8: InvocationResult = InvokableMethod.invoke(this->parent(), this->IsAsync ? Qt::QueuedConnection : Qt::DirectConnection,
                                                       _returnArg, _arguments.at(0),_arguments.at(1),_arguments.at(2),_arguments.at(3),_arguments.at(4),
                                                       _arguments.at(5),_arguments.at(6),_arguments.at(7));break;
        case 9: InvocationResult = InvokableMethod.invoke(this->parent(), this->IsAsync ? Qt::QueuedConnection : Qt::DirectConnection,
                                                       _returnArg, _arguments.at(0),_arguments.at(1),_arguments.at(2),_arguments.at(3),_arguments.at(4),
                                                       _arguments.at(5),_arguments.at(6),_arguments.at(7),_arguments.at(8));break;
        case 10: InvocationResult = InvokableMethod.invoke(this->parent(), this->IsAsync ? Qt::QueuedConnection : Qt::DirectConnection,
                                                       _returnArg, _arguments.at(0),_arguments.at(1),_arguments.at(2),_arguments.at(3),_arguments.at(4),
                                                       _arguments.at(5),_arguments.at(6),_arguments.at(7),_arguments.at(8),_arguments.at(9));break;
        }
        if (InvocationResult == false)
            throw exHTTPInternalServerError(QString("Unable to invoke method"));
    }

private:
    void updateDefaultValues(QMetaMethod _method){
        if(_method.parameterNames().size() < this->RequiredParamsCount){
            this->RequiredParamsCount = _method.parameterNames().size();
            this->LessArgumentMethods.append(_method);
        }
    }

private:
    QMetaMethod   BaseMethod;
    QList<QMetaMethod> LessArgumentMethods;
    bool          IsAsync;
    qint32        Cache4Secs;
    QList<QByteArray>  ParamNames;
    quint8        RequiredParamsCount;

    friend class RESTAPIRegistry;
};

}
}
#endif // QHTTP_PRIVATE_CLSAPIOBJECT_HPP
