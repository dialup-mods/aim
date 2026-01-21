#pragma once
#include <memory>

class UObject;
class ObjectEntry;

namespace UEModel {
auto assignClass(UObject* rawObj) -> std::shared_ptr<ObjectEntry>;
auto assignClassFromRawPtr(void* raw) -> std::shared_ptr<ObjectEntry>;
}