# Begeerte-Next

Server plugins for Dragons of Dragons / Dragons Legacy

## 使用方法

按照以下步骤安装并使用 Begeerte-Next：

* [下载 *Begeerte-Next.rar*](https://github.com/zetsr/Begeerte-Next/archive/refs/heads/main.zip) 并将其解压。
* 选择对应的游戏版本，然后定向到 *Begeerte-Next-main\<游戏版本>\Windows\release\Begeerte-Next*
* 将 *Dragons* 文件夹移动到 Dragons of Dragons / Dragons Legacy 服务器根目录。
* 启动你的服务器。
* 如果安装成功，你应该能在服务器控制台或日志文件中看到类似 `*[Begeerte] 初始化*` 的输出。

## 卸载方法

要卸载 Begeerte-Next，请执行以下步骤：

* 移除 `*../../../Dragons/Binaries/Win64/version.dll*` 文件。

## 插件系统

### 使用方法

* 我们的插件使用类C语言，*将源代码保存为.beg格式并放在 `*../../../Dragons/Binaries/Win64/Begeerte/Scripts*`*，它们会在服务端启动的时候加载。

### API

### printf
```
printf(string [text], ...)
```

### print
```
print(string [text], ...)
```

### LogToFile
```
LogToFile(string [text], ...)
```

### EntityList_Update
```
EntityList_Update()
```

### EntityList_GetMaxPlayers
```
EntityList_GetMaxPlayers()
```

### EntityList_GetEntity
```
EntityList_GetEntity(int [entity id])
```

### EntityList_GetPlayer
```
EntityList_GetPlayer(int [entity id])
```

### EntityList_GetAllEntities
```
EntityList_GetAllEntities()
```

### Player_IsValid
```
Player_IsValid(Player* [player])
```

### Player_GetCharacter
```
Player_GetCharacter(Player* [player])
```

### Player_GetCharacterRaw
```
Player_GetCharacterRaw(Player* [player])
```

### Player_SetCharacter
```
Player_SetCharacter(Player* [player], byte [value])
```

### Player_GetValidFlag
```
Player_GetValidFlag(Player* [player])
```

### Player_SetValidFlag
```
Player_SetValidFlag(Player* [player], byte [value])
```

### Player_GetHealth
```
Player_GetHealth(Player* [player])
```

### Player_SetHealth
```
Player_SetHealth(Player* [player], byte [value])
```

### Player_GetSkinIndex
```
Player_GetSkinIndex(Player* [player])
```

### Player_SetSkinIndex
```
Player_SetSkinIndex(Player* [player], byte [value])
```

### Player_GetGenderRaw
```
Player_GetGenderRaw(Player* [player])
```

### Player_SetGender
```
Player_SetGender(Player* [player], byte [value])
```

### Player_GetGrowthStageRaw
```
Player_GetGrowthStageRaw(Player* [player])
```

### Player_SetGrowthStage
```
Player_SetGrowthStage(Player* [player], byte [value])
```

### Player_GetSavedGrowth
```
Player_GetSavedGrowth(Player* [player])
```

### Player_SetSavedGrowth
```
Player_SetSavedGrowth(Player* [player], byte [value])
```

### Player_GetVitalityHealth
```
Player_GetVitalityHealth(Player* [player])
```

### Player_SetVitalityHealth
```
Player_SetVitalityHealth(Player* [player], byte [value])
```

### Player_GetVitalityHealthGrade
```
Player_GetVitalityHealthGrade(Player* [player])
```

## 语法示例

以下是一个简单的插件语法示例：

```c#
// Day of Dragons
// 为所有玩家设置 Creator Skin
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
```
