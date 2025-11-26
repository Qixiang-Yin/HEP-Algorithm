// 用于检查ROOT文件中未赋值的Bins

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

void Process(const std::string &inputfile) {

    TFile *f = TFile::Open(inputfile.c_str()); // 确保路径正确
    if (!f || f->IsZombie()) {
        cout << "文件无法打开！" << endl; 
        return;
    }

    TH1F *h = (TH1F*)f->Get("DCR");
    int bad_bins = 0;
    // 遍历所有 Bin 寻找 NaN
    for (int i = 1; i <= h->GetNbinsX(); i++) 
    {
        double content = h->GetBinContent(i);
        if (std::isnan(content) || std::isinf(content)) 
        {
            bad_bins++;
            if (bad_bins == 1)
            {
                std::cout << "发现文件有错误: " << inputfile << std::endl;
            }
            cout << "发现NaN在Channel: " << i-1 << ", 内容是: " << content << endl;
        }
    }

    if (bad_bins != 0)
    {
        cout << "总共发现 " << bad_bins << " 个坏Bin" << endl;
    }

}

void Scan_Bad_Bins()
{
    std::string command = "ls " + inputFolder + " > filelist.txt";
    gSystem->Exec(command.c_str());
    std::ifstream filelist("filelist.txt");
    std::string filename;

    while(std::getline(filelist, filename))
    {
        Process(filename);
    }
    
    filelist.close();
    gSystem->Exec("rm filelist.txt"); 
}