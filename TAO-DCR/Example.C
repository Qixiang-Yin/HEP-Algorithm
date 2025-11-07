// Example.C
// by Jiayang Xu
// 使用前请确保已设置TAO环境: source /path/to/taosw/setup.sh
// 运行方式: root -l Example.C

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
    "./output/rtraw_data.root"
};

// 主执行函数
// 主处理函数
void process_file(const std::string& inputFile) {
    std::cout << "处理文件: " << inputFile << std::endl;
    std::string outputFile = "./output/elec_data.root";
    
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
    TH1F *h_channels = new TH1F("channels", "Channel Hits per Event", 8048, 0, 8048); // 每个事例中着火Channel的着火次数

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

            // Modified by Qixiang Yin
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
void Example() {
    std::cout << "开始批量处理文件..." << std::endl;
    std::cout << "文件数量: " << inputFiles.size() << std::endl;
    
    for (const auto& file : inputFiles) {
        process_file(file);
    }
    
    std::cout << "所有文件处理完成！" << std::endl;
}