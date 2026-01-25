#pragma once
#include <IModule.h>

#include "ObjectProvider.h"
#include "ProcessEvent.h"

#ifdef false
class Toaster : public IModule {
    AIM_INJECTABLE(Toast)
    AIM_INJECT(ObjectProvider, objectProvider)
    AIM_INJECT(ProcessEvent, processEvent)

    void showToast_SEH(UNotificationManager_TA* mgr, const FStringBacked& title, const FStringBacked& content, const int duration) {
        __try {
            auto* toaster = mgr->PopUpOnlyNotification(UGenericNotification_TA::StaticClass());
            toaster->SetTitle(title);
            toaster->SetBody(content);
            toaster->PopUpDuration = static_cast<float>(duration);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
//#ifdef DEBUG_BUILD
            MessageBoxW(nullptr, L"Toast notification exploded 💣", L"SEH Exception", MB_OK | MB_ICONERROR);
//#endif
        }
    }

    void toast(const std::wstring& title, const std::wstring& content, const int duration = 5) {
        showToast_SEH(objectProvider_->getInstanceOf<UNotificationManager_TA>(), FStringBacked(title), FStringBacked(content), duration);
    }
};

#endif