// Single_Channel_Analysis.C
// by Qixiang Yin
// 单通道分析
// 使用前请确保已设置TAO环境: source /path/to/taosw/setup.sh
// 运行方式: root -l Single_Channel_Analysis.C

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
    "root://junoeos01.ihep.ac.cn//eos/juno/tao-rtraw/J25.6.0/gitVbd15bb5e/CD/00000000/00000600/649/RUN.649.TAODAQ.TEST.ds-0.CD.20251120101220.001_J25.6.0_gitVbd15bb5e.rtraw"
    
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

    // 单通道图
    const int channel_ana_id = 4465;
    std::string id_string = std::to_string(channel_ana_id);
    std::string fileIndex = inputFile.substr(pos - 3);
    fileIndex = fileIndex.substr(0, fileIndex.find("_J25"));
    std::string outputFile = "./output_elec/elec_channel_"+ id_string
    +"_649_" + fileIndex + ".root";
    
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
    
    // 创建直方图
    TH1F *h_ADCs = new TH1F("ADCs", "ADCs Distribution", 20000, 0, 2e7); // 总ADC
    TH1F *h_TDCs = new TH1F("TDCs", "TDCs Distribution", 800, -500, 1500); // 总TDC
    TH1F *h_channelid = new TH1F("channelid", "Channel ID Distribution", 100000, 0, 100000); // 每个事例中着火Channel的编号
    TH1F *h_channels = new TH1F("channels", "Channel Hits per Event", 100000, 0, 100000); // 每个事例中着火Channel的着火次数
    
    // 设置轴标签
    h_ADCs->GetXaxis()->SetTitle("ADC");
    h_channelid->GetXaxis()->SetTitle("Channel ID");
    h_channels->GetXaxis()->SetTitle("Channel Number");
    h_TDCs->GetXaxis()->SetTitle("TDC [ns]");

    // 单通道图
    TString name = Form("TDC_Channel_%d", channel_ana_id);
    TString title = Form("TDC for Channel %d", channel_ana_id);
    TH1F* h_TDC_Channel = new TH1F(name, title, 800, -500, 1500);
    h_TDC_Channel->GetXaxis()->SetTitle("TDC [ns]");
    std::cout << "当前正在分析通道: " << channel_ana_id << std::endl;

    // 处理事件
    int Nentries = ElecTree->GetEntries();
    std::cout << "找到事件数: " << Nentries << std::endl;

    for (int i = 0; i < Nentries; i++) // 循环每一事例
    {
        ElecTree->GetEntry(i);
        // TrigTree->GetEntry(i); // 确保获取触发时间
        
        //获取通道数据
        std::vector<Tao::CdElecChannel> ElecfChannels_vec = ElecEvt->GetElecChannels();
        h_channels->Fill(ElecfChannels_vec.size()); // 获取每一事例的Channel数量
    
        for (unsigned int k = 0; k < ElecfChannels_vec.size(); k++) // 循环事例i中每一Channel
        {
            Tao::CdElecChannel* ElecfChannel = &ElecfChannels_vec[k];
            int channel_id = static_cast<int>(ElecfChannel->getChannelID()); // Channel编号
            std::vector<float> Elec_ADCs = ElecfChannel->getADCs(); // 事例i中通道k的ADC

            // Modified by Qixiang Yin
            auto TDC16 = ElecfChannel->getTDCs(); // 事例i中通道k的TDC
            std::vector<int64_t> Elec_TDCs(TDC16.begin(), TDC16.end());

            h_channelid->Fill(channel_id);
            
            for (size_t j = 0; j < Elec_ADCs.size(); j++) // 循环每次着火 每次着火是一个Hit
            {
                float charge = Elec_ADCs[j];
                float time = static_cast<float>(Elec_TDCs[j]);

                h_ADCs->Fill(charge);
                h_TDCs->Fill(time);

                // std::cout << "Entry:" << i << ' ' << "ChannelID:" << channel_id << ' ' << "TDC:" << time << std::endl;
                // Elec_ADCs.size()是第i个事例中，第k个通道的着火（Hit）次数
            }

            // 单通道图
            if (channel_id == channel_ana_id)
            {
                for (size_t j = 0; j < Elec_ADCs.size(); j++) // 循环每次着火 每次着火是一个Hit
                {
                    float time = static_cast<float>(Elec_TDCs[j]);
                    h_TDC_Channel->Fill(time);
                }
            }
        }

    }

    ////////////////////////////////////////////////////////////////
    struct TDC_Struct
    {
        int entry; // Hit数量
        double dt; // TDC分布图时间
        double ratio; // TDC分布图相邻Bin的比值
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
    std::vector<double> tdc_ratios;
    std::vector<TDC_Struct> raw_tdc_calib_datas;
    std::vector<TDC_Struct> tdc_calib_datas;
    std::vector<Fit_Struct> fit_datas; // 常数函数拟合的数据

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

    std::string outputTxtFile = "./output_file/tdc_649_"+ fileIndex + "_" + id_string +".txt";
    std::ofstream tdc_outfile(outputTxtFile);
    tdc_outfile << "dt(ns)\ttdc_entries\tratio" << std::endl;

    for (int i=0; i<tdc_calib_datas.size(); i++)
    {    
        tdc_outfile << tdc_calib_datas[i].dt << '\t' << tdc_calib_datas[i].entry 
            << '\t' << std::fabs(1-tdc_calib_datas[i].ratio) << std::endl;
    }
 

    float dcr = 0.0;
    float nhits = 0.0;
    float sumtime = 0.0;
    for(int i=0; i<tdc_calib_datas.size(); i++)
    {
        nhits += tdc_calib_datas[i].entry;
    }
    
    sumtime = std::fabs(tdc_calib_datas[0].dt-tdc_calib_datas.back().dt-2.5) * 1e-9 * static_cast<double>(Nentries);
    dcr = nhits / (sumtime * 50.7 * 50.7 * 0.5);

    tdc_outfile << "tdc_calib_datas[0].dt: " << tdc_calib_datas[0].dt << std::endl;
    tdc_outfile << "tdc_calib_datas.back().dt: " << tdc_calib_datas.back().dt << std::endl;
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
        h_ADCs->Write();
        h_TDCs->Write();
        h_channelid->Write();
        h_channels->Write();
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
void Single_Channel_Analysis() {
    std::cout << "开始批量处理文件..." << std::endl;
    std::cout << "文件数量: " << inputFiles.size() << std::endl;
    
    for (const auto& file : inputFiles) {
        process_file(file);
    }
    
    std::cout << "所有文件处理完成！" << std::endl;
}