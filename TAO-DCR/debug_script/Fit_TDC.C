// 用于检查拟合的信息

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

void Fit_TDC()
{
    TFile *input = TFile::Open("../output_elec/elec_channel_715_007.root");
    TH1F *h_TDC = (TH1F*)input->Get("TDC_Channels_7569");
    
    // 常数函数拟合TDC图中>100ns的部分 寻找平坦区间
    double startX = 0.0;
    double stepWidth = 100.0; // 区间宽度 100ns
    double endX = h_TDC->GetXaxis()->GetXmax();

    for (double x = startX; x < endX; x += stepWidth) 
    {
        double currentEnd = x + stepWidth;
        if (currentEnd > endX) currentEnd = endX; // 确保不溢出边界

        // 检查数据量 舍弃数据量低的区间
        int bin1 = h_TDC->GetXaxis()->FindBin(x);
        int bin2 = h_TDC->GetXaxis()->FindBin(currentEnd);
        if (h_TDC->Integral(bin1, bin2) < 5) 
        {
            continue; 
        }
        
        // 使用 "pol0" (常数函数 y = p0)
        TF1 *fConst = new TF1("fConst", "pol0", x, currentEnd);
        
        // 执行拟合
        h_TDC->Fit(fConst, "R Q N 0");
        
        // 获取结果
        double height = fConst->GetParameter(0);    // 平均高度
        double error  = fConst->GetParError(0);     // 误差
        double chi2   = fConst->GetChisquare();
        double ndf    = fConst->GetNDF();
                
        double reducedChi2 = (ndf > 0) ? chi2 / ndf : -1.0; // 计算Chi^2/NDF 确保NDF>0

        printf("%-12.1f %-12.1f | %-15.2f %-15.2f | %-10.2f | ", 
               x, currentEnd, height, error, reducedChi2);

        // 判定逻辑
        if (reducedChi2 > 0.5 && reducedChi2 < 2.0) {
            printf("\033[1;32m[GoodFit]\033[0m"); // 绿色高亮: 平坦
        } else if (reducedChi2 >= 2.0) {
            printf("\033[1;31m[BadFit]\033[0m"); // 红色高亮: 有结构/不平
        } else {
            printf("[LowStat]");
        }
        printf("\n");
        
        delete fConst;
    }
}