[English](DEVELOP_SPECIFICATIONS.md) | 中文

# 研发规范

## 代码规范
总体上遵循[Google C++代码规范](https://google.github.io/styleguide/cppguide.html)，下面对一些差异和注意事项等进行说明。

### 版权声明
所有的代码头文件及源文件都需要放置版本声明，具体见[版权声明](CONTRIBUTING.zh_CN.md#版权声明)。

### 代码行长度
框架使用的最大代码行长度为 120 characters。

### 注释

框架支持使用[doxygen](https://www.doxygen.nl/)来生成api文档，因此对框架代码注释风格有一定的要求。根据是否需要文档化注释，我们可以把注释分为如下两类。

#### 需要文档化的注释

范围：提供给用户使用的API，如Class及Struct的public、protected方法和成员，Global function等。

注释风格：统一使用doxygen javadoc style中的单行注释风格。

一些典型场景的注释示例如下：

- 函数
```
/// @brief Check if an address is an IPv4 address.
/// @param addr Ip address.
/// @return Return true if addr is a ipv4 address, otherwise return false.
bool IsIpv4(const std::string& addr);
```

- 结构体和类成员
```
/// @brief Location of a point.
struct Point {
  /// X-coordinate value
  float x = 0.0;
  /// Y-coordinate value
  float y = 0.0;
  /// Z-coordinate value
  float z = 0.0;
};
```

- Global变量

```
/// Max number of token.
const int kMaxTokenNum = 10; 
```

常用的一些doxygen注释命令：
```
@brief 函数或者类的简述
@param[in|out] arg 参数描述
@tparam[in|out] arg 模板参数描述
@return [返回类型(如bool、int)] 说明
@note 注意事项
@code(必须使用@endcode结束)
示例代码(无需缩进)
@endcode
@see 指明一些关联的类、接口说明文档，同man中的see also
@private 表示评论块记录的成员是私有的，在EXTRACT_PRIVATE=NO的情况下doxygen生产文档的时候不会包含评论块
```
更多的命令说明请参考[Special Commands](https://www.doxygen.nl/manual/commands.html)

补充说明：
1. 位于对外API头文件中，但不需要对外暴露的struct/class、全局函数及类public函数等，建议使用`/// @private`标识来将其排除在doxygen文档外。
2. 如果接口已弃用，除了增加deprecated编译attribute外，建议在文档注释中也增加对应的说明，如 `/// @brief Deprecated: use xxx() instead. 接口描述`，以便在API说明中能直接看出其为deprecated。

#### 不需要文档化的注释
范围：不提供给用户使用的API, 如仅用于框架内部使用的类及接口、所有类的private方法及成员、对某段代码的解释等。

注释风格：使用c++语言的"//"或者"/**/"注释即可。

## Commit规范

请参考[编写良好的提交消息](CONTRIBUTING_zh.md#编写良好的提交消息)。

## 版本规范

### 版本号规范
框架发布版本时，其版本号遵循[SemVer](https://semver.org/)规范。

发布的版本号由"MAJOR[主版本号].MINOR[次版本号].PATCH[修订版本号]"组成：

1. 主版本号：当做了不兼容的 API 修改
2. 次版本号：当做了向下兼容的功能性新增
3. 修订版本号：当做了向下兼容的问题修正

### 版本维护
随着框架的不断迭代，框架的版本号会越来越多，为了方便版本管理和代码维护，框架版本的维护方式如下：

对于主版本号：维护最新的2个的主版本号，主版本号不再迭代变更后，往后继续维护1年，只做其最新次版本号的bug修复。
例如：最新的主版本号是4的话，会维护主版本号3-4的版本，其中主版本3不再迭代变更后，往后继续维护1年。

对于次版本号和修订版本号：原则上维护主版本号下最新的2个次版本号，主要对这2个版本号已知bug和问题的修复，并更新修订版本号，同时tRPC各语言会根据实际情况是否增加其他更多版本维护，如某个版本使用的用户多即使在最近两个版本之前也考虑维护。

因此，用户在tRPC版本选择，建议选择主版本号最新的2个，次版本号选择最新的2个，修订版本号选择最新的1个。

## 安全规范

请参考[Tencent/C,C++安全指南](https://github.com/Tencent/secguide/blob/main/C,C++%E5%AE%89%E5%85%A8%E6%8C%87%E5%8D%97.md)。