// DCR_Calib.C
// by Qixiang Yin
// 使用前请确保已设置TAO环境: source /path/to/taosw/setup.sh
// 运行方式: root -l DCR_Calib.C

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
    "./rtraw_data/rtraw_411_001.root"
    /*
    "./rtraw_data/rtraw_411_002.root",
    "./rtraw_data/rtraw_411_003.root",
    "./rtraw_data/rtraw_411_004.root",
    "./rtraw_data/rtraw_411_005.root",
    "./rtraw_data/rtraw_411_006.root",
    "./rtraw_data/rtraw_411_007.root",
    "./rtraw_data/rtraw_411_008.root",
    "./rtraw_data/rtraw_411_009.root",
    "./rtraw_data/rtraw_411_010.root"
    */
};

// 主执行函数
// 主处理函数
void process_file(const std::string& inputFile) {
    std::cout << "处理文件: " << inputFile << std::endl;
    // 批量处理所有文件
    size_t pos = inputFile.find("rtraw_411_");
    if (pos == std::string::npos) {
        std::cerr << "错误：无法从文件路径提取索引" << std::endl;
        return;
    }

    std::string fileIndex = inputFile.substr(pos + 10);
    fileIndex = fileIndex.substr(0, fileIndex.find(".root"));
    std::string outputFile = "./output_elec/elec_411_" + fileIndex + ".root";


    // std::string outputFile = "./output_elec/elec_411_002.root";
    ////////////////////////////////////////////////
    std::string outputTxtFile = "./output_file/tdc_411_001.txt";
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
    
    Tao::CdTrigEvt* TrigEvt = new Tao::CdTrigEvt();
    TrigTree->SetBranchAddress("CdTrigEvt", &TrigEvt);    
    
    // 创建直方图
    TH1F *h_ADCs = new TH1F("ADCs", "ADCs Distribution", 20000, 0, 2e7); // 总ADC
    TH1F *h_Widths = new TH1F("Widths", "Widths Distribution", 2000, 0, 2000);
    TH1F *h_Baselines = new TH1F("Baselines", "Baselines Distribution", 300, 1900, 2200);
    TH1F *h_TDCs = new TH1F("TDCs", "TDCs Distribution", 800, -500, 1500); // 总TDC
    TH1F *h_channelid = new TH1F("channelid", "Channel ID Distribution", 8048, 0, 8048); // 每个事例中着火Channel的编号
    TH1F *h_channels = new TH1F("channels", "Channel Hits per Event", 8048, 0, 8048); // 每个事例中着火Channel的数量

    TH1F *h_OF1s = new TH1F("OF1s", "OF1s Distribution", 100, 0, 100);
    TH1F *h_OF2s = new TH1F("OF2s", "OF2s Distribution", 100, 0, 100);
    h_OF1s->SetStats(true);
    h_OF2s->SetStats(true);
    
    // 设置轴标签
    h_ADCs->GetXaxis()->SetTitle("ADC");
    h_channelid->GetXaxis()->SetTitle("Channel ID");
    h_channels->GetXaxis()->SetTitle("Channel Number");
    h_TDCs->GetXaxis()->SetTitle("TDC [ns]");

     // 处理事件
     int Nentries = ElecTree->GetEntries();
     std::cout << "找到事件数: " << Nentries << std::endl;

    for (int i = 0; i < Nentries; i++) // 循环每一事例
    {
        ElecTree->GetEntry(i);
        TrigTree->GetEntry(i); // 确保获取触发时间
        
        //获取通道数据
        std::vector<Tao::CdElecChannel> ElecfChannels_vec = ElecEvt->GetElecChannels();
        h_channels->Fill(ElecfChannels_vec.size()); // 获取每一事例的Channel数量
    
        for (unsigned int k = 0; k < ElecfChannels_vec.size(); k++) // 循环事例i中每一Channel
        {
            Tao::CdElecChannel* ElecfChannel = &ElecfChannels_vec[k];
            int channel_id = static_cast<int>(ElecfChannel->getChannelID()); // Channel编号
            std::vector<float> Elec_ADCs = ElecfChannel->getADCs(); // 事例i中通道k的ADC
            auto TDC16 = ElecfChannel->getTDCs(); // 事例i中通道k的TDC
            std::vector<int64_t> Elec_TDCs(TDC16.begin(), TDC16.end());
            std::vector<uint16_t> Elec_Widths = ElecfChannel->getWidths();
            std::vector<uint16_t> Elec_Baselines = ElecfChannel->getBaselines();
            std::vector<uint8_t> Elec_OF1s = ElecfChannel->getOF1s();
            std::vector<uint8_t> Elec_OF2s = ElecfChannel->getOF2s();

            h_channelid->Fill(channel_id);
            
            for (size_t j = 0; j < Elec_ADCs.size(); j++) // 循环每次着火 每次着火是一个Hit
            {
                float charge = Elec_ADCs[j];
                float time = static_cast<float>(Elec_TDCs[j]);
                float width = static_cast<float>(Elec_Widths[j]);
                float baseline = static_cast<float>(Elec_Baselines[j]);
                float of1 = static_cast<float>(Elec_OF1s[j]);
                float of2 = static_cast<float>(Elec_OF2s[j]);

                h_ADCs->Fill(charge);
                h_TDCs->Fill(time);
                h_Widths->Fill(width);
                h_Baselines->Fill(baseline);
                h_OF1s->Fill(of1);
                h_OF2s->Fill(of2);

                // std::cout << "Entry:" << i << ' ' << "ChannelID:" << channel_id << ' ' << "TDC:" << time << std::endl;
                // Elec_ADCs.size()是第i个事例中，第k个通道的着火（Hit）次数
            }
        }

    }

    // DCR Calibration
    ////////////////////////////////////////////////
    struct TDC_Struct
    {
        int entry; // Hit数量
        double dt; // TDC分布图时间
        double ratio; // TDC分布图相邻Bin的比值
        bool tag; // 标记是否去除
    };
    std::vector<int> tdc_entries;
    std::vector<double> tdc_dts;
    std::vector<double> tdc_ratios;
    std::vector<TDC_Struct> raw_tdc_calib_datas;
    std::vector<TDC_Struct> tdc_calib_datas;
    double nhits = 0;
    double sumtime = 0;
    double dcr_channel;
    double dcr;
    TH1F* h_TDC_Channels = new TH1F("TDC_Channels_4793","TDC for Ch4793", 800, -500, 1500);
    h_TDC_Channels->GetXaxis()->SetTitle("TDC [ns]");

    // Ch4793的TDC分布
    for(int i=0; i<Nentries; i++)
    {
        ElecTree->GetEntry(i);

        std::vector<Tao::CdElecChannel> Channels_vec = ElecEvt->GetElecChannels();

        for (unsigned int j=0; j<Channels_vec.size(); j++)
        {
            Tao::CdElecChannel* Channel = &Channels_vec[j];
            int channel_id_calib = static_cast<int>(Channel->getChannelID()); // Channel编号
            if(channel_id_calib == 4793)
            {
                std::vector<float> ADCs_Calib = Channel->getADCs();
                auto TDC_Calib = Channel->getTDCs(); // 事例i中通道j的TDC
                std::vector<int64_t> TDCs_Calib(TDC_Calib.begin(), TDC_Calib.end());
                //std::cout<<ADCs_Calib.size()<<' '<<TDCs_Calib.size()<<std::endl;
            
                for(size_t k=0; k<ADCs_Calib.size(); k++)
                {
                    float time = static_cast<float>(TDCs_Calib[k]);
                    h_TDC_Channels->Fill(time);
                }
            }
        }
    }


    // 读取Ch4793 TDC分布图
    for (int i=1; i<=h_TDC_Channels->GetNbinsX(); i++)
    {
        int tdc_entry = h_TDC_Channels->GetBinContent(i);
        double tdc_dt = h_TDC_Channels->GetBinLowEdge(i);

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
        TDC_Struct data;

        data.entry = tdc_entries[i];
        data.dt = tdc_dts[i];
        data.ratio = tdc_ratios[i];
        data.tag = true;

        raw_tdc_calib_datas.push_back(data);
    }

    for(int i=0; i<raw_tdc_calib_datas.size(); i++)
    {
        // 去除TDC分布图中靠前突变的部分
        /*
        double diff = 1.0 - raw_tdc_calib_datas[i].ratio;
        if(i > 0 && std::fabs(diff) > 0.3 
            && raw_tdc_calib_datas[i].dt <= 0.0)
        {
            for(int j=0; j<=i; j++)
            {
                raw_tdc_calib_datas[j].tag = false;
            }
            
            break;
        }
        */

        // 去除TDC时间大于等于零的部分
        if(raw_tdc_calib_datas[i].dt >= 0.0)
        {
            raw_tdc_calib_datas[i].tag = false;
        }
    }

    // 去除TDC分布图中Entries与前一个Bin相差过大的Bin+该Bin之后的所有Bin
    /*
    for(int i=0; i<raw_tdc_calib_datas.size(); i++)
    {
        double diff_with_last = 1.0 - raw_tdc_calib_datas[i].ratio;
        if(i > 0 && std::fabs(diff_with_last) > 0.3 
            && raw_tdc_calib_datas[i].tag != false)
        {
            for(int j=i; j<raw_tdc_calib_datas.size(); j++)
            {
                raw_tdc_calib_datas[j].tag = false;
            }
            
            break;
        }
    }
    */

    // 去除TDC分布图中Entries与第一个Bin相差过大的Bin+该Bin之后的所有Bin
    /*
    double entries_1st;
    for(int j=0; j<raw_tdc_calib_datas.size(); j++)
    {
        if (raw_tdc_calib_datas[j].tag != false)
        {
            entries_1st = raw_tdc_calib_datas[j].entry; // 获取第一个恰当的TDCBin的Entries

            break;
        }
    }
    
    for (int i=0; i<raw_tdc_calib_datas.size(); i++)
    {
        double diff_with_1st = 1.0 - static_cast<double>(raw_tdc_calib_datas[i].entry) / entries_1st; // 1-与第一个Bin的比值
        if(i > 0 && std::fabs(diff_with_1st) > 0.4 && raw_tdc_calib_datas[i].dt <= 0.0 
            && raw_tdc_calib_datas[i].tag == true)
        {
            for(int j=i; j<raw_tdc_calib_datas.size(); j++)
            {
                raw_tdc_calib_datas[j].tag = false;
            }

            break;
        }
    }
    */

    // 保存为新结构体
    for(int i=0; i<raw_tdc_calib_datas.size(); i++)
    {
        if(raw_tdc_calib_datas[i].tag != false)
        {
            tdc_calib_datas.push_back(raw_tdc_calib_datas[i]);
        }
    }

    // 填充Ratio图
    std::vector<double> bin_edges;
    for (int i=0; i<tdc_calib_datas.size(); i++) 
    {
        bin_edges.push_back(tdc_calib_datas[i].dt);
    }

    bin_edges.push_back(0.0);

    TH1F *h_Ratio_AdjBin = new TH1F("Ratio_AdjacentBin", "TDC Ratio; dt[ns]", 
        bin_edges.size()-1, bin_edges.data());

    for (int i=0; i<tdc_calib_datas.size(); i++) 
    {
        h_Ratio_AdjBin->SetBinContent(i+1, tdc_calib_datas[i].ratio);
    }

    // 计算DCR  
    for(int i=0; i<tdc_calib_datas.size(); i++)
    {
        nhits += tdc_calib_datas[i].entry;
    }

    sumtime = std::fabs(tdc_calib_datas[0].dt-tdc_calib_datas.back().dt-2.5)
        * 1e-9 * static_cast<double>(Nentries);

    dcr_channel = static_cast<double>(nhits) / sumtime;
    dcr = dcr_channel / (50.7*50.7*0.5);
    
    std::cout << "DCR for Ch4793:" << dcr << std::endl;

    



    // 输出
    
    std::ofstream tdc_outfile(outputTxtFile);
    tdc_outfile << "dt(ns)\ttdc_entries\tratio" << std::endl;

    for (int i=0; i<tdc_calib_datas.size(); i++)
    {    
        tdc_outfile << tdc_calib_datas[i].dt << '\t' << tdc_calib_datas[i].entry 
            << '\t' << std::fabs(1-tdc_calib_datas[i].ratio) << std::endl;
    }

    tdc_outfile.close();
    ////////////////////////////////////////////////

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
        h_Widths->Write();
        h_Baselines->Write();
        h_OF1s->Write();
        h_OF2s->Write();
        //
        h_Ratio_AdjBin->Write();
        h_TDC_Channels->Write();

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