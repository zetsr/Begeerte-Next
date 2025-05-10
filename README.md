# Begeerte-Next

Server-side Plugins for Dragons Legacy

## 项目简介

Begeerte-Next 为 Dragons Legacy 提供服务器端插件功能，允许通过脚本扩展服务器行为。

## 使用方法

按照以下步骤安装并使用 Begeerte-Next：

1.  **下载并解压**: 下载 `*Begeerte-Next.rar*` 并将其解压。
2.  **移动文件夹**: 将解压得到的 `*Dragons*` 文件夹移动到 Dragons Legacy 服务器根目录，即 `*../../../Dragons-Legacy-Dedicated-Server*`。
3.  **运行服务端**: 启动你的 Dragons Legacy 专用服务器。
4.  **检查日志**: 如果安装成功，你应该能在服务器控制台或日志文件中看到类似 `*[Begeerte] 初始化*` 的输出。

## 卸载方法

要卸载 Begeerte-Next，请执行以下步骤：

1. 移除 `*../../../Dragons/Binaries/Win64/version.dll*` 文件。

## 插件系统

将源代码保存为.beg格式并放在 `*../../../Dragons/Binaries/Win64/Begeerte/Scripts*`

目前可用的插件 API 包括：

* API 1
* API 2
* API 3

(更多 API 等待扩展)

## 语法示例

以下是一个简单的插件语法示例：

```c
// Test Script
LogToFile("Starting test.beg script", "!")
print("i am ", "test ", "message")

while (true){

    /*
        print("i am ", "test ", "message")
    */

    EntityList_Update()

    let max_p = EntityList_GetMaxPlayers()
    let player_id_to_check = 1
    let entity_1 = nil
    // print(max_p)

    if (max_p != 0){
        entity_1 = EntityList_GetEntity(player_id_to_check)
        // print(entity_1)
    }else{
        // print("No max_p")
    }

    if (entity_1 != 0){
        let p1 = EntityList_GetPlayer(player_id_to_check)
        let p1_IsValid = Player_IsValid(p1)

        if (p1_IsValid){
            let p1_health = Player_GetHealth(p1)
            // mprint(p1_health)

            Player_SetSkinIndex(p1, 30)
        }
    }
}
