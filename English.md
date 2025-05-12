# Begeerte-Next

Server plugins for Dragons of Dragons / Dragons Legacy

## Usage

Follow these steps to install and use Begeerte-Next:

* [Download *Begeerte-Next.rar*](https://github.com/zetsr/Begeerte-Next/archive/refs/heads/main.zip) and extract it.
* Select the corresponding game version, then navigate to *Begeerte-Next-main\<Game Version>\Windows\release\Begeerte-Next*
* Move the *Dragons* folder to the root directory of your Dragons of Dragons / Dragons Legacy server.
* Start your server.
* If the installation is successful, you should see output similar to `*[Begeerte] Initializing*` in the server console or log file.

## Uninstallation

To uninstall Begeerte-Next, follow these steps:

* Remove the `*../../../Dragons/Binaries/Win64/version.dll*` file.

## Plugin System

### Usage

* Our plugins use a C-like language. *Save the source code in .beg format and place it in `*../../../Dragons/Binaries/Win64/Begeerte/Scripts*`*. They will be loaded when the server starts.

### API

### printf
printf(string [text], ...)


### print
print(string [text], ...)


### LogToFile
LogToFile(string [text], ...)


### EntityList_Update
EntityList_Update()


### EntityList_GetMaxPlayers
EntityList_GetMaxPlayers()


### EntityList_GetEntity
EntityList_GetEntity(int [entity id])


### EntityList_GetPlayer
EntityList_GetPlayer(int [entity id])


### EntityList_GetAllEntities
EntityList_GetAllEntities()


### Player_IsValid
Player_IsValid(Player* [player])


### Player_GetCharacter
Player_GetCharacter(Player* [player])


### Player_GetCharacterRaw
Player_GetCharacterRaw(Player* [player])


### Player_SetCharacter
Player_SetCharacter(Player* [player], byte [value])


### Player_GetValidFlag
Player_GetValidFlag(Player* [player])


### Player_SetValidFlag
Player_SetValidFlag(Player* [player], byte [value])


### Player_GetHealth
Player_GetHealth(Player* [player])


### Player_SetHealth
Player_SetHealth(Player* [player], byte [value])


### Player_GetSkinIndex
Player_GetSkinIndex(Player* [player])


### Player_SetSkinIndex
Player_SetSkinIndex(Player* [player], byte [value])


### Player_GetGenderRaw
Player_GetGenderRaw(Player* [player])


### Player_SetGender
Player_SetGender(Player* [player], byte [value])


### Player_GetGrowthStageRaw
Player_GetGrowthStageRaw(Player* [player])


### Player_SetGrowthStage
Player_SetGrowthStage(Player* [player], byte [value])


### Player_GetSavedGrowth
Player_GetSavedGrowth(Player* [player])


### Player_SetSavedGrowth
Player_SetSavedGrowth(Player* [player], byte [value])


### Player_GetVitalityHealth
Player_GetVitalityHealth(Player* [player])


### Player_SetVitalityHealth
Player_SetVitalityHealth(Player* [player], byte [value])


### Player_GetVitalityHealthGrade
Player_GetVitalityHealthGrade(Player* [player])


## Syntax Example

The following is a simple plugin syntax example:

```c#
// Day of Dragons
// Set Creator Skin for all players
LogToFile("Starting beg script", "!")

let Creator_Skin = 10

while (true){
    // Update Entities
    EntityList_Update()

    // Get Current Players
    let CurrentPlayers = EntityList_GetMaxPlayers()

    // Start from player 1
    let i = 1

    // Check All Players
    while (i <= CurrentPlayers){
        // Entity
        let entity = EntityList_GetEntity(i)

        if (entity != 0){
            // Player
            let player = EntityList_GetPlayer(i)

            // Valid Player
            if (Player_IsValid(player)){
                if (Player_GetSkinIndex(player) != Creator_Skin){
                    // Example action: Set skin
                    Player_SetSkinIndex(player, Creator_Skin)
                    LogToFile("player: ", player, "\'s skin set to: ", Creator_Skin)
                    print("player: ", player, "\'s skin set to: ", Creator_Skin)
                }
            }
        }

        i = i + 1
    }
}
