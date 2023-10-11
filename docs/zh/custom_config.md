
## 1. 前言
tRPC-Cpp框架配置系统默认提供了一套键值（KV）型配置插件（DefaultConfig）以满足大部分配置管理需求。为了让开发者能够更灵活地处理配置数据，配置系统采用插件化设计，允许开发者基于DefaultConfig插件实现自定义的数据源Provider插件和编解码器Codec插件。

配置系统的设计架构如下图所示：
![enter image description here](../../docs/images/config_design.png)
-  `Provider` 数据源插件：负责从不同类型的数据源（如文件、数据库）读取原始配置数据。
-  `Codec` 编解码器插件：负责将原始配置数据解码为键值对形式。
-  `DefaultConfig` 配置插件：提供统一的接口来访问解码后的配置数据。

数据驱动过程：
1. 用户调用 `Load` 函数。
2.  `Load` 函数从指定的 `Provider` 插件读取原始配置数据。
3.  `Load` 函数将原始配置数据传递给指定的 `Codec` 插件进行解码。
4. 解码后的配置数据存储在 `DefaultConfig` 配置插件中，供用户访问。

tRPC-Cpp 配置系统功能概述：
1. **提供默认加载器Load和默认的KV配置插件（DefaultConfig）**：用户可以调用 `Load` 函数，根据指定的数据源路径、数据源名称和解码器加载配置文件。 `Load` 函数返回一个 `DefaultConfigPtr` ，用户可以通过这个指针访问配置数据。
2. **内置数据源插件**：tRPC-Cpp框架默认提供了 `LocalFileProvider` 数据源插件，支持从本地文件系统读取配置文件。插件开发者可以实现 `trpc::config::Provider` 接口创建自定义数据源插件。
3. **内置编解码插件**：tRPC-Cpp框架默认提供了YAML、JSON和TOML三种编解码插件。插件开发者可以实现 `trpc::config::Codec` 接口创建自定义编解码插件。

用户可以使用 `tRPC-Cpp` 框架默认提供的数据源插件和编解码插件，或根据需要开发自定义插件。

本文档将指导开发者如何开发和注册自定义数据源 `Provider` 插件和编解码器 `Codec` 插件。


## 2. 开发自定义数据源Provider插件

要实现一个自定义的数据源 `Provider` 插件，您需要完成以下步骤：
### 2.1 实现Provider基类接口

首先，插件开发者需要创建一个类，继承自 `trpc::config::Provider` 接口，并实现接口中的以下方法：
| 方法名称 | 返回类型 | 描述 |
|---------|---------|------|
| std::string Name() const | std::string | 返回数据源插件的名称。 |
| std::string Read(const std::string&) | std::string | 从数据源中读取配置文件内容。 |
| void Watch(trpc::config::ProviderCallback callback) | void | 监听数据源中配置文件的变化，并在发生变化时调用回调函数。 |
以下是一个自定义数据源 `Provider` 插件的示例：
**custom_provider.h**

```cpp
#pragma once

#include <string>
#include <mutex>

#include "trpc/config/provider.h"

namespace trpc::config::custom_provider {

class CustomProvider : public trpc::config::Provider {
 public:
  CustomProvider(); // 构造函数，可以根据需要添加参数

  std::string Name() const override;

  std::string Read(const std::string& name) override;

  void Watch(trpc::config::ProviderCallback callback) override;

 private:
  std::string name_; // 数据源插件的名称
  std::mutex callback_mutex_; // 用于保护回调函数列表的互斥锁
  std::vector<trpc::config::ProviderCallback> callbacks_; // 回调函数列表
};

}  // namespace trpc::config::custom_provider
```
**custom_provider.cc**

```cpp 
#include "custom_provider.h"

namespace trpc::config::custom_provider {

CustomProvider::CustomProvider() {
  // 初始化操作，如连接远程服务器、读取配置文件等
}

std::string CustomProvider::Name() const {
  return name_;
}

std::string CustomProvider::Read(const std::string& name) {
  // 从数据源中读取配置文件内容，并返回
  // 这里可以根据实际情况实现，如从远程服务器获取数据、从数据库中查询等
}

void CustomProvider::Watch(trpc::config::ProviderCallback callback) {
  std::unique_lock callback_lock(callback_mutex_);
  callbacks_.emplace_back(callback);
  // 监听数据源中配置文件的变化
  // 当配置文件发生变化时，调用回调函数列表中的所有回调函数
}

}  // namespace trpc::config::custom_provider
```
在这个示例中， `CustomProvider` 类继承自 `trpc::config::Provider` 接口，并实现了 `Name()` 、 `Read()` 和 `Watch()` 方法。开发者可以根据实际需求修改这个示例，以实现对自定义数据源的支持。

### 2.2 注册自定义Provider插件

创建完成自定义的 `Provider` 插件后，还需要提供一个初始化接口，例如 `Init()` 。推荐在 `custom_provider_api.h` 中定义接口，然后在 `custom_provider_api.cc` 中实现。例如：
**custom_provider_api.h**

```cpp
#pragma once

namespace trpc::config::custom_provider {

int Init();

}  // namespace trpc::config::custom_provider
```
**custom_provider_api.cc**
```cpp
#include "trpc/common/trpc_plugin.h"
#include "custom_provider_api.h"
#include "custom_provider.h"

namespace trpc::config::custom_provider {

int Init() {
  auto custom_provider = MakeRefCounted<CustomProvider>();
  TrpcPlugin::GetInstance()->RegisterConfigProvider(custom_provider);
  return 0;
}

}  // namespace trpc::config::custom_provider
```


## 3. 开发自定义编解码器Codec插件

要实现一个自定义的编解码器 `Codec` 插件，需要完成以下步骤：
### 3.1 实现Codec基类接口

首先，插件开发者需要创建一个类，继承自 `trpc::config::Codec` 接口，并实现接口中的以下方法：

|方法名称 |返回类型 |描述 |
|:--|
|std::string Name() const |std::string |返回编解码器插件的名称 |
|std::unordered_map<std::string, std::string> Decode(const std::string& content) |std::unordered_map<std::string, std::string> |将从数据源读取到的配置文件内容解码为键值对形式的 std::unordered_map  |
以下是一个自定义编解码器 `Codec` 插件的示例：
**custom_codec.h**

```cpp 
#pragma once

#include <string>
#include <unordered_map>

#include "trpc/config/codec.h"

namespace trpc::config::custom_codec {

class CustomCodec : public trpc::config::Codec {
 public:
  CustomCodec() {
    // 构造函数，可以根据需要添加参数
    // 初始化操作，如加载解码器库等
  }

  std::string Name() const override {
    return name_;
  }

  std::unordered_map<std::string, std::string> Decode(const std::string& content) override {
    // 将从数据源读取到的配置文件内容解码为键值对形式的 std::unordered_map
    // 这里可以根据实际情况实现，如调用第三方库进行解码等
  }

 private:
  std::string name_; // 编解码器插件的名称
};

}  // namespace trpc::config::custom_codec
```

**custom_codec_api.cc**

```cpp 
#include "trpc/common/trpc_plugin.h"
#include "custom_codec_api.h"
#include "custom_codec.h"

namespace trpc::config::custom_codec {

int Init() {
  auto custom_codec = MakeRefCounted<CustomCodec>();
  TrpcPlugin::GetInstance()->RegisterConfigCodec(custom_codec);
  return 0;
}

}  // namespace trpc::config::custom_codec
```
在上述示例中， `CustomCodec` 是实现的自定义编解码器插件类。用户需要显式调用 `Init()` 函数以完成插件的注册。

### 3.2 注册Codec插件

创建完成自定义的 `Codec` 插件后，插件开发者需要提供一个初始化接口，例如 `Init()` 。推荐在 `custom_codec_api.h` 中定义接口，然后在 `custom_codec_api.cc` 中实现。例如：
**custom_codec_api.h**

```cpp 
#pragma once

namespace trpc::config::custom_codec {

int Init();

}  // namespace trpc::config::custom_codec
```

**custom_codec_api.cc**

``` 
#include "trpc/common/trpc_plugin.h"
#include "custom_codec_api.h"
#include "custom_codec.h"

namespace trpc::config::custom_codec {

int Init() {
  auto custom_codec = MakeRefCounted<CustomCodec>();
  TrpcPlugin::GetInstance()->RegisterConfigCodec(custom_codec);
  return 0;
}

}  // namespace trpc::config::custom_codec
```


## 4. 用户使用自定义插件

在开发和注册完成自定义的数据源 `Provider` 插件和编解码器 `Codec` 插件后，用户需要在框架初始化完毕之后调用相应的 `Init()` 函数以完成插件的注册。用户可以在调用 `trpc::config::Load` 函数时指定使用自定义插件。
|参数名称 |类型 |描述 |
|:--|
|path |std::string |配置文件在数据源中的路径。 |
|WithCodec |LoadOptions |使用的解码器。用户需要通过 `trpc::config::WithCodec` 函数指定解码器名称，如"custom_codec"。 |
|WithProvider |LoadOptions |数据源名称。用户需要通过 `trpc::config::WithProvider` 函数指定数据源名称，如"custom_provider"。 |
例如，用户可以使用以下代码加载一个自定义格式的配置文件：
```cpp
#include "custom_provider_api.h"
#include "custom_codec_api.h"

// 初始化自定义插件
trpc::config::custom_provider::Init();
trpc::config::custom_codec::Init();

// 使用自定义插件加载配置文件
DefaultConfigPtr config = trpc::config::Load("./test_load.custom", trpc::config::WithCodec("custom_codec"), trpc::config::WithProvider("custom_provider"));

// 获取配置
int32 value = config->GetInt32("integer", 0);
std::string str = config->GetString("string", "");
```
在这个示例中，用户指定了配置文件的路径为 `./test_load.custom` ，使用的解码器为 `custom_codec` ，数据源名称为 `custom_provider` 。 `Load` 函数将返回一个 `DefaultConfigPtr` ，用户可以通过这个指针访问配置数据。
