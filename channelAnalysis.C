// Graphite channel sweep (second layer of Juan's sketch).
// Run from the build folder:   root -l './channelAnalysis.C'
// Main results at the SCREEN (beam spot r < 30 mm). Also: back-projection of fast neutrons
// in the spot to the rear lead plate (through the hole vs out of the lead).
//
// Two places are analysed:
//   EXIT  = particles leaving the back face of the last block before the pinhole layer
//           (moderator, or lead if present), 10 x 10 cm  -> good statistics, for comparing designs
//   SPOT  = particles reaching the screen inside r < 30 mm behind the pinhole -> the real beam, low statistics
//
// Fluence (particles per cm2) from surface crossings: each particle counts 1/|cos(theta)| / area
// (|cos theta| limited to >= 0.1 to avoid huge weights from grazing particles).
// Gamma dose: ambient dose equivalent H*(10) using ICRP 74 photon conversion coefficients.
// Cadmium ratio = response of a bare detector / response of the same detector covered with cadmium.
//   Cd absorbs everything below ~0.5 eV, so: R_Cd = (all neutrons) / (neutrons above 0.5 eV).
//   Two versions are printed:
//   - "counts": every neutron counts the same (simple, NOT what experiments measure)
//   - "1/v":    each neutron weighted by 1/sqrt(E), like an activation foil (Mn, Au) or other 1/v detector.
//               This is the one to compare with measured Cd ratios (e.g. Yang 2014: Cd-ratio(Mn) > 150).
//               Approximation: foil resonances (e.g. Mn at 337 eV) are ignored.
// n/g targets: Li et al. 2020: n/g > 1e10 n cm-2 Sv-1.
//              MacGillivray: n/g >= 1e6 n cm-2 mR-1  ~ 1e11 n cm-2 Sv-1 (using 1 mR ~ 1e-5 Sv).
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TGraphErrors.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLine.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TAxis.h"
#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>

// ICRP 74: photon fluence -> H*(10), in pSv cm2. Energy in MeV.
static const double kE[]  = {0.010, 0.015, 0.020, 0.030, 0.040, 0.050, 0.060, 0.080, 0.10, 0.15,
                             0.20, 0.30, 0.40, 0.50, 0.60, 0.80, 1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.0, 8.0, 10.0};
static const double kH[]  = {0.061, 0.83, 1.05, 0.81, 0.64, 0.55, 0.51, 0.53, 0.61, 0.89,
                             1.20, 1.80, 2.38, 2.93, 3.44, 4.38, 5.20, 6.90, 8.60, 11.1, 13.4, 15.5, 17.6, 21.6, 25.6};
static const int    kN    = 25;

double doseCoeff_pSvcm2(double E_MeV)   // log-log interpolation
{
    if (E_MeV < kE[0]) return 0.;
    if (E_MeV >= kE[kN-1]) return kH[kN-1];
    int i = 0; while (E_MeV > kE[i+1]) ++i;
    double f = std::log(E_MeV/kE[i]) / std::log(kE[i+1]/kE[i]);
    return std::exp(std::log(kH[i]) + f*std::log(kH[i+1]/kH[i]));
}

struct Result {
    double phiTh025 = 0, phiTh05 = 0, phiFast = 0;  // n per cm2 per primary
    double nTh05 = 0;                               // raw counts (for errors)
    double doseSv = 0;                              // Sv per primary
    double nGam = 0;
    double nAll = 0, nAboveCd = 0;                  // counts, for the Cd ratio
    double vAll = 0, vAboveCd = 0;                  // 1/v-weighted fluence, for the Cd ratio
    double cdCounts() const { return nAboveCd > 0 ? nAll/nAboveCd : 0.; }
    double cdOneOverV() const { return vAboveCd > 0 ? vAll/vAboveCd : 0.; }
};

// Loop over a tree (Screen or ModExit). If rMax > 0, keep only r < rMax (mm).
Result analyse(TTree* t, double area_cm2, double nPrim, double rMax)
{
    Result r;
    int pdg; double E, x, y, c;
    t->SetBranchAddress("pdg", &pdg);
    t->SetBranchAddress("E_eV", &E);
    t->SetBranchAddress("x_mm", &x);
    t->SetBranchAddress("y_mm", &y);
    t->SetBranchAddress("cosTheta", &c);
    const Long64_t n = t->GetEntries();
    for (Long64_t i = 0; i < n; ++i) {
        t->GetEntry(i);
        if (rMax > 0 && x*x + y*y >= rMax*rMax) continue;
        const double w = 1. / std::max(std::fabs(c), 0.1);       // fluence weight
        if (pdg == 2112) {
            r.nAll += 1;
            if (E >= 0.5)   r.nAboveCd += 1;
            const double wv = (E > 0) ? w / std::sqrt(E) : 0.;   // 1/v detector response
            r.vAll += wv;
            if (E >= 0.5)   r.vAboveCd += wv;
            if (E < 0.025)  r.phiTh025 += w;
            if (E < 0.5)  { r.phiTh05  += w; r.nTh05 += 1; }
            if (E >= 5.e4)  r.phiFast  += w;
        } else if (pdg == 22) {
            r.nGam  += 1;
            r.doseSv += w * doseCoeff_pSvcm2(E*1.e-6) * 1.e-12;   // pSv -> Sv
        }
    }
    const double norm = 1. / (area_cm2 * nPrim);
    r.phiTh025 *= norm; r.phiTh05 *= norm; r.phiFast *= norm; r.doseSv *= norm;
    t->ResetBranchAddresses();
    return r;
}


// fraction of fast neutrons in the spot that left through the rear-lead opening
void backFraction(TTree* t, double gap_mm, double rear_mm, double D_mm, double L_mm, double& nHole, double& nLead)
{
    nHole = nLead = 0;
    if (!t->GetBranch("dirX")) return;
    const double d = gap_mm - rear_mm, Rhole = 0.5*D_mm + rear_mm*D_mm/L_mm;
    int pdg; double E, x, y, c, dx, dy;
    t->SetBranchAddress("pdg", &pdg); t->SetBranchAddress("E_eV", &E);
    t->SetBranchAddress("x_mm", &x);  t->SetBranchAddress("y_mm", &y);
    t->SetBranchAddress("cosTheta", &c); t->SetBranchAddress("dirX", &dx); t->SetBranchAddress("dirY", &dy);
    for (Long64_t i = 0; i < t->GetEntries(); ++i) {
        t->GetEntry(i);
        if (pdg != 2112 || E < 5.e4 || c <= 0 || x*x + y*y >= 900.) continue;
        const double xb = x - dx/c*d, yb = y - dy/c*d;
        if (xb*xb + yb*yb <= Rhole*Rhole) nHole += 1; else nLead += 1;
    }
    t->ResetBranchAddresses();
}

void channelAnalysis(const char* prefix = "chan")
{
    const std::vector<int> chan = {0, 2, 4, 6};    // cm, must match channelSweep.sh
    const double rSpot    = 30.;                  // mm
    const double areaSpot = M_PI * 3.0 * 3.0;     // cm2
    const double areaScr  = 30. * 30.;            // cm2
    const double areaExit = 100.;                 // cm2

    auto gTh = new TGraphErrors(); auto gFast = new TGraphErrors(); auto gDose = new TGraphErrors();
    // NEW: same quantities inside the real beam footprint at the screen
    //      r_beam = D/2 + gap * D/L  (straight lines through a channel of length L, diameter D)
    auto gNGb = new TGraphErrors(); auto gCdb = new TGraph(); auto gThb = new TGraphErrors();
    std::vector<TString> footLines;

    printf("\nModerator 6 cm, lead brick 4 cm, rear lead 4 cm. Graphite channel behind the brick. Per primary.\n");
    printf("\n SCREEN SPOT (r < 30 mm):\n");
    printf("  Chan  L/D   thermal<0.5eV     fast>50keV        gamma dose         n/g ratio    Cd(1/v)  fast: hole / lead\n");
    printf("  (cm)        (n/cm2) [counts]  (n/cm2) [counts]  (Sv) [counts]      (n cm-2 Sv-1)\n");

    int ip = 0; std::vector<double> exitTh, exitFast;
    for (int c : chan) {
        TFile* f = TFile::Open(Form("%s_%dcm.root", prefix, c));
        if (!f || f->IsZombie()) { printf("  %d cm  (file missing - skipped)\n", c); continue; }
        TH1D*  hPrim = (TH1D*)  f->Get("nPrimaries");
        TTree* tx    = (TTree*) f->Get("ModExit");
        TTree* ts    = (TTree*) f->Get("Screen");
        if (!hPrim || !tx || !ts) { printf("  %d cm  (missing objects - skipped)\n", c); f->Close(); continue; }
        const double nPrim = hPrim->GetEntries();
        const double L = 30. + 10.*c;   // mm, total channel length

        Result s = analyse(ts, areaSpot, nPrim, rSpot);
        Result x = analyse(tx, areaExit, nPrim, -1.);
        exitTh.push_back(x.phiTh05); exitFast.push_back(x.phiFast);
        double nFast = ts->GetEntries(Form("pdg==2112 && E_eV >= 5e4 && x_mm*x_mm+y_mm*y_mm < %g", rSpot*rSpot));
        double nHole, nLead; backFraction(ts, 50., 40., 10., L, nHole, nLead);

        const double rBeam = 5. + 50.*10./L;                       // mm
        const double areaBeam = M_PI * (rBeam/10.) * (rBeam/10.);  // cm2
        Result bm = analyse(ts, areaBeam, nPrim, rBeam);
        const double ngB = bm.doseSv > 0 ? bm.phiTh05/bm.doseSv : 0.;
        const double fastOverTh = bm.phiTh05 > 0 ? bm.phiFast/bm.phiTh05 : 0.;
        footLines.push_back(Form("  %d     %4.1f  %4.1f      %.2e [%4.0f]   %.2e       %.2e [%4.0f]   %.2e     %5.2f     %5.1f",
               c, L/10., rBeam, bm.phiTh05, bm.nTh05, bm.phiFast, bm.doseSv, bm.nGam, ngB, bm.cdOneOverV(), fastOverTh));
        const double relTh = bm.nTh05 > 0 ? 1./std::sqrt(bm.nTh05) : 0.;
        const double relG  = bm.nGam  > 0 ? 1./std::sqrt(bm.nGam)  : 0.;
        gThb->SetPoint(ip, c, bm.phiTh05); gThb->SetPointError(ip, 0., bm.phiTh05*relTh);
        gNGb->SetPoint(ip, c, ngB);        gNGb->SetPointError(ip, 0., ngB*std::sqrt(relTh*relTh + relG*relG));
        gCdb->SetPoint(ip, c, bm.cdOneOverV());

        printf("  %d     %4.1f  %.2e [%3.0f]    %.2e [%4.0f]   %.2e [%3.0f]    %.2e     %5.2f    %3.0f / %3.0f\n",
               c, L/10., s.phiTh05, s.nTh05, s.phiFast, nFast, s.doseSv, s.nGam,
               s.doseSv > 0 ? s.phiTh05/s.doseSv : 0., s.cdOneOverV(), nHole, nLead);

        auto rel = [](double n) { return n > 0 ? 1./std::sqrt(n) : 0.; };
        gTh  ->SetPoint(ip, c, s.phiTh05); gTh  ->SetPointError(ip, 0., s.phiTh05*rel(s.nTh05));
        gFast->SetPoint(ip, c, s.phiFast); gFast->SetPointError(ip, 0., s.phiFast*rel(nFast));
        gDose->SetPoint(ip, c, s.doseSv);  gDose->SetPointError(ip, 0., s.doseSv*rel(s.nGam));
        ++ip;
        f->Close();
    }
    if (ip == 0) { printf("No files found.\n"); return; }

    printf("\n BEAM FOOTPRINT (r < r_beam = D/2 + gap*D/L): the area the pinhole beam actually covers\n");
    printf("  Chan  L/D   r_beam    thermal<0.5eV       fast>50keV     gamma dose          n/g ratio    Cd(1/v)   fast/thermal\n");
    printf("  (cm)        (mm)      (n/cm2) [counts]    (n/cm2)        (Sv) [counts]       (n cm-2 Sv-1)\n");
    for (auto& l : footLines) printf("%s\n", l.Data());

    printf("\n Check - at the lead brick back face (should not change much): thermal / fast (n/cm2)\n  ");
    for (size_t i = 0; i < exitTh.size(); ++i) printf("%.2e / %.2e   ", exitTh[i], exitFast[i]);
    printf("\n\n Error bars use 1/sqrt(counts). Spot thermal counts are low: treat thermal and n/g as rough.\n");

    gStyle->SetOptStat(0);
    auto style = [](TGraph* g, int col, int mk) {
        g->SetLineColor(col); g->SetMarkerColor(col); g->SetMarkerStyle(mk); g->SetLineWidth(2);
    };
    style(gFast, kRed+1, 20); style(gTh, kBlue+1, 21); style(gDose, kOrange+7, 22);

    TCanvas* cv = new TCanvas("cv", "Channel sweep", 1500, 450);
    cv->Divide(3, 1);
    cv->cd(1); gFast->SetTitle("Fast neutrons (> 50 keV) in the beam spot;Graphite channel length (cm);n per cm^{2} per primary");
    gFast->SetMinimum(0.); gFast->Draw("APL");
    cv->cd(2); gTh->SetTitle("Thermal (< 0.5 eV) in the beam spot (low stats);Graphite channel length (cm);n per cm^{2} per primary");
    gTh->SetMinimum(0.); gTh->Draw("APL");
    cv->cd(3); gDose->SetTitle("Gamma dose in the beam spot;Graphite channel length (cm);Sv per primary");
    gDose->SetMinimum(0.); gDose->Draw("APL");
    cv->SaveAs(Form("%s_sweep.png", prefix));

    // NEW: footprint plots
    style(gThb, kBlue+1, 21); style(gNGb, kGreen+2, 21); style(gCdb, kMagenta+1, 20);
    TCanvas* cb = new TCanvas("cb", "Beam footprint", 1500, 450);
    cb->Divide(3, 1);
    cb->cd(1); gThb->SetTitle("Thermal (< 0.5 eV) in the beam footprint;Graphite channel length (cm);n per cm^{2} per primary");
    gThb->SetMinimum(0.); gThb->Draw("APL");
    cb->cd(2); gPad->SetLogy();
    gNGb->SetTitle("n/#gamma in the beam footprint;Graphite channel length (cm);n cm^{-2} Sv^{-1}");
    gNGb->SetMinimum(3e9); gNGb->SetMaximum(1e12); gNGb->Draw("APL");
    gPad->Update();
    TLine* l10 = new TLine(gPad->GetUxmin(), 1e10, gPad->GetUxmax(), 1e10); l10->SetLineStyle(2); l10->SetLineColor(kRed); l10->Draw();
    TLine* l11 = new TLine(gPad->GetUxmin(), 1e11, gPad->GetUxmax(), 1e11); l11->SetLineStyle(2); l11->SetLineColor(kMagenta+1); l11->Draw();
    cb->cd(3);
    gCdb->SetTitle("Cd ratio (1/v) in the beam footprint;Graphite channel length (cm);Cd ratio");
    gCdb->SetMinimum(0.); gCdb->Draw("APL");
    cb->SaveAs(Form("%s_footprint.png", prefix));
}
