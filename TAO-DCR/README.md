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

**2025.11.11 Update**

增加批量刻度DCR的功能

首先编译离线软件`taosw`，执行`source taosw/complie.sh`

运行脚本`python download_dat.py`，从eos盘内下载所有DAT文件（DAQ Raw Data）：

```python
# Download DAT file from eos directory.

import os
import subprocess

server = "root://junoeos01.ihep.ac.cn/"
folder = "/eos/juno/tao-raw/global_trigger/00000000/00000500/514/"
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

赋予脚本权限`chmod +x raw2rtraw.sh`，运行脚本`./raw2rtraw.sh`，将DAT文件转换成Rtraw文件：

```bash
#!/bin/bash
# 批量生成并提交 TAO 数据转换作业

# ========== 基本配置 ==========
INPUT_DIR="./raw_data"
OUTPUT_DIR="./rtraw_data"
JOB_DIR="./r2r_job_lists"
RUN_SCRIPT="$TAO_OFFLINE_OFF/RawToRTRaw/TAORawToRTRaw/share/run.py"

# ========== 初始化 ==========
mkdir -p "$OUTPUT_DIR"
mkdir -p "$JOB_DIR"

echo "输入目录: $INPUT_DIR"
echo "输出目录: $OUTPUT_DIR"
echo "作业脚本目录: $JOB_DIR"
echo ""

# ========== 循环生成作业脚本 ==========
for file in "$INPUT_DIR"/*.dat; do
    # 获取文件名（不带路径）
    filename=$(basename "$file")

    # 提取 RUN 编号和三位数字编号
    run_num=$(echo "$filename" | grep -oE 'RUN\.[0-9]+' | cut -d'.' -f2)
    number=$(echo "$filename" | grep -oE '[0-9]{3}\.dat$' | cut -d'.' -f1)

    # 构造作业脚本与输出文件名
    job_name="job_${run_num}_${number}.sh"
    job_path="${JOB_DIR}/${job_name}"
    output_file="${OUTPUT_DIR}/rtraw_${run_num}_${number}.root"

    # 写入作业脚本内容
    cat > "$job_path" <<EOF
#!/bin/bash
# ====== Job Script: $filename ======

python $RUN_SCRIPT \\
    --output-elec "$output_file" \\
    --evtmax -1 \\
    --input "$file"

echo "Finished processing: $filename"
EOF

    # 赋予执行权限
    chmod +x "$job_path"

    echo "Created: $job_path"
done

echo ""
echo "所有作业脚本已生成，开始提交到集群..."

# ========== 提交所有作业 ==========
for job_script in "$JOB_DIR"/job_*.sh; do
    echo "Submitting $job_script ..."
    hep_sub "$job_script"
done

echo ""
echo "全部作业已成功提交！"
```

刻度DCR，可以选择两种方式：

（1）在`DCR_Calib.C`中修改输入Rtraw文件地址与输出ROOT文件地址，执行命令`root -l -q 'DCR_Calib.C()'`开始刻度；

（2）向集群交作业，赋予脚本权限`chmod +x dcr_job.sh`，运行脚本`./dcr_job.sh`：

```bash
#!/bin/bash
# ===========================================
# 批量生成并提交 DCR_Calib ROOT 作业
# ===========================================

# ========== 基本配置 ==========
INPUT_DIR="./rtraw_data"            # 输入文件目录
JOB_DIR="./dcr_job_lists"           # 作业脚本存放目录
ROOT_MACRO="./DCR_Calib.C"          # ROOT 宏路径
SUBMIT_CMD="hep_sub"                # 提交命令 (如用 SLURM 改为 sbatch)

# ========== 初始化 ==========
mkdir -p "$JOB_DIR"

echo "输入目录: $INPUT_DIR"
echo "作业脚本目录: $JOB_DIR"
echo ""

# 检查输入目录
if [ ! -d "$INPUT_DIR" ]; then
    echo "输入目录不存在: $INPUT_DIR"
    exit 1
fi

# ========== 循环生成作业脚本 ==========
for file in "$INPUT_DIR"/rtraw_*.root; do
    [ -e "$file" ] || { echo "未找到任何 .root 文件"; exit 1; }

    filename=$(basename "$file")                 # rtraw_514_001.root
    base="${filename%.*}"                        # rtraw_514_001
    run_num=$(echo "$filename" | cut -d'_' -f2)  # 514
    file_idx=$(echo "$filename" | cut -d'_' -f3 | cut -d'.' -f1)  # 001

    # 作业脚本路径
    job_name="job_${run_num}_${file_idx}.sh"
    job_path="${JOB_DIR}/${job_name}"

    # 写入作业脚本
    cat > "$job_path" <<EOF
#!/bin/bash
# ====== Job Script: $filename ======

echo "开始处理文件: $file"
root -l -q '${ROOT_MACRO}("${file}")'
echo "完成处理: $file"
EOF

    chmod +x "$job_path"
    echo "Created job script: $job_path"
done

echo ""
echo "所有作业脚本已生成，开始提交到集群..."

# ========== 提交所有作业 ==========
for job_script in "$JOB_DIR"/job_*.sh; do
    echo "Submitting $job_script ..."
    $SUBMIT_CMD "$job_script"
done

echo ""
echo "全部作业已成功提交！"
```

会把`./rtraw_data`文件夹下的全部Rtraw文件作为输入刻度DCR。

`Channel_Analysis.C`文件用于查看某一通道的电子学数据，用于异常分析。
