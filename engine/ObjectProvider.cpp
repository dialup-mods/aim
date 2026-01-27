#include "AsyncGate.h"
#include "ILogger.h"
#include "ObjectProvider.h"

ObjectProvider::~ObjectProvider() {
    log_->debug("ObjectProvider unloading...");
    //    for (UObject* uObject : m_createdObjects) {
    //        if (uObject) {
    //            MarkForDestroy(uObject);
    //        }
    //    }
    //    m_createdObjects.clear();
    log_->debug("ObjectProvider unloaded");
}
