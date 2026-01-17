#pragma once

void invokeHook(HookDefinition& hook, HookContext& ctx) {
    if (hook.runType == RunType::Immediate || !ctx.safeToDefer) {
        hook.callback(ctx);
    } else {
        scheduler->queueTask([hook, ctx]() mutable {
            hook.callback(ctx);
        });
    }
}