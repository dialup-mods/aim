#pragma once

#include <functional>
#include <string>

class ScopedHook {
  public:
    ScopedHook(const std::function<void()>& hookFunc, std::function<void()> unhookFunc)
      : unhookFunc_(std::move(unhookFunc)) {
        if (hookFunc) {
            hookFunc();
        }
    }

    ~ScopedHook() {
        if (unhookFunc_) {
            unhookFunc_();
        }
    }

    ScopedHook(const ScopedHook&) = delete;
    ScopedHook& operator=(const ScopedHook&) = delete;

  private:
    std::function<void()> unhookFunc_;
};