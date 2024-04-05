/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "autobotmodule.h"

#include <QQmlEngine>

#include "modularity/ioc.h"
#include "ui/iinteractiveuriregister.h"
#include "ui/iuiactionsregister.h"

#include "internal/autobot.h"
#include "internal/autobotconfiguration.h"
#include "view/autobotscriptsmodel.h"
#include "view/testcaserunmodel.h"

#include "draw/painter.h"
#include "internal/draw/abpaintprovider.h"
#include "internal/autobotactionscontroller.h"
#include "internal/autobotactions.h"
#include "internal/autobotscriptsrepository.h"

#include "internal/api/autobotapi.h"
#include "internal/api/contextapi.h"
#include "internal/api/diagnosticsapi.h"

#include "diagnostics/idiagnosticspathsregister.h"

using namespace muse::autobot;
using namespace mu::api;

std::string AutobotModule::moduleName() const
{
    return "autobot";
}

void AutobotModule::registerExports()
{
    m_configuration = std::make_shared<AutobotConfiguration>();
    m_autobot = std::make_shared<Autobot>();
    m_actionsController = std::make_shared<AutobotActionsController>();

    modularity::ioc()->registerExport<IAutobot>(moduleName(), m_autobot);
    modularity::ioc()->registerExport<IAutobotConfiguration>(moduleName(), m_configuration);
    modularity::ioc()->registerExport<IAutobotScriptsRepository>(moduleName(), new AutobotScriptsRepository());

    // draw::Painter::extended = AbPaintProvider::instance();
}

void AutobotModule::resolveImports()
{
    auto ir = modularity::ioc()->resolve<mu::ui::IInteractiveUriRegister>(moduleName());
    if (ir) {
        ir->registerQmlUri(Uri("musescore://autobot/batchtests"), "MuseScore/Autobot/BatchTestsDialog.qml");
        ir->registerQmlUri(Uri("musescore://autobot/scripts"), "MuseScore/Autobot/ScriptsDialog.qml");
        ir->registerQmlUri(Uri("musescore://autobot/selectfile"), "MuseScore/Autobot/AutobotSelectFileDialog.qml");
    }

    auto ar = modularity::ioc()->resolve<mu::ui::IUiActionsRegister>(moduleName());
    if (ar) {
        ar->reg(std::make_shared<AutobotActions>());
    }

    auto api = modularity::ioc()->resolve<IApiRegister>(moduleName());
    if (api) {
        api->regApiCreator("autobot", "api.autobot", new ApiCreator<api::AutobotApi>());
        api->regApiCreator("autobot", "api.context", new ApiCreator<ContextApi>());
        api->regApiCreator("diagnostics", "api.diagnostics", new ApiCreator<DiagnosticsApi>());
    }
}

void AutobotModule::registerUiTypes()
{
    qmlRegisterType<AutobotScriptsModel>("MuseScore.Autobot", 1, 0, "AutobotScriptsModel");
    qmlRegisterType<TestCaseRunModel>("MuseScore.Autobot", 1, 0, "TestCaseRunModel");
}

void AutobotModule::onInit(const IApplication::RunMode& mode)
{
    if (mode == IApplication::RunMode::AudioPluginRegistration) {
        return;
    }

    m_autobot->init();
    m_actionsController->init();

    //! --- Diagnostics ---
    auto pr = modularity::ioc()->resolve<mu::diagnostics::IDiagnosticsPathsRegister>(moduleName());
    if (pr) {
        for (const io::path_t& p : m_configuration->scriptsDirPaths()) {
            pr->reg("autobotScriptsPath", p);
        }
        for (const io::path_t& p : m_configuration->testingFilesDirPaths()) {
            pr->reg("autobotTestingFilesPath", p);
        }
        pr->reg("autobotDataPath", m_configuration->dataPath());
        pr->reg("autobotSavingFilesPath", m_configuration->savingFilesPath());
        pr->reg("autobotReportsPath", m_configuration->reportsPath());
        pr->reg("autobotDrawDataPath", m_configuration->drawDataPath());
    }
}
