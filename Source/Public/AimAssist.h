//

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FAimAssist : public IModuleInterface
{
public:

    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};