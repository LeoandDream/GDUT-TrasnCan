UART 自动化测试脚本

文件:
- `uart_test.py` : 主脚本，位于 `tools/` 下
- `requirements.txt` : 依赖（pyserial）

快速开始：
1. 安装依赖（推荐使用 venv）：

```powershell
python -m venv .venv; .\.venv\Scripts\Activate.ps1; pip install -r tools\requirements.txt
```

2. 查看可用示例帧：

```powershell
python tools\uart_test.py --port COM5 --baud 115200 --list
```

3. 发送单帧并等待 MCU 输出比对：

```powershell
python tools\uart_test.py --port COM5 --baud 115200 --send one_obj --timeout 1.0
```

4. 连续发送所有示例帧 5 次，每次间隔 200ms：

```powershell
python tools\uart_test.py --port COM5 --baud 115200 --send all --repeat 5 --interval 0.2
```

说明：
- 脚本会查找 MCU 输出中是否包含与所发送帧相同的十六进制字节序列（形如 `AA BB 01 ...`），并尝试捕获 `[USART1]` 或 `RX_LEN` 等信息行。
- 若 MCU 在 ISR 中打印原始接收字节（如我们已修改 `host1_usart.c`），脚本会把这些行展示出来，便于人工或自动比对。
