# 证券考题批量生成 Demo (card-batch-demo)

> 离线批量生成证券从业资格考试的配对学习卡片（连连看式），用 LLM（DeepSeek）
> 生成 → 人工审核 → 发布给游戏前端。

---

## 特性

- **批量生成**：10 个固定知识点，每个生成 5 对配对卡片 = 50 张
- **严格 JSON**：`response_format: json_object` + 自愈解析，绕开 provider 的偶发破 JSON
- **指数退避重试**：默认 3 次（1s → 2s → 4s），避免瞬时网络抖动
- **断点续跑**：cards.json 既是结果也是 checkpoint，crash 后自动跳过已完成的
- **审核状态**：每张卡可标 `pending / approved / rejected / needs_revision`
- **发布过滤**：`publish` 命令只把 approved 的卡写进 `published.json`
- **环境变量配置**：API key 走 env，不进代码

---

## 技术栈

- **C++17** on Linux (Ubuntu VM)
- **libcurl**：HTTP 请求
- **nlohmann/json**：JSON 解析与构造
- **CMake**：构建系统
- **DeepSeek API**（OpenAI 兼容）：内容生成

---

## 项目结构
- `interface/`：主代码目录
- `interface/test.cpp`：测试代码
- `interface/README.md`：项目说明
- `interface/Makefile`：构建脚本
- `interface/CMakeLists.txt`：CMake 配置
- `interface/include/`：头文件目录
- `interface/lib/`：库文件目录
- `interface/bin/`：可执行文件目录
- `interface/logs/`：日志目录
- `interface/cards.json`：生成的卡片 JSON 文件
- `interface/published.json`：已发布的卡片 JSON 文件
- `interface/retry.h`：重试策略头文件
- `interface/publish.h`：发布策略头文件
- `interface/provider.h`：提供者配置头文件
- `interface/knowledge_points.h`：知识点头文件
- `interface/batch_meta.h`：批次元数据头文件
- `interface/review.h`：审核头文件
- `interface/checkpoint.h`：检查点头文件
- `interface/publish.h`：发布头文件


---

## 快速开始

### 1. 装依赖

```bash
sudo apt update -y build- -y build-essential cmake libcurl4-openssl-dev nlohmann-json3-dev lib-dev

cd ~/card-batch-demo
echo 'DEEPSEEK_API_KEY=sk-你的真key' > .env
chmod 600 .env

