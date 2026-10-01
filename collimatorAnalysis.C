// ROOT analysis for the collimator simulation.
// Run from the build folder:   root -l './collimatorAnalysis.C("collimator_run3.root")'
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TLine.h"
#include "TStyle.h"
#include "TPad.h"
#include "TSystem.h"
#include <vector>
#include <cmath>
#include <cstdio>

void collimatorAnalysis(const char* fileName = "collimator_run3.root")
{
    // ---- energy bands (eV), from Juan's email ----
    const double eThermalMax = 0.025;  // thermal:     0      - 0.025 eV
    const double eEpiMax     = 50.e3;  // epithermal:  0.025 eV - 50 keV
    const double eFastMax    = 5.e6;   // fast:        50 keV  - 5 MeV

    TFile* f = TFile::Open(fileName);
    if (!f || f->IsZombie()) { printf("Cannot open %s\n", fileName); return; }

    TH1D*  hPrim = (TH1D*)  f->Get("nPrimaries");
    TTree* t     = (TTree*) f->Get("Screen");
    if (!hPrim || !t) { printf("nPrimaries or Screen missing in %s\n", fileName); return; }

    const double nPrim = hPrim->GetEntries();
    // picture names include the run name, e.g. spectra_collimator_run2.png
    TString tag = gSystem->BaseName(fileName);
    tag.ReplaceAll(".root", "");
    printf("\nFile: %s\nPrimaries: %.0f\n", fileName, nPrim);

    // ---- cuts ----
    TString cutN  = "pdg==2112";
    TString cutTh = Form("pdg==2112 && E_eV <  %g", eThermalMax);
    TString cutEp = Form("pdg==2112 && E_eV >= %g && E_eV < %g", eThermalMax, eEpiMax);
    TString cutFa = Form("pdg==2112 && E_eV >= %g && E_eV <= %g", eEpiMax, eFastMax);
    TString cutG  = "pdg==22";

    // ---- 1) summary numbers, per primary ----
    auto report = [&](const char* label, const TString& cut) {
        double n = t->GetEntries(cut);
        printf("  %-22s %10.0f counts  ->  %.3e +- %.1e per primary\n",
               label, n, n/nPrim, std::sqrt(n)/nPrim);
    };
    printf("\nParticles reaching the screen:\n");
    report("all neutrons",        cutN);
    report("thermal (<0.025 eV)", cutTh);
    report("thermal (<0.5 eV, Cd)", "pdg==2112 && E_eV < 0.5");
    report("epithermal",          cutEp);
    report("fast (>50 keV)",      cutFa);
    report("gammas",              cutG);

    // NEW: same numbers, but only inside the beam spot behind the pinhole
    const double rSpot = 30.;   // mm. Pinhole D = 1 cm, L = 3 cm, gap 5 cm -> spot radius ~2 cm
    TString inSpot = Form("(x_mm*x_mm + y_mm*y_mm) < %g", rSpot*rSpot);
    auto spot = [&](const TString& cut) { return TString::Format("(%s) && (%s)", cut.Data(), inSpot.Data()); };
    printf("\nInside the beam spot (r < %.0f mm):\n", rSpot);
    report("all neutrons",        spot(cutN));
    report("thermal (<0.025 eV)", spot(cutTh));
    report("thermal (<0.5 eV, Cd)", spot("pdg==2112 && E_eV < 0.5"));
    report("epithermal",          spot(cutEp));
    report("fast (>50 keV)",      spot(cutFa));
    report("gammas",              spot(cutG));

    // ---- 2) energy spectra with a LOG energy axis ----
    const int perDecade = 10;
    const int nBins = 11 * perDecade;               // 1e-4 eV ... 1e7 eV
    std::vector<double> edges(nBins + 1);
    for (int i = 0; i <= nBins; ++i) edges[i] = std::pow(10., -4. + double(i)/perDecade);

    TH1D* hN = new TH1D("hN", "Neutrons at screen;Energy (eV);neutrons per primary (per bin)",
                        nBins, edges.data());
    TH1D* hG = new TH1D("hG", "Gammas at screen;Energy (eV);gammas per primary (per bin)",
                        nBins, edges.data());
    t->Draw("E_eV>>hN", cutN, "goff");
    t->Draw("E_eV>>hG", cutG, "goff");
    hN->Scale(1./nPrim);
    hG->Scale(1./nPrim);

    // ---- 3) spatial maps at the screen, split by energy ----
    const int nXY = 60; const double half = 150.;   // screen is 30 x 30 cm -> 5 mm pixels
    TH2D* hTh = new TH2D("hTh", "Thermal neutrons (< 0.025 eV);x (mm);y (mm)",       nXY,-half,half, nXY,-half,half);
    TH2D* hEp = new TH2D("hEp", "Epithermal neutrons (0.025 eV - 50 keV);x (mm);y (mm)", nXY,-half,half, nXY,-half,half);
    TH2D* hFa = new TH2D("hFa", "Fast neutrons (50 keV - 5 MeV);x (mm);y (mm)",      nXY,-half,half, nXY,-half,half);
    TH2D* hGa = new TH2D("hGa", "Gammas;x (mm);y (mm)",                              nXY,-half,half, nXY,-half,half);
    t->Draw("y_mm:x_mm>>hTh", cutTh, "goff");
    t->Draw("y_mm:x_mm>>hEp", cutEp, "goff");
    t->Draw("y_mm:x_mm>>hFa", cutFa, "goff");
    t->Draw("y_mm:x_mm>>hGa", cutG,  "goff");
    for (TH2D* h : {hTh, hEp, hFa, hGa}) {
        h->Scale(1./nPrim);
        h->GetZaxis()->SetTitle("per primary per pixel");
    }

    // ---- draw and save ----
    gStyle->SetOptStat(0);

    TCanvas* c1 = new TCanvas("c1", "Spectra", 1100, 450);
    c1->Divide(2, 1);
    c1->cd(1); gPad->SetLogx(); gPad->SetLogy(); hN->SetLineWidth(2); hN->Draw("hist");
    gPad->Update();
    double y1 = std::pow(10., gPad->GetUymin()), y2 = std::pow(10., gPad->GetUymax());
    for (double e : {eThermalMax, eEpiMax}) {           // band limits as dashed lines
        TLine* l = new TLine(e, y1, e, y2); l->SetLineStyle(2); l->SetLineColor(kRed); l->Draw();
    }
    c1->cd(2); gPad->SetLogx(); gPad->SetLogy(); hG->SetLineWidth(2); hG->SetLineColor(kOrange+7); hG->Draw("hist");
    c1->SaveAs(Form("spectra_%s.png", tag.Data()));

    TCanvas* c2 = new TCanvas("c2", "Maps", 1100, 1000);
    c2->Divide(2, 2);
    c2->cd(1); gPad->SetRightMargin(0.15); hTh->Draw("colz");
    c2->cd(2); gPad->SetRightMargin(0.15); hEp->Draw("colz");
    c2->cd(3); gPad->SetRightMargin(0.15); hFa->Draw("colz");
    c2->cd(4); gPad->SetRightMargin(0.15); hGa->Draw("colz");
    c2->SaveAs(Form("maps_%s.png", tag.Data()));

    // =====================================================================
    // 4) NEW: particles leaving the BACK face of the moderator (10 x 10 cm)
    //    = what is available to go into the pinhole
    // =====================================================================
    TTree* tx = (TTree*) f->Get("ModExit");
    if (!tx) { printf("\n(No ModExit tree: this file was made before the exit diagnostic.)\n"); return; }

    const double faceArea = 100.;               // cm2
    const double pinholeD = 10., pinholeL = 30.; // mm  (change if you change the pinhole)
    const double cosAcc   = std::cos(std::atan(pinholeD/pinholeL));   // pinhole acceptance angle

    auto reportX = [&](const char* label, const TString& cut) {
        double n = tx->GetEntries(cut);
        printf("  %-30s %9.0f counts -> %.3e per primary, %.3e per cm2 per primary (+- %.0f%%)\n",
               label, n, n/nPrim, n/nPrim/faceArea, n > 0 ? 100./std::sqrt(n) : 0.);
    };
    TString thCd = "pdg==2112 && E_eV < 0.5";
    printf("\nLeaving the BACK face of the moderator (10 x 10 cm):\n");
    reportX("all neutrons",            cutN);
    reportX("thermal (<0.025 eV)",     cutTh);
    reportX("thermal (<0.5 eV, Cd)",   thCd);
    reportX("epithermal",              cutEp);
    reportX("fast (>50 keV)",          cutFa);
    reportX(Form("thermal <0.5 eV, angle < %.0f deg", std::acos(cosAcc)*180./M_PI),
            Form("(%s) && cosTheta > %g", thCd.Data(), cosAcc));
    reportX("gammas",                  cutG);

    TH1D* hXn = new TH1D("hXn", "Neutrons leaving moderator back face;Energy (eV);neutrons per primary (per bin)",
                         nBins, edges.data());
    TH2D* hXth = new TH2D("hXth", "Thermal (<0.5 eV) leaving back face;x (mm);y (mm)", 20,-50,50, 20,-50,50);
    TH1D* hXang = new TH1D("hXang", "Thermal (<0.5 eV) leaving back face;cos#theta (1 = along beam);per primary",
                           20, 0., 1.);
    tx->Draw("E_eV>>hXn", cutN, "goff");
    tx->Draw("y_mm:x_mm>>hXth", thCd, "goff");
    tx->Draw("cosTheta>>hXang", thCd, "goff");
    hXn->Scale(1./nPrim); hXth->Scale(1./nPrim); hXang->Scale(1./nPrim);

    TCanvas* c3 = new TCanvas("c3", "Moderator exit", 1500, 450);
    c3->Divide(3, 1);
    c3->cd(1); gPad->SetLogx(); gPad->SetLogy(); hXn->SetLineWidth(2); hXn->Draw("hist");
    c3->cd(2); gPad->SetRightMargin(0.15); hXth->Draw("colz");
    c3->cd(3); hXang->SetLineWidth(2); hXang->Draw("hist");
    c3->SaveAs(Form("exit_%s.png", tag.Data()));
}
