// Summary of the moderator thickness sweep.
// Run from the build folder:   root -l './sweepAnalysis.C'
// Uses the "ModExit" diagnostic: particles leaving the BACK face of the moderator (10 x 10 cm).
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TGraphErrors.h"
#include "TMultiGraph.h"
#include "TLegend.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TStyle.h"
#include <vector>
#include <cmath>
#include <cstdio>

void sweepAnalysis()
{
    const std::vector<int> thick = {2, 4, 6, 8, 10, 12};   // cm, must match sweep.sh
    const double faceArea = 100.;                           // cm2, moderator back face
    const double cosAcc   = std::cos(std::atan(10./30.));   // pinhole D = 1 cm, L = 3 cm -> ~18 deg

    const char* cutThCd  = "pdg==2112 && E_eV < 0.5";
    const char* cutTh025 = "pdg==2112 && E_eV < 0.025";
    TString     cutAcc   = Form("pdg==2112 && E_eV < 0.5 && cosTheta > %g", cosAcc);
    const char* cutFast  = "pdg==2112 && E_eV >= 5e4";
    const char* cutGam   = "pdg==22";

    auto gThCd  = new TGraphErrors();  gThCd ->SetTitle("thermal < 0.5 eV");
    auto gTh025 = new TGraphErrors();  gTh025->SetTitle("thermal < 0.025 eV");
    auto gAcc   = new TGraphErrors();  gAcc  ->SetTitle("thermal < 0.5 eV, within 18 deg");
    auto gFast  = new TGraphErrors();  gFast ->SetTitle("fast > 50 keV");
    auto gGam   = new TGraphErrors();  gGam  ->SetTitle("gammas");

    printf("\nParticles leaving the moderator BACK face, per cm2 per primary:\n");
    printf("  HDPE   thermal<0.5eV      thermal<0.025eV   thermal<0.5eV,<18deg  fast>50keV        gammas      thermal/fast\n");

    int ip = 0;
    for (int t : thick) {
        TFile* f = TFile::Open(Form("sweep_%dcm.root", t));
        if (!f || f->IsZombie()) { printf("  %2d cm  (file missing - skipped)\n", t); continue; }
        TH1D*  hPrim = (TH1D*)  f->Get("nPrimaries");
        TTree* tx    = (TTree*) f->Get("ModExit");
        if (!hPrim || !tx) { printf("  %2d cm  (nPrimaries or ModExit missing - skipped)\n", t); f->Close(); continue; }
        const double nPrim = hPrim->GetEntries();
        const double norm  = 1. / (nPrim * faceArea);

        // counts
        double nThCd  = tx->GetEntries(cutThCd);
        double nTh025 = tx->GetEntries(cutTh025);
        double nAcc   = tx->GetEntries(cutAcc);
        double nFast  = tx->GetEntries(cutFast);
        double nGam   = tx->GetEntries(cutGam);

        auto setP = [&](TGraphErrors* g, double n) {
            g->SetPoint(ip, t, n*norm);
            g->SetPointError(ip, 0., std::sqrt(n)*norm);
        };
        setP(gThCd, nThCd); setP(gTh025, nTh025); setP(gAcc, nAcc); setP(gFast, nFast); setP(gGam, nGam);
        ++ip;

        printf("  %2d cm  %.2e (%4.0f)   %.2e (%4.0f)   %.2e (%4.0f)     %.2e (%4.0f)   %.2e (%4.0f)   %.2f\n",
               t, nThCd*norm, nThCd, nTh025*norm, nTh025, nAcc*norm, nAcc,
               nFast*norm, nFast, nGam*norm, nGam, nFast > 0 ? nThCd/nFast : 0.);
        f->Close();
    }
    printf("  (numbers in brackets = raw counts; statistical error = 1/sqrt(counts))\n");
    if (ip == 0) { printf("No sweep files found.\n"); return; }

    // ---- plots ----
    gStyle->SetOptStat(0);
    auto style = [](TGraphErrors* g, int col, int mk) {
        g->SetLineColor(col); g->SetMarkerColor(col); g->SetMarkerStyle(mk); g->SetLineWidth(2);
    };
    style(gThCd, kBlue+1, 20); style(gTh025, kAzure+7, 24); style(gAcc, kGreen+2, 21);
    style(gFast, kRed+1, 20);  style(gGam, kOrange+7, 22);

    TCanvas* c = new TCanvas("c", "Thickness sweep", 1200, 500);
    c->Divide(2, 1);

    c->cd(1);
    auto mgTh = new TMultiGraph();
    mgTh->SetTitle("Thermal neutrons leaving back face;HDPE thickness (cm);per cm^{2} per primary");
    mgTh->Add(gThCd, "PL"); mgTh->Add(gTh025, "PL"); mgTh->Add(gAcc, "PL");
    mgTh->Draw("A");
    mgTh->SetMinimum(0.);
    auto l1 = new TLegend(0.45, 0.72, 0.89, 0.89);
    l1->AddEntry(gThCd, gThCd->GetTitle(), "pl");
    l1->AddEntry(gTh025, gTh025->GetTitle(), "pl");
    l1->AddEntry(gAcc, gAcc->GetTitle(), "pl");
    l1->Draw();

    c->cd(2);
    gPad->SetLogy();
    auto mgBg = new TMultiGraph();
    mgBg->SetTitle("Fast neutrons and gammas leaving back face;HDPE thickness (cm);per cm^{2} per primary");
    mgBg->Add(gThCd, "PL"); mgBg->Add(gFast, "PL"); mgBg->Add(gGam, "PL");
    mgBg->Draw("A");
    auto l2 = new TLegend(0.55, 0.75, 0.89, 0.89);
    l2->AddEntry(gThCd, "thermal < 0.5 eV", "pl");
    l2->AddEntry(gFast, gFast->GetTitle(), "pl");
    l2->AddEntry(gGam, gGam->GetTitle(), "pl");
    l2->Draw();

    c->SaveAs("sweep_thickness.png");
}
