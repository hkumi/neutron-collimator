// Lead brick sweep: thermal neutron fluence, gamma dose, neutron-to-gamma ratio, cadmium ratio.
// Run from the build folder:   root -l './leadAnalysis.C(10)'     (10 = moderator thickness in cm)
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

void leadAnalysis(int moderator = 10)
{
    const std::vector<int> lead = {0, 2, 4, 6, 8, 10};   // cm, must match leadSweep.sh
    const double areaExit = 100.;                        // cm2
    const double rSpot    = 30.;                         // mm
    const double areaSpot = M_PI * 3.0 * 3.0;            // cm2

    auto gPhiX  = new TGraphErrors(); auto gDoseX = new TGraph(); auto gNGX = new TGraph();
    auto gNGS   = new TGraph();

    printf("\nModerator %d cm. Per primary. n/g = thermal (<0.5 eV) fluence / gamma dose.\n", moderator);
    printf("\n EXIT (10 x 10 cm before the pinhole layer):\n");
    printf("  Lead   thermal<0.025eV  thermal<0.5eV     fast>50keV    gamma dose     n/g ratio      n/g (<0.025)   Cd ratio  Cd ratio\n");
    printf("         (n/cm2)          (n/cm2)           (n/cm2)       (Sv)           (n cm-2 Sv-1)  (n cm-2 Sv-1)  (counts)  (1/v)\n");

    std::vector<Result> spots; std::vector<int> spotLead;
    int ip = 0;
    for (int t : lead) {
        TFile* f = TFile::Open(Form("lead_%dcm_mod%d.root", t, moderator));
        if (!f || f->IsZombie()) { printf("  %2d cm  (file missing - skipped)\n", t); continue; }
        TH1D*  hPrim = (TH1D*)  f->Get("nPrimaries");
        TTree* tx    = (TTree*) f->Get("ModExit");
        TTree* ts    = (TTree*) f->Get("Screen");
        if (!hPrim || !tx || !ts) { printf("  %2d cm  (missing objects - skipped)\n", t); f->Close(); continue; }
        const double nPrim = hPrim->GetEntries();

        Result x = analyse(tx, areaExit, nPrim, -1.);
        Result s = analyse(ts, areaSpot, nPrim, rSpot);
        spots.push_back(s); spotLead.push_back(t);

        const double ng    = x.doseSv > 0 ? x.phiTh05 / x.doseSv : 0.;
        const double ng025 = x.doseSv > 0 ? x.phiTh025 / x.doseSv : 0.;
        printf("  %2d cm  %.2e         %.2e (%4.0f)  %.2e      %.2e      %.2e       %.2e       %5.2f     %5.1f\n",
               t, x.phiTh025, x.phiTh05, x.nTh05, x.phiFast, x.doseSv, ng, ng025, x.cdCounts(), x.cdOneOverV());

        gPhiX->SetPoint(ip, t, x.phiTh05);
        gPhiX->SetPointError(ip, 0., x.nTh05 > 0 ? x.phiTh05/std::sqrt(x.nTh05) : 0.);
        gDoseX->SetPoint(ip, t, x.doseSv);
        gNGX->SetPoint(ip, t, ng);
        if (s.doseSv > 0 && s.phiTh05 > 0) gNGS->SetPoint(gNGS->GetN(), t, s.phiTh05 / s.doseSv);
        ++ip;
        f->Close();
    }
    if (ip == 0) { printf("No files found.\n"); return; }

    printf("\n SPOT (screen, r < 30 mm) - low statistics, for reference:\n");
    printf("  Lead   thermal<0.5eV     gamma dose    n/g ratio      Cd(1/v)    (raw thermal counts, gamma counts)\n");
    for (size_t i = 0; i < spots.size(); ++i) {
        const Result& s = spots[i];
        printf("  %2d cm  %.2e          %.2e      %.2e       %5.2f      (%.0f, %.0f)\n",
               spotLead[i], s.phiTh05, s.doseSv, s.doseSv > 0 ? s.phiTh05/s.doseSv : 0.,
               s.cdOneOverV(), s.nTh05, s.nGam);
    }
    printf("\n Targets: n/g > 1e10 n cm-2 Sv-1 (Li et al. 2020); n/g >= 1e6 n cm-2 mR-1 ~ 1e11 n cm-2 Sv-1 (MacGillivray).\n");
    printf("          Cd ratio (Mn foil) > 150 (Yang 2014) -> compare with the 1/v column.\n");

    // ---- plots ----
    gStyle->SetOptStat(0);
    auto style = [](TGraph* g, int col, int mk) {
        g->SetLineColor(col); g->SetMarkerColor(col); g->SetMarkerStyle(mk); g->SetLineWidth(2);
    };
    style(gPhiX, kBlue+1, 20); style(gDoseX, kOrange+7, 22); style(gNGX, kGreen+2, 21); style(gNGS, kGray+2, 24);

    TCanvas* c = new TCanvas("c", "Lead sweep", 1500, 450);
    c->Divide(3, 1);

    c->cd(1);
    gPhiX->SetTitle(Form("Thermal (<0.5 eV) fluence at exit, moderator %d cm;Lead thickness (cm);n per cm^{2} per primary", moderator));
    gPhiX->Draw("APL"); gPhiX->SetMinimum(0.);

    c->cd(2); gPad->SetLogy();
    gDoseX->SetTitle("Gamma dose at exit;Lead thickness (cm);Sv per primary");
    gDoseX->Draw("APL");

    c->cd(3); gPad->SetLogy();
    gNGX->SetTitle("Neutron-to-gamma ratio;Lead thickness (cm);n cm^{-2} Sv^{-1}");
    gNGX->SetMinimum(3e9); gNGX->SetMaximum(1e13);
    gNGX->Draw("APL");
    if (gNGS->GetN() > 0) gNGS->Draw("PL same");
    gPad->Update();
    TLine* l10 = new TLine(gPad->GetUxmin(), 1e10, gPad->GetUxmax(), 1e10);
    l10->SetLineStyle(2); l10->SetLineColor(kRed); l10->Draw();
    TLine* l11 = new TLine(gPad->GetUxmin(), 1e11, gPad->GetUxmax(), 1e11);
    l11->SetLineStyle(2); l11->SetLineColor(kMagenta+1); l11->Draw();
    auto leg = new TLegend(0.15, 0.70, 0.65, 0.89);
    leg->AddEntry(gNGX, "at exit (10 x 10 cm)", "pl");
    if (gNGS->GetN() > 0) leg->AddEntry(gNGS, "screen spot (low stats)", "pl");
    leg->AddEntry(l10, "10^{10} (Li et al. 2020)", "l");
    leg->AddEntry(l11, "~10^{11} (MacGillivray)", "l");
    leg->Draw();

    c->SaveAs(Form("lead_sweep_mod%d.png", moderator));
}
