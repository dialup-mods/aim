//#pragma once
//
//#include "TaskStructs.h"
//
//#include <future>
//#include <thread>
//#include "patchutils.h"
//
//template <typename Derived>
//class FunctionBase {
//
//    
//public:
//    bool init() {
////        derived()->getLogger()->debug("Creating instance: " + derived()->getName());
////        std::future<bool> result = waitForBakkesAndPatch<Derived>();
////        result.wait();
////        return result.get();
//    }
//
//    void shutdown() {
//        derived()->getHooker()->shutdown();
//    }
//
//    void registerTask(TaskDefinition def) {
//        derived()->getHooker()->registerTask(def);
//    }
//
//    void releaseTask(TaskDefinition def) {
//        derived()->getHooker()->releaseTask(def);
//    }
//
//    void clearTasks() {
//        derived()->getHooker()->clearTasks();
//    }
//    
//protected:
//
//    uintptr_t getAddress() const { 
//        return ptr_to_uintptr(getRLFn()); 
//    }
//
//    std::string getAddressString() const {
//        std::ostringstream oss;
//        oss << std::hex << std::uppercase << ptr_to_uintptr(getRLFn());
//        return oss.str();
//    }
//
//
//    void* getRLFn() const {
//        if (derived()->cachedVTableFn != nullptr) return derived()->cachedVTableFn;
//        
//        return derived()->shouldUseVTableEntry()
//            ? resolveFromVTable()
//            : resolveFromPattern();
//    }
//
//    bool getIsBakkesModPatched() const {
//        BYTE* func = reinterpret_cast<BYTE*>(resolveProcessEventFromVTable());
//        return (func[0] == 0xE9);  // Check if first byte is JMP rel32
//    }
//
//    void* resolveFromPattern() const {
//        return patchutils::patternScan(derived()->getPattern(), derived()->shouldResolveJump());
//    }
//
//    void* resolveFromVTable() const {
//        derived()->cachedVTableFn = reinterpret_cast<void**>(UObject::StaticClass()->VfTableObject.Dummy)[derived()->getVTableIndex()];
//        return derived()->cachedVTableFn;
//    }
//
//    void* resolveProcessEventFromVTable() const {
//        return reinterpret_cast<void**>(UObject::StaticClass()->VfTableObject.Dummy)[67];
//    }
//    
//
//private:
//    inline Derived* derived() { return static_cast<Derived*>(this); }
//    inline const Derived* derived() const { return static_cast<const Derived*>(this); }
//    
//};