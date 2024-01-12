// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "console.h"
#include "base/abstractwidget.h"
#include "services/window/windowservice.h"
#include "services/terminal/terminalservice.h"
#include "common/util/eventdefinitions.h"
#include "consolewidget.h"

using namespace dpfservice;
void Console::initialize()
{
    qInfo() << __FUNCTION__;
    //发布Console到edit导航栏界面布局
    if (QString(getenv("TERM")).isEmpty()) {
        setenv("TERM", "xterm-256color", 1);
    }

    // load terminal service.
    QString errStr;
    auto &ctx = dpfInstance.serviceContext();
    if (!ctx.load(TerminalService::name(), &errStr)) {
        qCritical() << errStr;
    }
}

bool Console::start()
{
    qInfo() << __FUNCTION__;

    auto &ctx = dpfInstance.serviceContext();
    WindowService *windowService = ctx.service<WindowService>(WindowService::name());
    if (windowService) {
        windowService->addContextWidget(QString(tr("&Console")), new AbstractWidget(ConsoleWidget::instance()), MWNA_EDIT, true);
    }

    // bind service.
    auto terminalService = ctx.service<TerminalService>(TerminalService::name());
    if (terminalService) {
        using namespace std::placeholders;
        terminalService->executeCommand = std::bind(&ConsoleWidget::sendText, ConsoleWidget::instance(), _1);
    }
    return true;
}

dpf::Plugin::ShutdownFlag Console::stop()
{
    return Sync;
}
