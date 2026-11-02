# DeepSeek Harness Linux 桌面封装

这是 [DeepSeek Harness](https://github.com/deepseek-ai/deepseek-harness) 的社区 Linux amd64 桌面打包项目，使用 WebKitGTK 窗口运行 dsh Web 服务。它不是官方 Linux 桌面发行版。

## 同步和发布

1. 官方仓库更新后，打开 **Actions → Sync upstream → Run workflow**，选择 `main` 并运行。工作流会将官方 `master` 最新提交写入 `upstream-revision.txt`。等待运行成功。
2. 打开 **Actions → Build Linux desktop release → Run workflow**，选择 `main`，填写新版本号，例如 `1.0.0`。版本号只能使用 `数字.数字.数字`，且标签不能已存在。
3. 构建成功后，工作流会检查打包后的服务，再将 `.deb` 和 `SHA256SUMS` 发布到 Releases。Release 说明会记录上游提交和本项目提交。

本仓库不是 GitHub fork，因此使用 **Sync upstream** 工作流代替 GitHub 的 Sync fork 按钮。同步只是固定最新官方提交，不会自动发布。上游构建结构发生变化时，可能需要调整打包脚本。工作流需要 GitHub Actions 可运行，且需要 `contents: write` 才能同步和发布；首次使用时如有提示，请在仓库设置中启用 Actions。

## 安装

在 Release 下载 `.deb` 后，在兼容的 Debian/Ubuntu amd64 系统执行：

```bash
sudo apt install ./deepseek-harness_1.0.0_amd64.deb
deepseek-harness
```

用户数据和日志位于 `~/.dsh/`。安装包没有签名；校验和可验证下载完整性，不能证明发布者身份。上游版本与输入的打包版本相互独立。系统需要 WebKitGTK 4.1。

首次构建应检查 Actions 日志并实际启动应用：上游更新可能改变构建方式，自动服务检查不等于完整的桌面功能测试。

封装代码见 `desktop/`；上游代码使用其自身许可证，本项目封装代码许可证见 `desktop/LICENSE`。
