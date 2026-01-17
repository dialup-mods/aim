#pragma once
#include <cstdint>
#include <fstream>
#include <string>

#include "GFxUI_classes.hpp"

struct FlashMatrix {
    float translateX = 0;
    float translateY = 0;
};

FlashMatrix
parseMatrix(const uint8_t* data, size_t size) {
    BitStream bs(data, size);
    FlashMatrix result;

    bool hasScale = bs.readBits(1);
    if (hasScale) {
        int scaleBits = bs.readBits(5);
        bs.readSignedBits(scaleBits); // skip scaleX
        bs.readSignedBits(scaleBits); // skip scaleY
    }

    bool hasRotate = bs.readBits(1);
    if (hasRotate) {
        int rotateBits = bs.readBits(5);
        bs.readSignedBits(rotateBits); // skip skew0
        bs.readSignedBits(rotateBits); // skip skew1
    }

    int translateBits = bs.readBits(5);
    int32_t tx = bs.readSignedBits(translateBits);
    int32_t ty = bs.readSignedBits(translateBits);

    // Flash units: 20 units = 1 pixel
    result.translateX = static_cast<float>(tx) / 20.f;
    result.translateY = static_cast<float>(ty) / 20.f;
    return result;
}

#include <algorithm>

void
dumpHexAroundPattern(const uint8_t* base, size_t size, const uint8_t* pattern, size_t patternSize, size_t context = 16) {
    for (size_t i = 0; i <= size - patternSize; ++i) {
        bool matched = true;
        for (size_t j = 0; j < patternSize; ++j) {
            if (base[i + j] != pattern[j]) {
                matched = false;
                break;
            }
        }

        if (matched) {
            printf("Match at offset 0x%zx:\n", i);

            size_t start = (i >= context) ? i - context : 0;
            size_t end = std::min(i + patternSize + context, size);

            for (size_t k = start; k < end; ++k) {
                if (k % 16 == 0)
                    printf("\n%08zx: ", k);
                printf("%02X ", base[k]);
            }

            printf("\n\n");
        }
    }
}

void dumpSwf() {
    auto allObjects = op->getAllInstancesOf<USwfMovie>();
    for (auto* obj : allObjects) {
        auto* player = obj;
        if (!player)
            continue; // || !player->MovieInfo) continue;

        auto* swf = player;
        if (!swf || swf->RawData.Num() == 0)
            return;

        std::string filename = "C:/Users/Public/hud_" + std::to_string(reinterpret_cast<uintptr_t>(player)) + ".swf";

        std::ofstream out(filename, std::ios::binary);
        out.write(reinterpret_cast<const char*>(swf->RawData.data()), swf->RawData.Num());
        out.close();

        printf("✅ dumped %s (%d bytes)\n", filename.c_str(), swf->RawData.Num());
    }
}


        cvarManager->registerNotifier(
            "dumpswf",
            [op, log, pe](std::vector<std::string> args) {
                auto allObjects = op->getAllInstancesOf<USwfMovie>();
                for (auto* obj : allObjects) {
                    auto* player = obj;
                    if (!player)
                        continue; // || !player->MovieInfo) continue;

                    auto* swf = player;
                    if (!swf || swf->RawData.Num() == 0)
                        return;

                    std::string filename = "C:/Users/Public/hud_" + std::to_string(reinterpret_cast<uintptr_t>(player)) + ".swf";

                    std::ofstream out(filename, std::ios::binary);
                    out.write(reinterpret_cast<const char*>(swf->RawData.data()), swf->RawData.Num());
                    out.close();

                    printf("✅ dumped %s (%d bytes)\n", filename.c_str(), swf->RawData.Num());
                }

                //            pe->registerTask(
                //                TaskBuilder()
                //                .name("BoostFixer")
                //                .functionName("Function Engine.HUD.PostRender")
                //                .phase(HookPhase::Post)
                //                .callback([log, pe](const PostEventContext& ctx) {
                //                    auto hud = ctx.getSelf<AGFxHUD_TA>();
                //
                //                    if (!hud || !hud->MatchInfoMovie) return;
                //
                //                    auto* swf = hud->MatchInfoMovie->MovieInfo;
                //                    if (!swf || swf->RawData.Num() == 0) return;
                //
                //                    std::string filename = "C:/Users/Public/hud_" + std::to_string(reinterpret_cast<uintptr_t>(hud))
                //                    + ".swf";
                //
                //                    std::ofstream out(filename, std::ios::binary);
                //                    out.write(reinterpret_cast<const char*>(swf->RawData.data()), swf->RawData.Num());
                //                    out.close();
                //
                //                    printf("✅ dumped %s (%d bytes)\n", filename.c_str(), swf->RawData.Num());
                //                })
                //                .once()
                //                .build()
                //                );
            },
            "ProcessEvent add hook",
            PERMISSION_ALL);
        // auto player = hud->MatchInfoMovie;
        // USwfMovie* swf = player->MovieInfo;
        //
        // const uint8_t pattern[] = {
        //     0xBF,             // PlaceObject2 flags
        //     0x06,             // matrix flag start
        //     0x19, 0x00,       // length or tag-specific
        //     0x00, 0x26,       // maybe part of depth setup
        //     0x1C, 0x00,       // depth = 28
        //     0x23, 0x02        // character ID = 547 ✅
        //     //0x20, 0x3D, // 20220 (X)
        //     //0xD0, 0x5F  // 24528 (Y)
        // };
        // size_t context = 16;

        // log->info("doing needful");
        // for (size_t i = 0; i <= swf->RawData.Num() - sizeof(pattern); ++i) {
        //
        //     bool matched = true;
        //     for (size_t j = 0; j < sizeof(pattern); ++j) {
        //         if (swf->RawData[i + j] != pattern[j]) {
        //             matched = false;
        //             break;
        //         }
        //     }

        //     if (matched) {
        //         log->logf_debug("Match at offset 0x{:X}", i);

        //         size_t start = (i >= context) ? i - context : 0;
        //         size_t end = std::min(i + sizeof(pattern) + context, static_cast<size_t>(swf->RawData.Num()));

        //         for (size_t k = start; k < end; ++k) {
        //             if (k % 16 == 0) printf("\n%08zx: ", k);
        //             printf("%02X ", swf->RawData[k]);
        //         }

        //         printf("\n\n");
        //     }
        // }
        //
        //
        //               // std::ofstream out("C:/Users/Public/xswagchat.swf", std::ios::binary);
        //               // out.write(reinterpret_cast<const char*>(swf->RawData.data()), swf->RawData.Num());
        //               // out.close();
        //
        //                //std::cout << "[+] Dumped .swf: " << swf->RawData.Num() << " bytes\n";
        //
        //
        //
        //
        //
        //
        //
        //        UClass* c = op->findStaticClass("Class GFxUI.GFxObject");
        //        if (c == nullptr) {
        //            log_->error("can't resolve c");
        //        }
        //
        //        cvarManager->registerNotifier(
        //            "bmeter",
        //            [log, pe, c](std::vector<std::string> args) {
        //
        //                pe->registerTask(
        //                    TaskBuilder()
        //                    .name("BoostFixer")
        //                    .functionName("Function Engine.HUD.PostRender")
        //                    .phase(HookPhase::Post)
        //                    .callback([c](InvocationContext& ctx) {
        //
        //                        auto* hud = ctx.getSelf<AGFxHUD_TA>();
        //
        //                        if (!c) {return;}
        //
        //                        auto movie = hud->MatchInfoMovie;
        //                        if (!movie) return;
        //
        //                        UGFxObject* boostMeter = movie->GetVariableObject(FString(L"_root.boostMeterView"), c);
        //                        if (!boostMeter) return;
        //
        //                        FASDisplayInfo info = boostMeter->GetDisplayInfo();
        //
        //                        if (info.X)         printf("  X:         %.2f\n", info.X);
        //                        if (info.Y)         printf("  Y:         %.2f\n", info.Y);
        //                        if (info.Z)         printf("  Z:         %.2f\n", info.Z);
        //                        if (info.Visible)   printf("  Visible:   %s\n", info.Visible ? "true" : "false");
        //
        //                        info.X = 100.f;
        //                        info.Y = 200.f;
        //                        boostMeter->SetDisplayInfo(info);
        //
        //                        //FString x = FString(std::string("_x"));
        //
        //                        //printf("HUD ptr: %p\n", static_cast<void*>(hud));
        //                        //printf("Movie ptr: %p\n", static_cast<void*>(movie));
        //                        //printf("Class ptr: %p\n", static_cast<void*>(c));
        //                        //printf("boostMeter ptr: %p\n", static_cast<void*>(boostMeter));
        //
        //                        //FString x = FString(L"_x");
        //                        //FString y = FString(L"_y");
        //                        //boostMeter->SetFloat(x, 100.f);
        //                        //boostMeter->SetFloat(y, 200.f);
        //                    })
        //                    .once()
        //                    .build()
        //                    );
        //
        //            }
        //            , "boost meter adjust"
        //            , PERMISSION_ALL
        //        );
        //
