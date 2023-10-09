English | [中文](DEVELOP_SPECIFICATIONS.zh_CN.md)

# Develop Specification

## Coding-style

Following the [Google C++ coding style](https://google.github.io/styleguide/cppguide.html) in general, some differences and considerations are explain below.

### Copyright statement

All code header files and source files need to have a version declaration, please refer to [Copyright](CONTRIBUTING.md#copyright-headers)

### Line length

The maximum code line length used by the framework is 120 characters.

### Comment

The framework supports using [doxygen](https://www.doxygen.nl/) to generate API documentation, so there are certain requirements for the commenting style in the framework code. Based on whether documentation comments are needed, we can categorize comments into the following two types.

#### Documentation comments

Scope: APIs provided for user access, such as public and protected methods and members of classes and structs, global functions, etc.

Commenting style: Consistently use the single-line commenting style of doxygen javadoc.

Examples of typical annotation scenarios:

- Functions

```
/// @brief Check if an address is an IPv4 address.
/// @param addr Ip address.
/// @return Return true if addr is a ipv4 address, otherwise return false.
bool IsIpv4(const std::string& addr);
```

- Structs and class members

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

- Global variable

```
/// Max number of token.
const int kMaxTokenNum = 10; 
```

Some common doxygen annotation commands are listed as following:

```
@brief Function or class summary
@param[in|out] arg Parameter description
@tparam[in|out] arg Template parameter description
@return [Return type (e.g., bool, int)] Explanation
@note Notes
@code(Must be ended with @endcode)
Example code (no indentation required)
@endcode
@see Indicates some related class or interface documentation, similar to 'see also' in man pages
@private Indicates that the member documented by the comment block is private.
```
See [Special Commands](https://www.doxygen.nl/manual/commands.html) for more annotation commands.

Additional Notes：
1. If there are struct/class, global functions, or public functions of a class in the external API header file that do not need to be exposed externally, it is recommended to use the `/// @private` tag to exclude them from the doxygen documentation.
2. If the interface has been deprecated, in addition to adding the deprecated compilation attribute, it is recommended to add corresponding instructions in the documentation comments, such as `/// @brief Deprecated: use xxx() instead. Interface description` so that it can be displayed directly in the API document that it's deprecated.

#### Non-documentation comments
Scope: APIs not provided for user access, such as classes and interfaces that only used internally by the framework, private methods and members of all classes, explanations for a specific code segment, etc.

Commenting style: Use "//" or "/**/" comments in the C++ language.

## Commit specification

See [Writing good commit messages](./CONTRIBUTING.md#writing-good-commit-messages).

## Versioning specification

### Version number specification

The framework version number follows the [SemVer](https://semver.org/) specification.

The released version number consists of "MAJOR[Major version].MINOR[Minor version].PATCH[Patch version]":

1. MAJOR version when you make incompatible API changes
2. MINOR version when you add functionality in a backward compatible manner
3. PATCH version when you make backward compatible bug fixes

### Version maintenance

As the framework continues to iterate, the number of framework versions will increase. To facilitate version management and code maintenance, the framework version is maintained as follows:

For the major version: Maintain the latest 2 major versions. After a major version is no longer iterated, it will be maintained for 1 year, focusing only on bug fixes for its latest minor version.
For example, if the latest major version is 4, versions 3-4 will be maintained, with version 3 being maintained for 1 year after it is no longer iterated.

For the minor and patch versions: In principle, maintain the latest 2 minor versions under the major version. This mainly involves fixing known bugs and issues in these 2 versions and updating the patch version. Additionally, tRPC in different languages may consider adding more version maintenance based on actual usage, such as maintaining a version that has a large user base even if it is not within the latest two versions.

Therefore, when choosing a tRPC version, it is recommended to select the latest 2 major versions, the latest 2 minor versions, and the latest 1 patch version.

## Security specification

See [Tencent/C,C++ security guide](https://github.com/Tencent/secguide/blob/main/C,C++%E5%AE%89%E5%85%A8%E6%8C%87%E5%8D%97.md)