// DCR_Calib.C
// by Qixiang Yin
// 使用前请确保已设置TAO环境: source /path/to/taosw/setup.sh
// 使用方法 root -l -q 'DCR_Calib.C("rtraw_data/rtraw_514_001.root")' 单文件
// 使用方法 root -l -q 'DCR_Calib.C()' 批处理文件（旧方式）

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cmath>
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TSystem.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLatex.h"
const std::vector<std::string> inputFiles = {
    /*
    "./rtraw_data/rtraw_514_001.root",
    "./rtraw_data/rtraw_514_002.root",
    "./rtraw_data/rtraw_514_003.root",
    "./rtraw_data/rtraw_514_004.root",
    "./rtraw_data/rtraw_514_005.root",
    "./rtraw_data/rtraw_514_006.root",
    "./rtraw_data/rtraw_514_007.root",
    "./rtraw_data/rtraw_514_008.root",
    "./rtraw_data/rtraw_514_009.root",
    "./rtraw_data/rtraw_514_010.root",
    */
    // "root://junoeos01.ihep.ac.cn//eos/juno/tao-rtraw/J25.6.0/gitVcd310cd8/global_trigger/00000000/00000500/516/RUN.516.TAODAQ.TEST.ds-0.global_trigger.20251108125748.001_J25.6.0_gitVcd310cd8.rtraw"
};

// 主执行函数
// 主处理函数
void process_file(const std::string& inputFile) {
    std::cout << "处理文件: " << inputFile << std::endl;
    // 批量处理所有文件
    size_t pos = inputFile.find("_J25.6.0_gitVbd15bb5e.rtraw");
    if (pos == std::string::npos) {
        std::cerr << "错误：无法从文件路径提取索引" << std::endl;
        return;
    }

    std::string fileIndex = inputFile.substr(pos - 3);
    fileIndex = fileIndex.substr(0, fileIndex.find("_J25"));
    std::string outputFile = "./output_dcr/dcr_649_" + fileIndex + ".root";

    std::cout << "输出文件: " << outputFile << std::endl;


    // std::string outputFile = "./output_elec/elec_411_001.root";
    ////////////////////////////////////////////////
    // std::string outputTxtFile = "./output_file/tdc_411_001.txt";
    ////////////////////////////////////////////////
    
    // 打开输入文件
    TFile *file2 = TFile::Open(inputFile.c_str());
    if (!file2 || file2->IsZombie()) {
        std::cerr << "错误：无法打开输入文件！ " << inputFile << std::endl;
        return;
    }
    
    // 获取电子学事件树
    TTree* ElecTree = (TTree*)file2->Get("Event/Elec/CdElecEvt");
    if (!ElecTree) {
        std::cerr << "错误：找不到树 Event/Elec/CdElecEvt" << std::endl;
        file2->Close();
        return;
    }
    
    // 获取触发事件树
    TTree* TrigTree = (TTree*)file2->Get("Event/Trig/CdTrigEvt");
    if (!TrigTree) {
        std::cerr << "错误：找不到树 Event/Trig/CdTrigEvt" << std::endl;
        file2->Close();
        return;
    }

    
    // 创建事件对象
    Tao::CdElecEvt* ElecEvt = new Tao::CdElecEvt();    
    ElecTree->SetBranchAddress("CdElecEvt", &ElecEvt);
    
    // Tao::CdTrigEvt* TrigEvt = new Tao::CdTrigEvt();
    // TrigTree->SetBranchAddress("CdTrigEvt", &TrigEvt);    

     // 处理事件
     int Nentries = ElecTree->GetEntries();
     std::cout << "找到事件数: " << Nentries << std::endl;

    // DCR Calibration
    ////////////////////////////////////////////////
    struct TDC_Struct
    {
        int entry; // Hit数量
        double dt; // TDC分布图时间
        double ratio; // TDC分布图相邻Bin的比值
        bool tag; // 标记是否去除
    };
    struct DCR_Struct
    {
        int ch_id; // 通道号
        double dcr_ch; // DCR
    };
    struct Fit_Struct
    {
        double fit_value; // 拟合的直线高度
        double chi2_ndf; // Chi^2/NDF
        double leftedge; // 拟合区间的左边界
        bool fit_tag; // 标记拟合好坏
    };
    std::vector<int> tdc_entries;
    std::vector<double> tdc_dts;
    std::vector<double> tdc_ratios;
    std::vector<TDC_Struct> raw_tdc_calib_datas; // 原始TDC数据
    std::vector<TDC_Struct> tdc_calib_datas; // 用于刻度的TDC数据
    std::vector<DCR_Struct> dcrs; // 通道-DCR 组合
    std::vector<Fit_Struct> fit_datas; // 常数函数拟合的数据
    double nhits = 0; // 着火数
    double sumtime = 0; // 时间窗
    double dcr_channel; // DCR (单通道的DCR Hz)
    double dcr; // DCR (单通道单位面积的DCR Hz/mm^2)

    TH1F* h_DCR = new TH1F("DCR", "DCR for Channels", 100000, 0, 100000);
    h_DCR->GetXaxis()->SetTitle("Channel ID");
    h_DCR->GetYaxis()->SetTitle("DCR (Hz/mm2)");

    const int number_channel = 100000;
    TH1F* h_TDC_Channels[number_channel];
    for (int i = 0; i < number_channel; i++) 
    {
        TString name = Form("TDC_Channels_%d", i);
        TString title = Form("TDC for Channel %d", i);
        h_TDC_Channels[i] = new TH1F(name, title, 800, -500, 1500);
        h_TDC_Channels[i]->GetXaxis()->SetTitle("TDC [ns]");
    }

    // Ch的TDC分布
    for(int i=0; i<Nentries; i++)
    {
        ElecTree->GetEntry(i);

        std::vector<Tao::CdElecChannel> Channels_vec = ElecEvt->GetElecChannels();

        for (unsigned int j=0; j<Channels_vec.size(); j++)
        {
            Tao::CdElecChannel* Channel = &Channels_vec[j];
            int channel_id_calib = static_cast<int>(Channel->getChannelID()); // Channel编号
            std::vector<float> ADCs_Calib = Channel->getADCs();
            auto TDC_Calib = Channel->getTDCs(); // 事例i中通道j的TDC
            std::vector<int64_t> TDCs_Calib(TDC_Calib.begin(), TDC_Calib.end());
            //std::cout<<ADCs_Calib.size()<<' '<<TDCs_Calib.size()<<std::endl;
            
            for(size_t k=0; k<ADCs_Calib.size(); k++)
            {
                float time = static_cast<float>(TDCs_Calib[k]);
                h_TDC_Channels[channel_id_calib]->Fill(time);
            }
        }
    }


    for(int l=0; l<number_channel; l++)
    {
        // 初始化
        tdc_entries.clear();
        tdc_dts.clear();
        tdc_ratios.clear();
        raw_tdc_calib_datas.clear();
        tdc_calib_datas.clear();
        nhits = 0;
        sumtime = 0;
        
        // 读取TDC分布图
        for (int i=1; i<=h_TDC_Channels[l]->GetNbinsX(); i++)
        {
            int tdc_entry = h_TDC_Channels[l]->GetBinContent(i);
            double tdc_dt = h_TDC_Channels[l]->GetBinLowEdge(i);

            if (tdc_entry != 0)
            {
                tdc_entries.push_back(tdc_entry);
                tdc_dts.push_back(tdc_dt);
            }
        }

        if(tdc_entries.size() == 0) // 如果某个Channel没有计数 则该Channel的DCR置零 直接开始读取下一个Channel
        {
            DCR_Struct dcr_tmp;
            dcr_tmp.ch_id = l;
            dcr_tmp.dcr_ch = 0;
            dcrs.push_back(dcr_tmp);
            continue;
        }
        
        // 计算相邻bin的比值
        for (int i=0; i<tdc_entries.size(); i++)
        {
        if (i == 0)
            {
                tdc_ratios.push_back(0);
            }
            else
            {
                tdc_ratios.push_back(static_cast<double>(tdc_entries[i])/tdc_entries[i-1]); // ratio=bin/(bin-1)
            }
        }

        //将击中、时间、比值存入结构体
        for(int i=0; i<tdc_entries.size(); i++)
        {
            TDC_Struct tdc_tmp;

            tdc_tmp.entry = tdc_entries[i];
            tdc_tmp.dt = tdc_dts[i];
            tdc_tmp.ratio = tdc_ratios[i];
            tdc_tmp.tag = true;

            raw_tdc_calib_datas.push_back(tdc_tmp);
        }

        // 寻找Enetries的最大值
        TDC_Struct tdc_max;
        for (int i=0; i<raw_tdc_calib_datas.size(); i++)
        {
            if (i == 0)
            {
                tdc_max = raw_tdc_calib_datas[0];
            }

            else
            {
                if(raw_tdc_calib_datas[i].entry >= tdc_max.entry)
                {
                    tdc_max = raw_tdc_calib_datas[i];
                }
            }
        }

        if (tdc_max.dt > 0) // TDC峰值<=0
        {
            for(int i=0; i<raw_tdc_calib_datas.size(); i++)
            {
                // 去除TDC时间大于等于零的部分
                if(raw_tdc_calib_datas[i].dt >= 0.0)
                {
                    raw_tdc_calib_datas[i].tag = false;
                }
            }
        }

        else // TDC峰值<=0
        {
            // 常数函数拟合TDC图中>100ns的部分 寻找平坦区间
            double startX = 0.0;
            double stepWidth = 100.0; // 区间宽度 100ns
            double endX = h_TDC_Channels[l]->GetXaxis()->GetXmax();

            for (double x = startX; x < endX; x += stepWidth) 
            {
                double currentEnd = x + stepWidth;
                if (currentEnd > endX) currentEnd = endX; // 确保不溢出边界

                // 检查数据量 舍弃数据量低的区间
                int bin1 = h_TDC_Channels[l]->GetXaxis()->FindBin(x);
                int bin2 = h_TDC_Channels[l]->GetXaxis()->FindBin(currentEnd);
                if (h_TDC_Channels[l]->Integral(bin1, bin2) < 5) 
                {
                    continue; 
                }
        
                // 使用 "pol0" (常数函数 y = p0)
                TF1 *fConst = new TF1("fConst", "pol0", x, currentEnd);
        
                // 执行拟合
                h_TDC_Channels[l]->Fit(fConst, "R Q N 0");
        
                // 获取结果
                double height = fConst->GetParameter(0);    // 平均高度
                double error  = fConst->GetParError(0);     // 误差
                double chi2   = fConst->GetChisquare();
                double ndf    = fConst->GetNDF();
                
                double reducedChi2 = (ndf > 0) ? chi2 / ndf : -1.0; // 计算Chi^2/NDF 确保NDF>0

                Fit_Struct fit_tmp;
                fit_tmp.fit_value = height;
                fit_tmp.chi2_ndf = reducedChi2;
                fit_tmp.leftedge = x;
                if(fit_tmp.chi2_ndf >= 2.0 || fit_tmp.chi2_ndf <= 0.5) // Chi2/NDF作为判断指标 拟合结果差表明Hits在该区间内变化较大
                {
                    fit_tmp.fit_tag = false;
                }
                else
                {
                    fit_tmp.fit_tag = true;
                }
                fit_datas.push_back(fit_tmp);
        
                delete fConst;
            }

            double goodfit_starttime = 10000.0; // 选取常数函数拟合效果好的区间
            double goodfit_enttime = 10000.0;

            for(int i=0; i<fit_datas.size(); i++)
            {
                if(fit_datas[i].fit_tag == true)
                {
                    if(goodfit_starttime == 10000.0)
                    {
                        goodfit_starttime = fit_datas[i].leftedge;
                    }
                }
                
                if(fit_datas[i].fit_tag == false)
                {
                    if(goodfit_starttime != 10000.0)
                    {
                        goodfit_enttime = fit_datas[i].leftedge;
                        break;
                    }
                }
            }

            for(int i=0; i<raw_tdc_calib_datas.size(); i++)
            {
                if(raw_tdc_calib_datas[i].dt < goodfit_starttime || raw_tdc_calib_datas[i].dt > goodfit_enttime)
                {
                    raw_tdc_calib_datas[i].tag = false;
                }
            }

        }

        // 保存为新结构体
        for(int i=0; i<raw_tdc_calib_datas.size(); i++)
        {
            if(raw_tdc_calib_datas[i].tag != false)
            {
                tdc_calib_datas.push_back(raw_tdc_calib_datas[i]);
            }
        }

        // 计算DCR  
        for(int i=0; i<tdc_calib_datas.size(); i++)
        {
            nhits += tdc_calib_datas[i].entry;
        }

        sumtime = std::fabs(tdc_calib_datas[0].dt-tdc_calib_datas.back().dt-2.5) // Bin Width = 2.5
            * 1e-9 * static_cast<double>(Nentries);

        dcr_channel = static_cast<double>(nhits) / sumtime;
        dcr = dcr_channel / (50.7*50.7*0.5);

        DCR_Struct dcr_tmp;
        dcr_tmp.ch_id = l;
        dcr_tmp.dcr_ch = dcr;
        dcrs.push_back(dcr_tmp);

        // 清空容器 开始读取下一个Channel
        tdc_entries.clear();
        tdc_dts.clear();
        tdc_ratios.clear();
        raw_tdc_calib_datas.clear();
        tdc_calib_datas.clear();
        nhits = 0;
        sumtime = 0;
    }

    // 填图
    for (const auto& tmp : dcrs) 
    {
        int bin = h_DCR->FindBin(tmp.ch_id);
        h_DCR->SetBinContent(bin, tmp.dcr_ch);
    }

    // 输出
    /*
    std::ofstream tdc_outfile(outputTxtFile);
    tdc_outfile << "dt(ns)\ttdc_entries\tratio" << std::endl;

    for (int i=0; i<tdc_calib_datas.size(); i++)
    {    
        tdc_outfile << tdc_calib_datas[i].dt << '\t' << tdc_calib_datas[i].entry 
            << '\t' << std::fabs(1-tdc_calib_datas[i].ratio) << std::endl;
    }

    tdc_outfile.close();
    */
    ////////////////////////////////////////////////

    // 保存输出
    TFile* output = new TFile(outputFile.c_str(), "RECREATE");
    if (output && !output->IsZombie()) 
    {
        output->cd();

        // 保存全局直方图
        // h_ADCs->Write();
        // h_TDCs->Write();
        // h_channelid->Write();
        // h_channels->Write();
        // h_Widths->Write();
        // h_Baselines->Write();
        // h_OF1s->Write();
        // h_OF2s->Write();
        // h_TDC_Channels->Write();
        h_DCR->Write();

        output->Close();
        std::cout << "输出已保存到: " << outputFile << std::endl;
    }
    
    std::cout << "程序执行完成，资源已清理" << std::endl;
}

// 主函数 - 批量处理所有文件
void DCR_Calib() {
    std::cout << "开始批量处理文件..." << std::endl;
    std::cout << "文件数量: " << inputFiles.size() << std::endl;
    
    for (const auto& file : inputFiles) {
        process_file(file);
    }
    
    std::cout << "所有文件处理完成！" << std::endl;
}

// 主函数 - 从外部传递文件名作为参数
// 使用方法 root -l -q 'DCR_Calib.C("rtraw_data/rtraw_514_001.root")' 单文件
// 使用方法 root -l -q 'DCR_Calib.C()' 批处理文件（旧方式）
void DCR_Calib(const char* inputFile) {
    if (inputFile == nullptr || strlen(inputFile) == 0) {
        return;
    }

    std::cout << "开始处理传入文件..." << std::endl;

    process_file(inputFile);
}