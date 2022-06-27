/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     huangyu<huangyub@uniontech.com>
 *
 * Maintainer: huangyu<huangyub@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef SENDEVENTS_H
#define SENDEVENTS_H

#include <QString>
#include "services/project/projectservice.h"

class SendEvents final
{
    SendEvents() = delete;
    SendEvents(const SendEvents &) = delete;
public:
    static void generateStart(const QString &buildSystem, const QString &projectPath, const QString &targetPath);
    static void gengrateEnd(const QString &buildSystem,const QString &projectPath, const QString &targetPath);
    static void doubleCliekedOpenFile(const QString &workspace, const QString &language, const QString &filePath);
    static void projectActived(const dpfservice::ProjectInfo &info);
    static void projectCreated(const dpfservice::ProjectInfo &info);
    static void projectDeleted(const dpfservice::ProjectInfo &info);
};

#endif // SENDEVENTS_H