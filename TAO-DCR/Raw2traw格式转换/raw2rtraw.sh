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
