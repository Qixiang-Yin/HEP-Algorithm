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