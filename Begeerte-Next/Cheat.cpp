#include "Cheat.h"
#include "Hook.h"
#include "Console.h"
#include "Memory.h"
#include <thread>
#include <stdio.h>
#include "SDK.h"
#include "Vector.h"
#include "EntityList.h"

namespace Cheat {
    static bool isRunning = false;
    static HANDLE cheatThread = nullptr;

    static void UpdateThread() {
        // ��ʱ����Ҫ��
    }

    static void CheatThread() {
        while (isRunning) {
            // �������е�ַ
            UpdateThread();

            EntityList::Update();

            for (int i = 1; i <= EntityList::GetMaxPlayers(); i++) {

                auto entity = EntityList::GetEntity(i);

                // û��ʵ��
                if (entity == 0) {
                    continue;
                }

                auto player = EntityList::GetPlayer(i);

                // ��ָ��
                if (!player) {
                    continue;
                }

                // ��Чʵ��
                if (!player->IsValid()) {
                    continue;
                }

                // ��ֵĶ���
                if (player->Health <= 0) {
                    continue;
                }

                wprintf(L"[Begeerte] Entity (%d) | HP: %d | %s | %s | %s\n",
                    i,
                    player->Health,
                    player->GetCharacter(),
                    player->GetGrowthStage(),
                    player->GetGender()
                );

                player->SkinIndex = 30; // GF
                player->GrowthStage = 3; // Elder

                player->VitalityHealth = 14; // A++
                player->VitalityArmor = 14;
                player->VitalityBile = 14;
                player->VitalityStamina = 14;
                player->VitalityHunger = 14;
                player->VitalityThirst = 14;
                player->VitalityTorpor = 14;
                player->DamageBite = 14;
                player->DamageProjectile = 14;
                player->DamageSwipe = 14;
                player->MitigationBlunt = 14;
                player->MitigationPierce = 14;
                player->MitigationFire = 14;
                player->MitigationFrost = 14;
                player->MitigationAcid = 14;
                player->MitigationVenom = 14;
                player->MitigationPlasma = 14;
                player->MitigationElectricity = 14;
                player->OverallQuality = 14;
            }
        }
    }

    void Start() {
        if (!isRunning) {
            isRunning = true;
            cheatThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)CheatThread, nullptr, 0, nullptr);
            printf("[Begeerte] thread started.\n");
        }
    }

    void Stop() {
        isRunning = false;
        if (cheatThread) {
            WaitForSingleObject(cheatThread, INFINITE);
            CloseHandle(cheatThread);
            cheatThread = nullptr;
        }
        printf("[Begeerte] thread stopped.\n");
    }
}