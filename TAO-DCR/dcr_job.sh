#!/bin/bash
# ===========================================
# 批量生成并提交 DCR_Calib ROOT 作业
# by Qixiang Yin
# ===========================================

# ========== 基本配置 ==========
EOS_DIR="root://junoeos01.ihep.ac.cn//eos/juno/tao-rtraw/J25.6.0/gitVcd310cd8/CD/00000000/00000500/527"
JOB_DIR="./dcr_job_lists"
ROOT_MACRO="./DCR_Calib.C"
SUBMIT_CMD="hep_sub"

# ========== 初始化 ==========
mkdir -p "$JOB_DIR"
echo "输入目录: $EOS_DIR"
echo "作业脚本目录: $JOB_DIR"
echo ""

# ========== 获取EOS文件列表 ==========
echo "正在从EOS目录获取文件列表..."
EOS_PATH=$(echo "$EOS_DIR" | sed 's|^root://junoeos01.ihep.ac.cn||')
FILE_LIST=$(eos root://junoeos01.ihep.ac.cn ls "$EOS_PATH" | grep '\.rtraw$')

if [ -z "$FILE_LIST" ]; then
    echo "未找到任何 .rtraw 文件"
    exit 1
fi

# ========== 生成作业脚本 ==========
for fname in $FILE_LIST; do
    # 文件完整路径
    file="${EOS_DIR}/${fname}"

    # 提取编号 例：RUN.527.TAODAQ.TEST.ds-0.global_trigger.20251108125748.001_J25.6.0_gitVcd310cd8.rtraw
    file_idx=$(echo "$fname" | grep -oE '\.[0-9]{3}_' | tr -d '._')
    [ -z "$file_idx" ] && file_idx="unknown"

    # 作业脚本路径
    job_name="job_527_${file_idx}.sh"
    job_path="${JOB_DIR}/${job_name}"

    # 写入作业脚本
    cat > "$job_path" <<EOF
#!/bin/bash
# ====== Job Script: $fname ======

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
