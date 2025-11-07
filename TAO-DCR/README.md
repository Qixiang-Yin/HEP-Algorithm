### README

该算法将原始的RAW文件转换为RTRaw文件，即电子学格式文件，用于后续的分析。首先编译离线软件`taosw`，执行`source taosw/complie.sh`：

```bash
source setup_J25-1-0.sh
./build.sh
source setup.sh
```

运行脚本`python download_dat.py`，从eos盘内下载DAT文件：

```python
# Download DAT file from eos directory.

import os
import subprocess

server = "root://junoeos01.ihep.ac.cn/"
folder = "/eos/juno/tao-raw/global_trigger/00000000/00000200/242/"
file_name = "RUN.242.TEST.TEST.ds-0.global_trigger.20250915171305.001.dat"

file_full_path = f"{server}{os.path.join(folder, file_name)}"

cmd = ["xrdcp", file_full_path, "./"]
subprocess.run(cmd, check=True)
```

或者下载一个文件夹中的所有文件：

```python
# Download DAT file from eos directory.

import os
import subprocess

server = "root://junoeos01.ihep.ac.cn/"
folder = "/eos/juno/tao-raw/global_trigger/00000000/00000400/411/"
local_dir = "./raw_data"  # 指定本地保存路径

# 创建本地文件夹（如果不存在）
os.makedirs(local_dir, exist_ok=True)

# 1. 列出远程目录下所有文件
cmd_ls = ["xrdfs", server, "ls", folder]
result = subprocess.run(cmd_ls, stdout=subprocess.PIPE, check=True, text=True)

# 2. 逐个下载 .dat 文件
for line in result.stdout.strip().split("\n"):
    if line.endswith(".dat"):
        file_full_path = f"{server}{line}"
        print(f"Downloading {file_full_path} ...")
        subprocess.run(["xrdcp", file_full_path, local_dir], check=True)

print("所有 .dat 文件已下载到:", os.path.abspath(local_dir))
```

运行脚本`source run.sh`，将DAT文件转换成ROOT文件：

```bash
python $TAO_OFFLINE_OFF/RawToRTRaw/TAORawToRTRaw/share/run.py --output-elec /path/to/taosw/MyJob/Test/output/rtraw_data.root --evtmax -1 --input /path/to/taosw/MyJob/Test/input/RUN.242.TEST.TEST.ds-0.global_trigger.20250915171305.001.dat
```

使用宏`Example.C`读取RTraw文件的数据。首先设置TAO环境：

```bash
source /path/to/taosw/setup.sh
```

执行脚本：`root -l Example.C`，生成存储电子学信息的`output.root`文件。
