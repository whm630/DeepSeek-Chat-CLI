# DeepSeek Chat CLI

一个使用 **C / C++** 编写的 DeepSeek 命令行聊天客户端。

DeepSeek Chat CLI 通过 DeepSeek API 在终端中与模型对话，支持 **SSE 流式输出**、多会话管理、聊天历史持久化、Thinking Mode、文件内容注入，并提供一个独立的配置工具。

项目依赖 [libcurl](https://curl.se/libcurl/)、[cJSON](https://github.com/DaveGamble/cJSON) 和 [isocline](https://github.com/daanx/isocline)（终端行编辑库）。

---

## 功能特性

- 在终端中与 DeepSeek 对话，回答**流式打印**，无需等待完整响应
- 实时显示模型的 reasoning content（思考过程）
- 支持开启 / 关闭 Thinking Mode
- 多会话管理：创建、切换、查看、删除，每个会话拥有独立的 System Prompt 和上下文
- 聊天历史以 JSON 格式持久化到本地，退出时自动保存
- `$>file` 可将文本文件内容注入对话并直接发送
- 独立的 `deepseek-config` 配置工具，用于设置 API Key、模型和思考模式
- 命令行编辑能力（isocline）：光标移动、历史回溯、`Ctrl-R` 搜索、多行输入等
- 单实例运行检测（lock 文件），避免多个进程同时读写历史
- 常见 HTTP 错误（400 / 401 / 402 / 422 / 429 / 500 / 503）与网络故障的中文提示

---

## 依赖与安装

| 依赖 | 用途 | Debian / Ubuntu 包名 |
| --- | --- | --- |
| C / C++ 编译器 | 构建 | `build-essential` |
| libcurl | 调用 DeepSeek API | `libcurl4-openssl-dev` |
| cJSON | 解析和生成 JSON | `libcjson-dev` |
| isocline | 终端行编辑 | 需从源码编译安装 |

其他发行版：

```bash
# Fedora
sudo dnf install gcc gcc-c++ libcurl-devel cjson-devel

# Arch Linux
sudo pacman -S base-devel curl cjson
```

### 安装 isocline

多数发行版没有 isocline 的软件包，需要从源码构建：

```bash
git clone https://github.com/daanx/isocline
cd isocline
mkdir -p build/release && cd build/release
cmake ../..
cmake --build .

# 安装头文件和静态库到 /usr/local
sudo cp ../../include/isocline.h /usr/local/include/
sudo cp libisocline.a /usr/local/lib/
sudo ldconfig
```

也可以直接把 `isocline/src/isocline.c` 加入本项目的编译命令，无需安装。

---

## 构建

在项目根目录执行：

```bash
# 主程序
g++ -std=c++17 main.cpp deepseek.cpp \
    -lcurl -lcjson -lisocline \
    -o deepseek

# 配置工具
gcc config.c -lcjson -o deepseek-config
```

如果库或头文件安装在非标准路径，请按实际环境补充 `-I` / `-L` 参数。

---

## 快速开始

```bash
# 1. 创建数据目录（必须，程序不会自动创建）
mkdir -p ~/.deepseek

# 2. 配置 API Key、模型和思考模式
./deepseek-config

# 3. 启动
./deepseek
```

启动后会显示当前配置，并打印当前会话的历史消息（不包含 System Prompt）：

```text
DeepSeek CLI
当前模型: deepseek-flash
思考模式: 开
输入'$>help'了解更多命令！
```

---

## 配置

`deepseek-config` 提供如下菜单：

```text
a)设置api_key                 b)设置模型
c)设置思考模式                 q)保存并退出
```

| 配置项 | 可选值 |
| --- | --- |
| `api_key` | DeepSeek API Key |
| `model` | `deepseek-flash`（主力）、`deepseek-v4-pro`（即将淘汰） |
| `thinking_type` | `enabled`（开启）、`disabled`（关闭） |

配置保存在 `~/.deepseek/config`，格式如下：

```json
{
    "api_key": "sk-xxxxxxxxxxxxxxxx",
    "model": "deepseek-flash",
    "thinking_type": "enabled"
}
```

> **注意**
>
> API Key 以**明文**形式存储在本地配置文件中，请勿将 `~/.deepseek/config` 上传到任何仓库。
> 建议同时收紧文件权限：`chmod 600 ~/.deepseek/config`。

---

## 使用

启动 `./deepseek` 后，直接输入内容并回车即可发送消息：

```text
>>你好
正在思考中
思考过程
（reasoning content 实时输出）

回答:
你好！有什么我可以帮助你的？
```

说明：

- `正在思考中`、`思考过程`、`回答:` 以及各类错误提示均以红色显示
- Thinking Mode 开启时才会打印 `思考过程` 及其内容，内容本身随流式响应实时刷新
- 如果本次请求失败，会输出对应的错误原因；该条用户消息仍会保留在会话中
- 输入 `Ctrl-D` 或 `$>exit` 退出，历史消息在退出时统一写回文件

---

## 内置命令

以 `$>` 开头的输入会被识别为命令：

| 命令 | 说明 |
| --- | --- |
| `$>help` | 显示命令帮助 |
| `$>msg` | 打印当前会话的历史消息（不含 System Prompt） |
| `$>new` | 创建新会话并自动切换 |
| `$>all` | 列出所有会话关键字，`*` 标记当前会话 |
| `$>switch` | 切换到指定会话 |
| `$>del` | 删除指定会话 |
| `$>file <path>` | 读取文件内容并作为用户消息发送 |
| `$>exit` | 保存历史并退出 |

### `$>new` 创建会话

```text
>>$>new
请输入新对话的关键字(空行以取消)>linux
请输入新对话中DeepSeek的设定(空行以使用默认设定)>你是一名Linux系统工程师
已创建新对话并切换
```

- 关键字为空时取消创建，且不会写入任何内容
- 关键字与已有会话重复时提示 `关键字冲突！`
- System Prompt 为空时使用默认设定：`你是一个乐于助人的助手`
- 创建成功后自动切换到新会话

### `$>all` 查看所有会话

```text
>>$>all
*[1]:默认
 [2]:linux
```

行首的 `*` 表示当前所在会话。

### `$>switch` 切换会话

```text
>>$>switch
*[1]:默认
 [2]:linux
请输入关键字(空行以取消)>linux
切换成功
```

### `$>del` 删除会话

```text
>>$>del
*[1]:默认
 [2]:linux
输入关键字(空行以取消)>linux
删除成功
```

- 删除当前会话时自动切换到第一个会话
- 若删除了最后一个会话，程序会自动重新创建默认会话

### `$>file` 发送文件内容

```text
>>$>file /home/user/main.c
正在思考中

回答:
...
```

- 文件内容会以 `路径:\n文件内容` 的形式作为一条用户消息加入上下文，并**立即发送**给模型
- 支持自动跳过 UTF-8 BOM；文件不存在时会发送 `文件不存在` 文本
- 文件路径长度需小于 256 字节，且**不支持包含空格**的路径（路径按空白字符切分）
- 仅面向文本文件，不适合上传二进制文件

### `$>exit` 退出

```text
>>$>exit
```

正常退出时会把所有会话写回 `~/.deepseek/history`，并删除锁文件。

---

## 数据文件

程序使用以下文件（均位于 `~/.deepseek/`）：

| 文件 | 说明 |
| --- | --- |
| `config` | API Key、模型、Thinking Mode |
| `history` | 所有会话及其聊天历史（JSON 数组） |
| `lock` | 运行时创建的锁文件，正常退出时删除 |

`history` 的结构大致如下：

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
                "content": "你好"
            },
            {
                "role": "assistant",
                "content": "你好！有什么我可以帮助你的？"
            }
        ]
    }
]
```

- 每个会话由 `key_word` 和 `messages` 组成，不同会话的上下文相互独立
- `messages` 的第一条固定为 `system` 消息
- 删除会话后，其历史会在下次退出写回时从 `history` 中移除

---

## 实现说明

- 请求地址：`https://api.deepseek.com/chat/completions`，使用 `POST` 与 Bearer Token 认证
- 请求体包含 `model`、`thinking.type`、`messages`，并将 `stream` 设为 `true`
- HTTP 响应按 SSE 解析：读取 `data: ` 前缀的事件，遇到 `[DONE]` 结束，增量提取 `delta.reasoning_content` 与 `delta.content`
- 网络层使用 libcurl，并在 `WRITEFUNCTION` 回调中边接收边解析、边打印
- 会话数据在内存中用 cJSON 维护，程序退出时统一序列化写回文件

---

## 项目结构

```text
deepseek-cli/
├── main.cpp      # 程序入口：isocline 读行循环
├── deepseek.hpp  # 类声明（NetWork / Parse / Conversation / Memory / DeepSeek）
├── deepseek.cpp  # 网络请求、SSE 解析、会话管理与命令解析
├── config.c      # deepseek-config 配置工具
├── README.md
└── .gitignore
```

---

## 常见问题与已知限制

**启动时提示“请使用deepseek-config配置！”**
`~/.deepseek/config` 不存在或为空。请先执行 `mkdir -p ~/.deepseek && ./deepseek-config`。

**启动时提示“其他进程正在使用DeepSeek CLI”**
`~/.deepseek/lock` 残留（例如上次异常退出）。确认没有其他实例在运行后删除：

```bash
rm ~/.deepseek/lock
```

**请求失败后的上下文**
请求失败时，刚发送的用户消息已经写入内存中的会话。网络恢复后重试会再次追加同样的内容，可能导致历史中出现重复消息，可用 `$>new` 重建会话或手动编辑 `history`。

**错误提示对照**

| 提示 | 含义 |
| --- | --- |
| 无法连接服务器 | 网络不可达或 DNS / 代理问题 |
| API Key错误 | `api_key` 无效 |
| 余额不足 | 账户额度不足 |
| 请求体格式错误 / 请求体参数错误 | 请求 JSON 或参数不合法 |
| 请求过快 | 触发限流，需稍后重试 |
| 服务器故障 / 服务器繁忙 | 服务端异常，稍后重试 |

**其他**

- 程序依赖 DeepSeek API，需要可用的网络连接、有效的 API Key 以及可用额度
- 只有在 API 返回 200 时，助手的回复才会写入会话
- 不支持包含空格的 `$>file` 路径，且路径长度受 256 字节限制
- 仅支持 Linux / Unix 终端环境

---

## 开发状态

DeepSeek Chat CLI 目前是一个轻量级 Linux 命令行客户端，用于学习和实践 C/C++、HTTP 网络编程、JSON 数据处理以及命令行应用程序设计，项目仍在持续开发中。
