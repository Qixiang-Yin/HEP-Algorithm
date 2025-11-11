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
