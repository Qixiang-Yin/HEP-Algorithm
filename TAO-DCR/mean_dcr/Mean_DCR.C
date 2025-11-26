// Mean_DCR.C
// 对dcr_xxx_xxx.root求平均 产生这一个RUN的平均DCR
// Usage: root Mean_DCR.C

#include <iostream>
#include <fstream>
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
#include "TSystem.h"

// 文件名
const std::string inputFolder = "../output_dcr/dcr_715_*.root";
const std::string outputFile = "./mean_dcr_715.root";
// RTRAW文件数
const double file_number = 69;

std::vector<double> Process(const std::string &inputFiles)
{
    std::vector<double> dcr_content;

    TFile *input = TFile::Open(inputFiles.c_str());
    TH1F *input_dcr = (TH1F*)input->Get("DCR");
    for(int i=1; i<=input_dcr->GetNbinsX(); i++)
    {
        double bin_content = input_dcr->GetBinContent(i);
        dcr_content.push_back(bin_content);
    }

    input->Close();

    return dcr_content;
}

void Mean_DCR()
{
    std::vector<double> sum_dcr_content;
    
    std::string command = "ls " + inputFolder + " > filelist.txt";
    gSystem->Exec(command.c_str());
    std::ifstream filelist("filelist.txt");
    std::string filename;
    while (std::getline(filelist, filename)) 
    {
        std::cout << "Processing File: " << filename << std::endl;
        std::vector<double> temp = Process(filename);
        if(sum_dcr_content.empty())
        {
            sum_dcr_content = temp;
        }
        
        else
        {
            for(int i=0; i<temp.size(); i++)
            {
                sum_dcr_content[i] += temp[i];
            }
        }
    }

    TH1F *h_Mean_DCR = new TH1F("Mean_DCR", "DCR for Channels", 100000, 0, 100000);
    h_Mean_DCR->GetXaxis()->SetTitle("Channel ID");
    h_Mean_DCR->GetYaxis()->SetTitle("DCR (Hz/mm2)");

    for(int i=1; i<=h_Mean_DCR->GetNbinsX(); i++)
    {
        h_Mean_DCR->SetBinContent(i,0);
    }

    for(int i=0; i<sum_dcr_content.size(); i++)
    {
        double mean_dcr_content = sum_dcr_content[i] / file_number;
        h_Mean_DCR->SetBinContent(i+1, mean_dcr_content);
    }

    TFile *output = new TFile(outputFile.c_str(), "RECREATE");
    output->cd();
    h_Mean_DCR->Write();
    output->Close();

    filelist.close();
    gSystem->Exec("rm filelist.txt"); 
}