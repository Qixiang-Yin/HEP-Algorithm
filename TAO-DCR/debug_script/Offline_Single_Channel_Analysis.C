// Offline_Single_Channel_Analysis.C
// by Qixiang Yin
// 本地用单通道分析 用于查看个别通道内存溢出的问题 检查算法是否存在Bug
// 不需要依赖TAOSW离线软件
// 运行方式: root -l Offline_Single_Channel_Analysis.C

#include <iostream>
#include <string>
#include <vector>
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TSystem.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLatex.h"
const std::vector<std::string> inputFiles = {
    "elec_channel_715_049.root"
};

// 主执行函数
// 主处理函数
void process_file(const std::string& inputFile) {

    int Nentries = 60000; // 总事例数
    
    std::cout << "处理文件: " << inputFile << std::endl;

    // 单通道图
    const int channel_ana_id = 2033; // Channel ID
    std::string outputFile = "output.root";
    
    // 打开输入文件
    TFile *file2 = TFile::Open(inputFile.c_str());
      

    // 单通道图
    TH1F* h_TDC_Channel = (TH1F*)file2->Get(("TDC_Channels_" + std::to_string(channel_ana_id)).c_str());
    h_TDC_Channel->GetXaxis()->SetTitle("TDC [ns]");
    std::cout << "当前正在分析通道: " << channel_ana_id << std::endl;

    ////////////////////////////////////////////////////////////////
    struct TDC_Struct
    {
        int entry; // Hit数量
        double dt; // TDC分布图时间
        bool tag; // 标记是否去除
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
    std::vector<TDC_Struct> raw_tdc_calib_datas;
    std::vector<TDC_Struct> tdc_calib_datas;
    std::vector<Fit_Struct> fit_datas; // 常数函数拟合的数据
    // 用于计算DCR的数据
    float dcr = 0.0;
    float nhits = 0.0;
    float sumtime = 0.0;

    // 读取TDC分布图
    for (int i=1; i<=h_TDC_Channel->GetNbinsX(); i++)
    {
        int tdc_entry = h_TDC_Channel->GetBinContent(i);
        double tdc_dt = h_TDC_Channel->GetBinLowEdge(i);

        if (tdc_entry != 0)
        {
            tdc_entries.push_back(tdc_entry);
            tdc_dts.push_back(tdc_dt);
        }
    }

    //将击中、时间、比值存入结构体
    for(int i=0; i<tdc_entries.size(); i++)
    {
        TDC_Struct tdc_tmp;

        tdc_tmp.entry = tdc_entries[i];
        tdc_tmp.dt = tdc_dts[i];
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

    if (tdc_max.dt > 0) // TDC峰值>0
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
        double endX = h_TDC_Channel->GetXaxis()->GetXmax();

        for (double x = startX; x < endX; x += stepWidth) 
        {
            double currentEnd = x + stepWidth;
            if (currentEnd > endX) currentEnd = endX; // 确保不溢出边界

            // 检查数据量 舍弃数据量低的区间
            int bin1 = h_TDC_Channel->GetXaxis()->FindBin(x);
            int bin2 = h_TDC_Channel->GetXaxis()->FindBin(currentEnd);
            if (h_TDC_Channel->Integral(bin1, bin2) < 5) 
            {
                continue; 
            }
    
            // 使用 "pol0" (常数函数 y = p0)
            TF1 *fConst = new TF1("fConst", "pol0", x, currentEnd);
    
            // 执行拟合
            h_TDC_Channel->Fit(fConst, "R Q N 0");
    
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

    std::string outputTxtFile = "./output.txt";
    std::ofstream tdc_outfile(outputTxtFile);
    tdc_outfile << "dt(ns)\ttdc_entries" << std::endl;
    
    if(!tdc_calib_datas.empty()) // Vector通常不为空 但也有例外
    {

        for (int i=0; i<tdc_calib_datas.size(); i++)
        {    
            tdc_outfile << tdc_calib_datas[i].dt << '\t' << tdc_calib_datas[i].entry 
                << std::endl;
        }
    

        // 输出用于计算DCR的数据
        for(int i=0; i<tdc_calib_datas.size(); i++)
        {
            nhits += tdc_calib_datas[i].entry;
        }
        
        sumtime = std::fabs(tdc_calib_datas[0].dt-tdc_calib_datas.back().dt-2.5) * 1e-9 * static_cast<double>(Nentries);
        if(sumtime == 0.0) // 分母不能为零
        {
            dcr = 0.0;
        }
        else
        {
            dcr = nhits / (sumtime * 50.7 * 50.7 * 0.5);
        }
    }

    else
    {
        dcr = 0.0;
        nhits = 0.0;
        sumtime = 0.0;
    }

    if(!tdc_calib_datas.empty())
    {
        tdc_outfile << "tdc_calib_datas[0].dt: " << tdc_calib_datas[0].dt << std::endl;
        tdc_outfile << "tdc_calib_datas.back().dt: " << tdc_calib_datas.back().dt << std::endl;
    }
    tdc_outfile << "sumtime: " << sumtime << std::endl;
    tdc_outfile << "nhits: " << nhits << std::endl;
    tdc_outfile << "dcr: " << dcr << std::endl;
    tdc_outfile.close();
    

    

    // 保存输出
    TFile* output = new TFile(outputFile.c_str(), "RECREATE");
    if (output && !output->IsZombie()) 
    {
        output->cd();

        // 保存全局直方图
        h_TDC_Channel->Write();
    
        output->Close();
        std::cout << "输出已保存到: " << outputFile << std::endl;
    } 
    else 
    {
        std::cerr << "错误：无法创建输出文件！ " << outputFile << std::endl;
    }
    
    std::cout << "程序执行完成，资源已清理" << std::endl;
}

// 主函数 - 批量处理所有文件
void Offline_Single_Channel_Analysis() {
    std::cout << "开始批量处理文件..." << std::endl;
    std::cout << "文件数量: " << inputFiles.size() << std::endl;
    
    for (const auto& file : inputFiles) {
        process_file(file);
    }
    
    std::cout << "所有文件处理完成！" << std::endl;
}