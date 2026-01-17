#pragma once
#include "ILogger.h"
#include "IModule.h"
#include "ProcessEvent.h"
#include "TaskBuilder.h"

class GracefulUnloader : public IModule {
    AIM_INJECTABLE(GracefulUnloader)
    AIM_INJECT(ILogger, log)
    AIM_INJECT(ProcessEvent, processEvent)

    void scheduleGracefulExit() const {
        // Hook into match end event
        processEvent_->registerTask(TaskBuilder()
            .name("GracefulCrashExit")
            .functionName("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded")
            .phase(HookPhase::Post)
            .callback([](InvocationContext&) {
                printf("[INFO] Match ended - exiting due to earlier exception\n");
                exit(0);  // or ShowMessageBox then exit
            })
            .once()
            .build()
        );
    }
};