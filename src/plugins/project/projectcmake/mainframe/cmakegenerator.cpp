﻿/*
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
#include "cmakegenerator.h"
#include "cmakeasynparse.h"
#include "cmakeitemkeeper.h"
#include "transceiver/sendevents.h"
#include "transceiver/projectcmakereceiver.h"
#include "services/window/windowservice.h"
#include "properties/propertiesdialog.h"
#include "properties/buildpropertywidget.h"
#include "properties/runpropertywidget.h"
#include "properties/configpropertywidget.h"

#include <QtXml>
#include <QFileIconProvider>
#include <QPushButton>

class CmakeGeneratorPrivate
{
    friend class CmakeGenerator;

    enum CreateItemMode{
        NewCreateProject,
        RebuildProject,
    };

    QHash<QStandardItem*, QThreadPool*> asynItemThreadPolls;
    QList<QStandardItem*> reloadCmakeFileItems;
    dpfservice::ProjectInfo configureProjectInfo;
};

CmakeGenerator::CmakeGenerator()
    : d(new CmakeGeneratorPrivate())
{
    QObject::connect(this, &CmakeGenerator::createRootItemAsynEnd,
                     this, &CmakeGenerator::setRootItemToView);

    // when execute command end can create root Item
    QObject::connect(ProjectCmakeProxy::instance(),
                     &ProjectCmakeProxy::buildExecuteEnd,
                     this, &CmakeGenerator::doBuildCmdExecuteEnd);

    // main thread init watcher class
    CmakeItemKeeper::instance();

    // build cmake file changed notify
    QObject::connect(CmakeItemKeeper::instance(),
                     &CmakeItemKeeper::cmakeFileNodeNotify,
                     this, &CmakeGenerator::doCmakeFileNodeChanged);

    using namespace dpfservice;
    auto &ctx = dpfInstance.serviceContext();
    ProjectService *projectService = ctx.service<ProjectService>(ProjectService::name());
    if (!projectService) {
        qCritical() << "Failed, not found service : projectService";
        abort();
    }

    QObject::connect(this, &ProjectGenerator::targetExecute,
                     [=](const QString &cmd, const QStringList &args) {
        // Execute project tree command.
        emit projectService->targetCommand(cmd, args);
    });
}

CmakeGenerator::~CmakeGenerator()
{
    qInfo() << __FUNCTION__;
    for (auto val : d->asynItemThreadPolls.keys()) {
        auto threadPoll = d->asynItemThreadPolls[val];
        if (threadPoll) {
            threadPoll->clear();
            while (threadPoll->activeThreadCount() != 0) {}
            delete threadPoll;
        }
    }

    d->asynItemThreadPolls.clear();

    if (d)
        delete d;
}

QWidget *CmakeGenerator::configureWidget(const QString &language,
                                         const QString &projectPath)
{
    using namespace dpfservice;
    auto &ctx = dpfInstance.serviceContext();
    ProjectService *projectService = ctx.service<ProjectService>(ProjectService::name());
    if (!projectService)
        return nullptr;

    auto proInfos = projectService->projectView.getAllProjectInfo();
    for (auto &val : proInfos) {
        if (val.language() == language && projectPath == val.projectFilePath()) {
            ContextDialog::ok(QDialog::tr("Cannot open repeatedly!\n"
                                          "language : %0\n"
                                          "projectPath : %1")
                              .arg(language, projectPath));
            return nullptr;
        }
    }

    // show build type config pane.
    ConfigPropertyWidget *configPropertyWidget = new ConfigPropertyWidget(language, projectPath);
    QObject::connect(configPropertyWidget, &ConfigPropertyWidget::configureDone, [this](const dpfservice::ProjectInfo &info) {
        configure(info);
    });

    return configPropertyWidget;
}

bool CmakeGenerator::configure(const dpfservice::ProjectInfo &info)
{
    // cache project info, asyn end to use
    d->configureProjectInfo = info;

    // asyn execute command generat project file from cmake
    ProjectCmakeProxy::instance()->setbuildOriginCmd(infoBuildCmd(info).join(" "));

    emit targetExecute(info.buildProgram(), info.buildCustomArgs());

    Generator::started(); // emit starded
    return true;
}

QStandardItem *CmakeGenerator::createRootItem(const dpfservice::ProjectInfo &info)
{
    using namespace dpfservice;
    QStandardItem * rootItem = new QStandardItem();
    d->asynItemThreadPolls[rootItem] = new QThreadPool;

    auto parse = new CmakeAsynParse;

    // asyn free parse, that .project file parse
    QObject::connect(parse, &CmakeAsynParse::parseProjectEnd,
                     [=](CmakeAsynParse::ParseInfo<QStandardItem*> info){
        d->asynItemThreadPolls.remove(info.result);
        delete parse;
        createRootItemAsynEnd(info.result);
    });

    // asyn execute logic,  that .project file parse
    QtConcurrent::run(d->asynItemThreadPolls[rootItem],
                      parse, &CmakeAsynParse::parseProject,
                      rootItem, info);

    return rootItem;
}

void CmakeGenerator::removeRootItem(QStandardItem *root)
{
    // remove watcher from current root item
    CmakeItemKeeper::instance()->delCmakeFileNode(root);

    auto threadPoll = d->asynItemThreadPolls[root];
    if (threadPoll) {
        threadPoll->clear();
        while(threadPoll->waitForDone());
        delete threadPoll;
        d->asynItemThreadPolls.remove(root);
    }

    recursionRemoveItem(root);
}

QMenu *CmakeGenerator::createItemMenu(const QStandardItem *item)
{
    QMenu *menu = nullptr;

    // create parse
    CmakeAsynParse *parse = new CmakeAsynParse();

    // create item from syn
    auto targetBuilds = parse->parseActions(item);

    // free parse from syn
    delete parse;

    if (!targetBuilds.isEmpty()) {
        menu = new QMenu();
        for (auto val : targetBuilds) {
            QAction *action = new QAction();
            action->setText(val.buildName);
            action->setProperty(CDT_CPROJECT_KEY::get()->buildCommand.toLatin1(), val.buildCommand);
            action->setProperty(CDT_CPROJECT_KEY::get()->buildArguments.toLatin1(), val.buildArguments);
            action->setProperty(CDT_CPROJECT_KEY::get()->buildTarget.toLatin1(), val.buildTarget);
            action->setProperty(CDT_CPROJECT_KEY::get()->stopOnError.toLatin1(), val.stopOnError);
            action->setProperty(CDT_CPROJECT_KEY::get()->useDefaultCommand.toLatin1(), val.useDefaultCommand);
            QObject::connect(action, &QAction::triggered, this, &CmakeGenerator::actionTriggered, Qt::UniqueConnection);
            menu->addAction(action);
        }
    }

    if (!menu) {
        menu = new QMenu();
    }

    QAction *action = new QAction("Properties");
    menu->addAction(action);
    QObject::connect(action, &QAction::triggered, this, &CmakeGenerator::actionProperties, Qt::UniqueConnection);

    return menu;
}

void CmakeGenerator::actionTriggered()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action) {
        QString program = action->property(CDT_CPROJECT_KEY::get()->buildCommand.toLatin1()).toString();
        QStringList args = action->property(CDT_CPROJECT_KEY::get()->buildArguments.toLatin1()).toString().split(" ");
        args << action->property(CDT_CPROJECT_KEY::get()->buildTarget.toLatin1()).toString();

        // remove extra quotes and empty argument.
        QStringList argsFiltered;
        for (auto &arg : args) {
            if (!arg.isEmpty()) {
                argsFiltered << arg.replace("\"", "");
            }
        }
        emit targetExecute(program, argsFiltered);
    }
}

void CmakeGenerator::setRootItemToView(QStandardItem *root)
{
    d->asynItemThreadPolls.remove(root);

    using namespace dpfservice;
    auto &ctx = dpfInstance.serviceContext();
    ProjectService *projectService = ctx.service<ProjectService>(ProjectService::name());
    if (!projectService)
        return;

    WindowService *windowService = ctx.service<WindowService>(WindowService::name());
    if (!windowService)
        return;

    if (root) {
        // setting item to view
        if (projectService->projectView.addRootItem)
            projectService->projectView.addRootItem(root);

        // expand view from tree two level
        if (projectService->projectView.expandedDepth)
            projectService->projectView.expandedDepth(root, 2);

        // switch navigation to edit
        if (windowService->switchWidgetNavigation)
            windowService->switchWidgetNavigation(MWNA_EDIT);
        // switch workspace view to projects
        if (windowService->switchWidgetWorkspace)
            windowService->switchWidgetWorkspace(MWCWT_PROJECTS);
    }
}

static QMutex mutex;
void CmakeGenerator::doBuildCmdExecuteEnd(const QString &cmd, int status)
{
    // configure function cached info
    if (d->configureProjectInfo.isEmpty())
        return;

    using namespace dpfservice;
    auto &ctx = dpfInstance.serviceContext();
    ProjectService *projectService = ctx.service<ProjectService>(ProjectService::name());
    if (!projectService)
        return;

    // get reload item from reload cmake file cache
    mutex.lock();
    QStandardItem *reloadItem = nullptr;
    for (auto val : d->reloadCmakeFileItems) {
        if(cmd == infoBuildCmd(ProjectInfo::get(val)).join(" ")) {
            reloadItem = val;
            break;
        }
    }
    mutex.unlock();

    if (reloadItem) {
        d->reloadCmakeFileItems.removeOne(reloadItem); //clean cache
        if (status == 0) {
            projectService->projectView.removeRootItem(reloadItem);
            createRootItem(d->configureProjectInfo);
        } else {
            qCritical() << "Failed execute cmd : " << cmd << "status : " << status;
        }
    } else {
        createRootItem(d->configureProjectInfo);
    }

    d->configureProjectInfo = {};

    emit projectService->projectConfigureDone();
}

void CmakeGenerator::doCmakeFileNodeChanged(QStandardItem *root, const QPair<QString, QStringList> &files)
{
    Q_UNUSED(files);

    if (d->reloadCmakeFileItems.contains(root))
        return;

    qInfo() << __FUNCTION__;
    using namespace dpfservice;

    // get current project info
    auto proInfo = ProjectInfo::get(root);

    // cache the reload item
    d->reloadCmakeFileItems.append(root);

    // reconfigure project info
    configure(proInfo);
}

QStringList CmakeGenerator::infoBuildCmd(const dpfservice::ProjectInfo &info) const
{
    QStringList args = info.buildCustomArgs();
    args.push_front(info.buildProgram());
    return args;
}

void CmakeGenerator::actionProperties()
{
    PropertiesDialog dlg;
    ConfigPropertyWidget *configWidget = new ConfigPropertyWidget("", "");
    BuildPropertyWidget *buildWidget = new BuildPropertyWidget();
    RunPropertyWidget *runWidget = new RunPropertyWidget();

    dlg.insertPropertyPanel("Config", configWidget);
    dlg.insertPropertyPanel("Build", buildWidget);
    dlg.insertPropertyPanel("Run", runWidget);

    dlg.exec();
}