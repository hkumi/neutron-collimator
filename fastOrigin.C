// Where do the particles reaching the screen come from?
// Run from the build folder:   root -l './fastOrigin.C'
//
// Each particle at the screen carries "path" bits (see trackflags.hh):
//   1 moderator, 2 lead brick, 4 side HDPE blocks, 8 borated sheet, 16 rear lead plate
// Categories:
//   AXIS   : came through the hole, is the source neutron itself, and only crossed the
//            central moderator + lead brick (the straight line from source to pinhole)
//   HOLE   : came through the hole (never inside the borated sheet or rear lead), other paths
//            (e.g. scattered in from the side blocks, or a secondary particle)
//   WALLS  : crossed the borated sheet and/or the rear lead plate material
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLegend.h"
#include "TStyle.h"
#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>

void fastOrigin(const char* fileName = "origin.root")
{
    TFile* f = TFile::Open(fileName);
    if (!f || f->IsZombie()) { printf("Cannot open %s\n", fileName); return; }
    TH1D*  hPrim = (TH1D*)  f->Get("nPrimaries");
    TTree* t     = (TTree*) f->Get("Screen");
    if (!hPrim || !t) { printf("nPrimaries or Screen missing\n"); return; }
    if (!t->GetBranch("path")) { printf("No 'path' column: this file was made with the old code.\n"); return; }
    const double nPrim = hPrim->GetEntries();
    const double rSpot = 30.;   // mm

    int pdg, path, isPrim; double E, x, y;
    t->SetBranchAddress("pdg", &pdg);   t->SetBranchAddress("E_eV", &E);
    t->SetBranchAddress("x_mm", &x);    t->SetBranchAddress("y_mm", &y);
    t->SetBranchAddress("path", &path); t->SetBranchAddress("isPrimary", &isPrim);

    // counts[particle class][origin]; classes: 0 thermal, 1 epithermal, 2 fast, 3 gamma
    // origins: 0 AXIS, 1 HOLE, 2 WALLS
    double spot[4][3] = {{0}}, screen[4][3] = {{0}};
    const char* clsName[4] = {"thermal n (<0.5 eV)", "epithermal n", "fast n (>50 keV)", "gammas"};

    // log energy bins for neutron spectra in the spot
    const int nb = 110; std::vector<double> edges(nb + 1);
    for (int i = 0; i <= nb; ++i) edges[i] = std::pow(10., -4. + i/10.);
    TH1D* hAxis = new TH1D("hAxis", "Neutrons in the beam spot by origin;Energy (eV);neutrons per primary (per bin)", nb, edges.data());
    TH1D* hHole = new TH1D("hHole", "", nb, edges.data());
    TH1D* hWall = new TH1D("hWall", "", nb, edges.data());
    // radial distribution of fast neutrons on the whole screen
    TH1D* rHole = new TH1D("rHole", "Fast neutrons (>50 keV) on the screen;radius r (mm);n per cm^{2} per primary", 30, 0., 150.);
    TH1D* rWall = new TH1D("rWall", "", 30, 0., 150.);

    const Long64_t n = t->GetEntries();
    for (Long64_t i = 0; i < n; ++i) {
        t->GetEntry(i);
        int cls;
        if (pdg == 2112) cls = (E < 0.5) ? 0 : (E < 5.e4 ? 1 : 2);
        else if (pdg == 22) cls = 3;
        else continue;

        const bool throughWalls = (path & (8 | 16)) != 0;
        const bool onAxis = !throughWalls && isPrim == 1 && (path & 4) == 0;
        const int origin = throughWalls ? 2 : (onAxis ? 0 : 1);

        const double r = std::sqrt(x*x + y*y);
        screen[cls][origin] += 1;
        if (r < rSpot) {
            spot[cls][origin] += 1;
            if (pdg == 2112) { TH1D* h = origin == 0 ? hAxis : (origin == 1 ? hHole : hWall); h->Fill(E); }
        }
        if (cls == 2) (throughWalls ? rWall : rHole)->Fill(r);
    }

    auto table = [&](const char* title, double c[4][3]) {
        printf("\n%s\n", title);
        printf("  %-22s %8s   %-16s %-16s %-16s\n", "particles", "total", "AXIS (hole)", "HOLE (other)", "WALLS");
        for (int k = 0; k < 4; ++k) {
            double tot = c[k][0] + c[k][1] + c[k][2];
            auto pc = [&](double v) { return tot > 0 ? 100.*v/tot : 0.; };
            printf("  %-22s %8.0f   %6.0f (%5.1f%%)  %6.0f (%5.1f%%)  %6.0f (%5.1f%%)\n",
                   clsName[k], tot, c[k][0], pc(c[k][0]), c[k][1], pc(c[k][1]), c[k][2], pc(c[k][2]));
        }
    };
    printf("\nFile %s, %.0f primaries. Raw counts (percent of row).\n", fileName, nPrim);
    table("BEAM SPOT (r < 30 mm):", spot);
    table("WHOLE SCREEN:", screen);

    // normalise
    for (TH1D* h : {hAxis, hHole, hWall}) h->Scale(1./nPrim);
    for (TH1D* h : {rHole, rWall}) {
        for (int b = 1; b <= h->GetNbinsX(); ++b) {
            double r1 = h->GetBinLowEdge(b)/10., r2 = (h->GetBinLowEdge(b) + h->GetBinWidth(b))/10.;  // cm
            double area = M_PI*(r2*r2 - r1*r1);
            h->SetBinContent(b, h->GetBinContent(b)/area/nPrim);
            h->SetBinError(b, h->GetBinError(b)/area/nPrim);
        }
    }

    gStyle->SetOptStat(0);
    TCanvas* c = new TCanvas("c", "Origin", 1200, 450);
    c->Divide(2, 1);
    c->cd(1); gPad->SetLogx(); gPad->SetLogy();
    hAxis->SetLineColor(kBlue+1); hHole->SetLineColor(kGreen+2); hWall->SetLineColor(kRed+1);
    for (TH1D* h : {hAxis, hHole, hWall}) h->SetLineWidth(2);
    double mx = std::max(hAxis->GetMaximum(), std::max(hHole->GetMaximum(), hWall->GetMaximum()));
    hAxis->SetMaximum(3*mx); hAxis->SetMinimum(0.5/nPrim);
    hAxis->Draw("hist"); hHole->Draw("hist same"); hWall->Draw("hist same");
    auto l1 = new TLegend(0.15, 0.72, 0.6, 0.89);
    l1->AddEntry(hAxis, "AXIS: straight through moderator + brick", "l");
    l1->AddEntry(hHole, "HOLE: other paths through the hole", "l");
    l1->AddEntry(hWall, "WALLS: through sheet / rear lead", "l");
    l1->Draw();

    c->cd(2); gPad->SetLogy();
    rHole->SetLineColor(kBlue+1); rWall->SetLineColor(kRed+1);
    rHole->SetLineWidth(2); rWall->SetLineWidth(2);
    rHole->SetMaximum(3*std::max(rHole->GetMaximum(), rWall->GetMaximum()));
    rHole->Draw("hist e"); rWall->Draw("hist e same");
    auto l2 = new TLegend(0.5, 0.75, 0.89, 0.89);
    l2->AddEntry(rHole, "through the hole (AXIS + HOLE)", "l");
    l2->AddEntry(rWall, "through the walls", "l");
    l2->Draw();
    c->SaveAs("fast_origin.png");
}
