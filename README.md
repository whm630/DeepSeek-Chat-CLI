# DeepSeek Chat CLI

一个使用 **C / C++** 编写的 DeepSeek 命令行聊天客户端。

DeepSeek Chat CLI 通过 DeepSeek API 在终端中进行对话，并提供多会话管理、历史记录持久化、文件内容输入、思考模式以及独立的配置程序。

项目主要依赖：

* C++
* C
* libcurl
* cJSON
* isocline

## Features

* 在终端中与 DeepSeek 对话
* 支持保存历史聊天记录
* 支持创建多个独立会话
* 使用关键字切换、删除会话
* 支持自定义 System Prompt
* 支持向对话中加入文本文件内容
* 支持显示模型的 reasoning content
* 支持开启 / 关闭 Thinking Mode
* 独立的 `deepseek-config` 配置工具
* API Key 保存在本地配置文件中
* 单实例运行检测
* 基于 libcurl 调用 DeepSeek API

---

## Dependencies

编译 DeepSeek Chat CLI 需要以下依赖：

```text
C++ compiler
C compiler
libcurl
cJSON
isocline
```

请根据你的 Linux 发行版安装对应的开发包。

---

## Build

假设项目中的主要源文件为：

```text
main.cpp
deepseek.cpp
deepseek.hpp
config.c
```

### 编译主程序

```bash
g++ -std=c++17 main.cpp deepseek.cpp \
    -lcurl -lcjson -lisocline \
    -o deepseek
```

### 编译配置程序

```bash
gcc config.c \
    -lcjson \
    -o deepseek-config
```

如果库或头文件安装在非标准路径，请根据实际环境添加对应的 `-I` 和 `-L` 参数。

---

## Configuration

程序默认使用：

```text
~/.deepseek/
```

保存配置和聊天数据。

第一次使用前请创建目录：

```bash
mkdir -p ~/.deepseek
```

然后运行配置程序：

```bash
./deepseek-config
```

配置界面可以设置：

```text
api_key
model
thinking_type
```

当前配置程序提供的模型选项包括：

```text
deepseek-flash
deepseek-v4-pro
```

Thinking Mode 可以设置为：

```text
enabled
disabled
```

配置最终保存在：

```text
~/.deepseek/config
```

格式类似：

```json
{
    "api_key": "YOUR_API_KEY",
    "model": "deepseek-flash",
    "thinking_type": "enabled"
}
```

> **注意：**
>
> API Key 当前以明文形式存储在本地配置文件中。
> 不要将自己的 `~/.deepseek/config` 上传到 GitHub。

---

## Usage

配置完成后启动：

```bash
./deepseek
```

程序启动时会显示当前模型和思考模式。

例如：

```text
DeepSeek CLI
当前模型: deepseek-flash
思考模式: 开
>>
```

直接输入内容即可发送消息：

```text
>>你好
正在思考中
回答:
你好！
```

如果当前模型返回 reasoning content，并且 Thinking Mode 已开启，程序还会显示：

```text
思考过程:
...

回答:
...
```

---

## Commands

DeepSeek Chat CLI 内置了一些以 `$>` 开头的命令。

### 查看帮助

```text
$>help
```

显示所有 CLI 命令。

---

### 退出

```text
$>exit
```

退出程序。

聊天历史会在程序正常退出时保存。

---

### 查看当前聊天记录

```text
$>msg
```

显示当前会话中的历史消息。

---

### 创建新会话

```text
$>new
```

程序会依次询问：

```text
请输入新对话的关键字>
请输入新对话中DeepSeek的设定>
```

例如：

```text
请输入新对话的关键字>linux
请输入新对话中DeepSeek的设定>你是一名Linux系统工程师
```

之后即可使用关键字：

```text
linux
```

找到这个会话。

每个会话拥有独立的聊天上下文。

---

### 查看所有会话

```text
$>all
```

例如：

```text
1:默认
2:linux
3:embedded
```

---

### 切换会话

```text
$>switch
```

然后输入会话关键字：

```text
请输入关键字>embedded
```

程序会切换到对应会话。

---

### 删除会话

```text
$>del
```

然后输入需要删除的会话关键字：

```text
输入关键字>linux
```

如果删除当前会话，程序会自动切换到第一个可用会话。

如果所有会话都被删除，程序会重新创建默认会话。

---

### 发送文件内容

```text
$>file <path>
```

例如：

```text
$>file /home/user/main.c
```

程序会读取文件内容并将其加入当前对话上下文，然后发送给 DeepSeek。

例如可以用于：

```text
$>file /home/user/main.c
$>file /home/userREADME.md
$>file /home/user/test.cpp
```

当前实现主要用于文本文件。

文件路径长度需要小于 256 字节，并且当前命令解析方式不支持路径中包含空格。

---

## Data Files

DeepSeek Chat CLI 使用以下文件：

```text
~/.deepseek/config
~/.deepseek/history
~/.deepseek/lock
```

### `config`

保存：

```text
API Key
模型
Thinking Mode
```

### `history`

以 JSON 格式保存所有会话以及聊天历史。

程序退出时会将当前会话状态写回该文件。

### `lock`

程序运行时创建的锁文件。

如果检测到该文件已经存在，DeepSeek Chat CLI 会认为另一个实例正在运行，并拒绝再次启动。

程序正常退出时会删除该文件。

---

## Conversation Storage

每个会话包含：

```text
key_word
messages
```

历史记录结构大致如下：

```json
[
    {
        "key_word": "默认",
        "messages": [
            {
                "role": "system",
                "content": "你是一个乐于助人的助手"
            },
            {
                "role": "user",
                "content": "Hello"
            },
            {
                "role": "assistant",
                "content": "Hi!"
            }
        ]
    }
]
```

不同会话拥有各自独立的消息上下文。

---

## Project Structure

推荐的仓库结构：

```text
DeepSeek-Chat-CLI/
├── src/
│   ├── main.cpp
│   └── deepseek.cpp
│
├── include/
│   └── deepseek.hpp
│
├── tools/
│   └── config.c
│
├── README.md
└── .gitignore
```

如果暂时不想拆目录，也可以直接：

```text
DeepSeek-Chat-CLI/
├── main.cpp
├── deepseek.cpp
├── deepseek.hpp
├── config.c
├── README.md
└── .gitignore
```

---

## `.gitignore`

建议至少加入：

```gitignore
*.o
*.out

deepseek
deepseek-config

core
core.*

.vscode/
.idea/
```

本项目的实际用户配置保存在：

```text
~/.deepseek/
```

因此正常情况下不会位于 Git 仓库中。

无论如何，都不要提交真实的 API Key。

---

## Notes

DeepSeek Chat CLI 目前是一个轻量级 Linux 命令行客户端。

当前实现使用同步 HTTP 请求，因此发送消息后程序会等待 DeepSeek API 返回完整结果，再继续接受新的用户输入。

文件输入功能目前主要面向文本文件，不适合作为通用二进制文件上传工具。

程序依赖 DeepSeek API，因此需要：

* 可用的网络连接
* 有效的 DeepSeek API Key
* API 账户拥有可用额度

---

## Example

```text
$ ./deepseek

DeepSeek CLI
当前模型: deepseek-flash
思考模式: 开

>>你好
正在思考中

回答:
你好！有什么我可以帮助你的？

>>$>new
请输入新对话的关键字>embedded
请输入新对话中DeepSeek的设定>你是一名嵌入式系统开发助手
切换成功
已创建新对话并切换

>>STM32 的 BTF 标志位是什么意思？
正在思考中

回答:
...

>>$>all
1:默认
2:embedded

>>$>exit
```

---

## About

DeepSeek Chat CLI 是一个用于学习和实践 C/C++、HTTP 网络编程、JSON 数据处理以及命令行应用程序设计的项目。

项目仍在持续开发中。

