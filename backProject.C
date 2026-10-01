// Where did each particle LEAVE the material before flying to the screen?
// Run from the build folder:   root -l './backProject.C'
//
// Between the rear lead plate and the screen there is only vacuum (1 cm), so each particle
// flew in a straight line over that last centimetre. We trace that line back to the
// back face of the rear lead plate:
//   - inside the cone opening (r <= R_hole): it came out through the HOLE
//   - outside the opening:                   it came out of the LEAD PLATE material (the "walls")
// Defaults match origin.mac: screen gap 50 mm, rear lead 40 mm, pinhole D 10 mm, L 30 mm.
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TEllipse.h"
#include "TStyle.h"
#include <cmath>
#include <cstdio>

void backProject(const char* fileName = "origin.root",
                 double gap_mm = 50., double rear_mm = 40., double D_mm = 10., double L_mm = 30.)
{
    TFile* f = TFile::Open(fileName);
    if (!f || f->IsZombie()) { printf("Cannot open %s\n", fileName); return; }
    TH1D*  hPrim = (TH1D*)  f->Get("nPrimaries");
    TTree* t     = (TTree*) f->Get("Screen");
    if (!hPrim || !t || !t->GetBranch("dirX")) { printf("Need a file made with the new code (dirX column).\n"); return; }
    const double nPrim = hPrim->GetEntries();

    const double d     = gap_mm - rear_mm;                 // vacuum between plate and screen
    const double Rhole = 0.5*D_mm + rear_mm*D_mm/L_mm;     // cone radius at the plate's back face
    const double rSpot = 30.;

    int pdg; double E, x, y, c, dx, dy;
    t->SetBranchAddress("pdg", &pdg);      t->SetBranchAddress("E_eV", &E);
    t->SetBranchAddress("x_mm", &x);       t->SetBranchAddress("y_mm", &y);
    t->SetBranchAddress("cosTheta", &c);   t->SetBranchAddress("dirX", &dx);
    t->SetBranchAddress("dirY", &dy);

    double spot[4][2] = {{0}}, screen[4][2] = {{0}};
    const char* clsName[4] = {"thermal n (<0.5 eV)", "epithermal n", "fast n (>50 keV)", "gammas"};

    TH2D* mFast = new TH2D("mFast", "Fast n in the spot: where they left the plate;x (mm);y (mm)", 60, -30, 30, 60, -30, 30);
    TH2D* mSlow = new TH2D("mSlow", "Thermal + epithermal n in the spot;x (mm);y (mm)",            60, -30, 30, 60, -30, 30);

    const Long64_t n = t->GetEntries();
    for (Long64_t i = 0; i < n; ++i) {
        t->GetEntry(i);
        if (c <= 0) continue;
        int cls;
        if (pdg == 2112) cls = (E < 0.5) ? 0 : (E < 5.e4 ? 1 : 2);
        else if (pdg == 22) cls = 3;
        else continue;

        const double xb = x - dx/c*d, yb = y - dy/c*d;   // position on the plate's back face
        const int origin = (std::sqrt(xb*xb + yb*yb) <= Rhole) ? 0 : 1;

        screen[cls][origin] += 1;
        if (x*x + y*y < rSpot*rSpot) {
            spot[cls][origin] += 1;
            if (cls == 2) mFast->Fill(xb, yb);
            if (cls == 0 || cls == 1) mSlow->Fill(xb, yb);
        }
    }

    auto table = [&](const char* title, double cnt[4][2]) {
        printf("\n%s\n", title);
        printf("  %-22s %8s   %-18s %-18s\n", "particles", "total", "through the HOLE", "out of the LEAD");
        for (int k = 0; k < 4; ++k) {
            double tot = cnt[k][0] + cnt[k][1];
            auto pc = [&](double v) { return tot > 0 ? 100.*v/tot : 0.; };
            printf("  %-22s %8.0f   %6.0f (%5.1f%%)    %6.0f (%5.1f%%)\n",
                   clsName[k], tot, cnt[k][0], pc(cnt[k][0]), cnt[k][1], pc(cnt[k][1]));
        }
    };
    printf("\nFile %s, %.0f primaries. Hole radius at the plate's back face: %.1f mm\n", fileName, nPrim, Rhole);
    table("BEAM SPOT (r < 30 mm):", spot);
    table("WHOLE SCREEN:", screen);

    gStyle->SetOptStat(0);
    TCanvas* cv = new TCanvas("cv", "Back projection", 1100, 500);
    cv->Divide(2, 1);
    for (int k = 0; k < 2; ++k) {
        cv->cd(k + 1); gPad->SetRightMargin(0.15);
        (k == 0 ? mFast : mSlow)->Draw("colz");
        TEllipse* e = new TEllipse(0., 0., Rhole, Rhole);
        e->SetFillStyle(0); e->SetLineColor(kRed); e->SetLineWidth(2); e->Draw();
    }
    cv->SaveAs("back_projection.png");
}
