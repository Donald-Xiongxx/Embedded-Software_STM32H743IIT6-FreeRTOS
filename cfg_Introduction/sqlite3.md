# SQLite3 静态库编译指南 (STM32H743)

## 1. 项目概述

本项目为 STM32H743IIT6 微控制器编译 SQLite3 静态库（完整版）。

| 项目 | 说明 |
|------|------|
| 目标芯片 | STM32H743IIT6 (Cortex-M7, 2MB Flash, 1MB RAM) |
| 编译器 | ARMCLANG 6.24 (MDK-ARM Plus) |
| SQLite版本 | 3.51.1 |
| 输出格式 | Keil静态库 (.lib) |
| 版本类型 | **完整版** (包含所有功能) |

## 2. 当前编译配置

### 2.1 编译器选项 (Keil UVprojx)

```xml
<Optim>3</Optim>              <!-- 优化级别: 3 (平衡优化) -->
<OneElfS>1</OneElfS>       <!-- 每个函数一个ELF段 -->
<MiscControls></MiscControls> <!-- 无额外编译器参数 -->
```

### 2.2 SQLite编译宏定义

```c
SQLITE_OS_OTHER=1        // 嵌入式平台必需
SQLITE_THREADSAFE=0      // 单线程模式
```

## 3. 包含的功能

完整版SQLite3包含以下所有功能：

### 3.1 数据库核心
- ✅ 完整SQL支持 (SELECT, INSERT, UPDATE, DELETE等)
- ✅ 事务处理 (BEGIN, COMMIT, ROLLBACK)
- ✅ 索引和查询优化
- ✅ 触发器 (TRIGGER)
- ✅ 视图 (VIEW)
- ✅ 子查询和联接
- ✅ 公共表表达式 (CTE/WITH)
- ✅ 窗口函数 (Window Functions)

### 3.2 高级功能
- ✅ JSON函数 (json_extract, json_object等)
- ✅ FTS5全文搜索 (CREATE VIRTUAL TABLE ... USING fts5)
- ✅ 虚拟表 (Virtual Tables)
- ✅ WAL模式 (Write-Ahead Logging)
- ✅ 自动增量 (AUTOINCREMENT)
- ✅ 外键约束 (FOREIGN KEY)
- ✅ 约束检查 (CHECK)
- ✅ 分析命令 (ANALYZE)

### 3.3 接口
- ✅ 扩展加载 (load_extension)
- ✅ sqlite3_trace()
- ✅ sqlite3_get_table()
- ✅ sqlite3_create_function()
- ✅ sqlite3_serialize()/deserialize()
- ✅ 编译选项诊断 (sqlite3_compileoption_used)

## 4. 编译结果

### 4.1 预期结果

- **库文件**: `Build\libsqlite3.lib`
- **预期大小**: ~8-12 MB (完整版)
- **编译时间**: 约30-60秒

### 4.2 警告说明

完整版编译应该**没有警告**，所有函数都被正确定义和调用。

## 5. 注意事项

### 5.1 必须初始化

```c
#include "sqlite3.h"

int main(void) {
    // 必须先初始化
    sqlite3_initialize();
    
    // 然后使用SQLite
    sqlite3 *db;
    sqlite3_open("test.db", &db);
    // ...
    
    return 0;
}
```

### 5.2 自定义VFS

由于使用 `SQLITE_OS_OTHER=1`，必须提供自定义VFS实现：

```c
// 必须实现的函数
int sqlite3_os_init(void);   // 注册VFS
int sqlite3_os_end(void);    // 清理

// 典型实现
int sqlite3_os_init(void) {
    return sqlite3_vfs_register(&my_vfs, 1);
}
```

### 5.3 线程安全

使用 `SQLITE_THREADSAFE=0` 意味着：
- ❌ 不支持多线程并发访问同一数据库
- ✅ 单线程访问性能最优
- ✅ 代码体积更小

## 6. 进一步优化

### 6.1 LTO (Link-Time Optimization)

启用后可能减小体积，但编译时间显著增加：

```xml
<v6Lto>1</v6Lto>
<Optim>4</Optim>
```

### 6.2 体积优化版本

如需减小体积，可选择性禁用某些功能：

```c
// 体积优化配置示例
SQLITE_OS_OTHER=1
SQLITE_THREADSAFE=0
SQLITE_OMIT_WAL=1              // 禁用WAL模式
SQLITE_OMIT_VIRTUALTABLE=1     // 禁用虚拟表
SQLITE_OMIT_JSON=1             // 禁用JSON函数
```

## 7. 使用方法

### 7.1 在Keil项目中添加库

1. 将 `Build\libsqlite3.lib` 复制到目标项目
2. 在目标项目 `Options -> Linker -> Library` 中添加
3. 添加头文件路径: `.\sqlite3.h`

### 7.2 包含头文件

```c
#include "sqlite3.h"

#pragma comment(lib, "libsqlite3.lib")
```

## 8. Flash/RAM占用估算

| 组件 | 估算大小 |
|------|---------|
| SQLite核心 | 1.0-1.5 MB |
| JSON支持 | 200-400 KB |
| FTS5全文搜索 | 500-800 KB |
| 虚拟表框架 | 300-500 KB |
| WAL支持 | 200-300 KB |
| 其他功能 | 500-800 KB |
| **总计** | **约2.7-4.3 MB** |

> 注：实际占用取决于使用的功能，未使用的代码会被链接器丢弃。

## 9. 参考资料

- [SQLite官方编译选项文档](https://www.sqlite.org/compile.html)
- [SQLite官方编译指南](https://www.sqlite.org/howtocompile.html)
- [SQLite Amalgamation下载](https://www.sqlite.org/download.html)
- STM32H743IIT6数据手册

## 10. 版本记录

| 日期 | 版本 | 修改内容 |
|------|------|---------|
| 2026-04-06 | 1.0 | 完整版配置，基于SQLite 3.51.1 |

---

**说明**: 本配置编译完整版SQLite3静态库，包含所有官方功能。如需体积优化版本，可参考第6.2节选择性禁用功能。
