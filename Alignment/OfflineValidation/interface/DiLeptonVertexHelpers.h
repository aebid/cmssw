#ifndef Alignment_OfflineValidation_DiLeptonVertexHelpers_h
#define Alignment_OfflineValidation_DiLeptonVertexHelpers_h

#include <vector>
#include <string>
#include <fmt/printf.h>
#include "TH2F.h"
#include "TLorentzVector.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

namespace DiLeptonHelp {

  //
  // Ancillary struct for counting
  //
  struct Counts {
    unsigned int eventsTotal;
    unsigned int eventsAfterMult;
    unsigned int eventsAfterPt;
    unsigned int eventsAfterEta;
    unsigned int eventsAfterVtx;
    unsigned int eventsAfterDist;

  public:
    void printCounts() {
      edm::LogInfo("DiLeptonHelpCounts") << " Total Events: " << eventsTotal << "\n"
                                         << " After multiplicity: " << eventsAfterMult << "\n"
                                         << " After pT cut: " << eventsAfterPt << "\n"
                                         << " After eta cut: " << eventsAfterEta << "\n"
                                         << " After Vtx: " << eventsAfterVtx << "\n"
                                         << " After VtxDist: " << eventsAfterDist << std::endl;
    }

    void zeroAll() {
      eventsTotal = 0;
      eventsAfterMult = 0;
      eventsAfterPt = 0;
      eventsAfterEta = 0;
      eventsAfterVtx = 0;
      eventsAfterDist = 0;
    }
  };

  enum flavour { MM = 0, EE = 1, UNDEF = -1 };

  enum etaRegion { BARBAR, BARFWD, BARBWD, FWDFWD, BWDBWD, FWDBWD, END };

  //
  // Ancillary class for plotting in different kinematics regions
  // of the two muon tracks
  //
  class PlotsVsDiLeptonRegion {
  public:
    PlotsVsDiLeptonRegion(const float etaBoundary) : m_etaBoundary(etaBoundary) {}
    ~PlotsVsDiLeptonRegion() = default;

    //________________________________________________________________________________//
    inline void bookSet(const TFileDirectory& fs, const TH1* histo) {
      const std::string name = histo->GetName();
      const std::string title = histo->GetTitle();
      const std::string xTitle = histo->GetXaxis()->GetTitle();
      const std::string yTitle = histo->GetYaxis()->GetTitle();
      std::string zTitle = "";
      if (((TObject*)histo)->InheritsFrom("TH2")) {
        zTitle = histo->GetZaxis()->GetTitle();
      }

      for (const auto& etaReg : m_etaRegions) {
        if (etaReg == etaRegion::END)
          continue;

        const auto& toSub = m_etaRegionNames[etaReg];

        if (((TObject*)histo)->InheritsFrom("TH2")) {
          m_h2_map[etaReg] = fs.make<TH2F>(
              (name + "_" + toSub).c_str(),
              (title + " (" + toSub + ");" + xTitle + " (" + toSub + ") ;" + yTitle + " (" + toSub + ");" + zTitle)
                  .c_str(),
              histo->GetNbinsX(),
              histo->GetXaxis()->GetXmin(),
              histo->GetXaxis()->GetXmax(),
              histo->GetNbinsY(),
              histo->GetYaxis()->GetXmin(),
              histo->GetYaxis()->GetXmax());
        } else {
          m_h1_map[etaReg] = fs.make<TH1F>(
              (name + "_" + toSub).c_str(),
              (title + " (" + toSub + ");" + xTitle + " (" + toSub + ") ;" + yTitle + " (" + toSub + ")").c_str(),
              histo->GetNbinsX(),
              histo->GetXaxis()->GetXmin(),
              histo->GetXaxis()->GetXmax());
        }
      }

      // flip the is booked bit
      m_isBooked = true;
    }

    //________________________________________________________________________________//
    // Determine the eta region based on eta values
    etaRegion getEtaRegion(const double eta1, const double eta2) {
      bool isEta1Barrel = std::abs(eta1) <= m_etaBoundary;
      bool isEta2Barrel = std::abs(eta2) <= m_etaBoundary;

      if (isEta1Barrel && isEta2Barrel) {
        return etaRegion::BARBAR;
      } else if ((isEta1Barrel && eta2 > m_etaBoundary) || (isEta2Barrel && eta1 > m_etaBoundary)) {
        return etaRegion::BARFWD;
      } else if ((isEta1Barrel && eta2 < -m_etaBoundary) || (isEta2Barrel && eta1 < -m_etaBoundary)) {
        return etaRegion::BARBWD;
      } else if (eta1 > m_etaBoundary && eta2 > m_etaBoundary) {
        return etaRegion::FWDFWD;
      } else if (eta1 < -m_etaBoundary && eta2 < -m_etaBoundary) {
        return etaRegion::BWDBWD;
      } else if ((eta1 > m_etaBoundary && eta2 < -m_etaBoundary) || (eta2 > m_etaBoundary && eta1 < -m_etaBoundary)) {
        return etaRegion::FWDBWD;
      }

      // Default case if none of the conditions match
      return etaRegion::END;  // Adjust the default based on your logic
    }

    //________________________________________________________________________________//
    inline void fillTH1Plots(const float val, const std::pair<TLorentzVector, TLorentzVector>& momenta) {
      if (!m_isBooked) {
        edm::LogError("PlotsVsDiLeptonRegion")
            << "In" << __FUNCTION__ << "," << __LINE__ << "trying to fill a plot not booked!" << std::endl;
        return;
      }

      etaRegion region = getEtaRegion(momenta.first.Eta(), momenta.second.Eta());
      if (region == etaRegion::END) {
        edm::LogError("PlotsVsDiLeptonRegion") << "undefined di-muon kinematics" << std::endl;
      }
      m_h1_map[region]->Fill(val);
    }

    //________________________________________________________________________________//
    inline void fillTH2Plots(const float valX,
                             const float valY,
                             const std::pair<TLorentzVector, TLorentzVector>& momenta) {
      if (!m_isBooked) {
        edm::LogError("PlotsVsDiLeptonRegion")
            << "In" << __FUNCTION__ << "," << __LINE__ << "trying to fill a plot not booked!" << std::endl;
        return;
      }

      etaRegion region = getEtaRegion(momenta.first.Eta(), momenta.second.Eta());
      if (region == etaRegion::END) {
        edm::LogError("PlotsVsDiLeptonRegion") << "undefined di-muon kinematics" << std::endl;
      }
      m_h2_map[region]->Fill(valX, valY);
    }

  private:
    const std::vector<etaRegion> m_etaRegions = {etaRegion::BARBAR,
                                                 etaRegion::BARFWD,
                                                 etaRegion::BARBWD,
                                                 etaRegion::FWDFWD,
                                                 etaRegion::BWDBWD,
                                                 etaRegion::FWDBWD};

    const std::vector<std::string> m_etaRegionNames = {"barrel-barrel",
                                                       "barrel-forward",
                                                       "barrel-backward",
                                                       "forward-forward",
                                                       "backward-backward",
                                                       "forward-backward"};
    const float m_etaBoundary;
    bool m_isBooked;
    std::map<etaRegion, TH1F*> m_h1_map;
    std::map<etaRegion, TH2F*> m_h2_map;
  };

  //
  // Ancillary class for plotting
  //
  class PlotsVsKinematics {
  public:
    PlotsVsKinematics(flavour FLAV) : m_name(""), m_title(""), m_ytitle(""), m_isBooked(false), m_flav(FLAV) {}

    //________________________________________________________________________________//
    // overloaded constructor
    PlotsVsKinematics(flavour FLAV, const std::string& name, const std::string& tt, const std::string& ytt)
        : m_name(name), m_title(tt), m_ytitle(ytt), m_isBooked(false), m_flav(FLAV) {
      if (m_flav < 0) {
        edm::LogError("PlotsVsKinematics") << "The initialization flavour is not correct!" << std::endl;
      }
    }

    ~PlotsVsKinematics() = default;

    //________________________________________________________________________________//
    inline void bookFromPSet(const TFileDirectory& fs, const edm::ParameterSet& hpar) {
      std::string namePostfix;
      std::string titlePostfix;
      float xmin, xmax;

      std::string sed = (m_flav ? "e" : "#mu");

      for (const auto& xAx : axisChoices) {
        switch (xAx) {
          case xAxis::Z_PHI:
            xmin = -M_PI;
            xmax = M_PI;
            namePostfix = m_flav ? "EEPhi" : "MMPhi";
            titlePostfix = fmt::sprintf("%s%s pair #phi;%s^{+}%s^{-} #phi", sed, sed, sed, sed);
            break;
          case xAxis::Z_ETA:
            xmin = -3.5;
            xmax = 3.5;
            namePostfix = m_flav ? "EEEta" : "MuMuEta";
            titlePostfix = fmt::sprintf("%s%s pair #eta;%s^{+}%s^{-} #eta", sed, sed, sed, sed);
            break;
          case xAxis::LP_PHI:
            xmin = -M_PI;
            xmax = M_PI;
            namePostfix = m_flav ? "EPlusPhi" : "MuPlusPhi";
            titlePostfix = fmt::sprintf("%s^{+} #phi;%s^{+} #phi [rad]", sed, sed);
            break;
          case xAxis::LP_ETA:
            xmin = -2.4;
            xmax = 2.4;
            namePostfix = m_flav ? "EPlusEta" : "MuPlusEta";
            titlePostfix = fmt::sprintf("%s^{+} #eta;%s^{+} #eta", sed, sed);
            break;
          case xAxis::LM_PHI:
            xmin = -M_PI;
            xmax = M_PI;
            namePostfix = m_flav ? "EMinusPhi" : "MuMinusPhi";
            titlePostfix = fmt::sprintf("%s^{-} #phi;%s^{-} #phi [rad]", sed, sed);
            break;
          case xAxis::LM_ETA:
            xmin = -2.4;
            xmax = 2.4;
            namePostfix = m_flav ? "EMinusEta" : "MuMinusEta";
            titlePostfix = fmt::sprintf("%s^{-} #eta;%s^{+} #eta", sed, sed);
            break;
          default:
            throw cms::Exception("LogicalError") << " there is not such Axis choice as " << xAx;
        }

        const auto& h2name = fmt::sprintf("%sVs%s", hpar.getParameter<std::string>("name"), namePostfix);
        const auto& h2title = fmt::sprintf("%s vs %s;%s% s",
                                           hpar.getParameter<std::string>("title"),
                                           titlePostfix,
                                           hpar.getParameter<std::string>("title"),
                                           hpar.getParameter<std::string>("yUnits"));

        m_h2_map[xAx] = fs.make<TH2F>(h2name.c_str(),
                                      h2title.c_str(),
                                      hpar.getParameter<int32_t>("NxBins"),
                                      xmin,
                                      xmax,
                                      hpar.getParameter<int32_t>("NyBins"),
                                      hpar.getParameter<double>("ymin"),
                                      hpar.getParameter<double>("ymax"));
      }

      // flip the is booked bit
      m_isBooked = true;
    }

    //________________________________________________________________________________//
    inline void bookPlots(
        TFileDirectory& fs, const float valmin, const float valmax, const int nxbins, const int nybins) {
      if (m_name.empty() && m_title.empty() && m_ytitle.empty()) {
        edm::LogError("PlotsVsKinematics")
            << "In" << __FUNCTION__ << "," << __LINE__
            << "trying to book plots without the right constructor being called!" << std::endl;
        return;
      }

      std::string dilep = (m_flav ? "e^{+}e^{-}" : "#mu^{+}#mu^{-}");
      std::string lep = (m_flav ? "e^{+}" : "#mu^{+}");
      std::string lem = (m_flav ? "e^{-}" : "#mu^{-}");

      static constexpr float maxMuEta = 2.4;
      static constexpr float maxMuMuEta = 3.5;
      TH1F::SetDefaultSumw2(kTRUE);

      // clang-format off
      m_h2_map[xAxis::Z_ETA] = fs.make<TH2F>(fmt::sprintf("%sVsMuMuEta", m_name).c_str(),
					     fmt::sprintf("%s vs %s pair #eta;%s #eta;%s", m_title, dilep, dilep, m_ytitle).c_str(),
					     nxbins, -M_PI, M_PI,
					     nybins, valmin, valmax);
      
      m_h2_map[xAxis::Z_PHI] = fs.make<TH2F>(fmt::sprintf("%sVsMuMuPhi", m_name).c_str(),
					     fmt::sprintf("%s vs %s pair #phi;%s #phi [rad];%s", m_title, dilep, dilep, m_ytitle).c_str(),
					     nxbins, -maxMuMuEta, maxMuMuEta,
					     nybins, valmin, valmax);
      
      m_h2_map[xAxis::LP_ETA] = fs.make<TH2F>(fmt::sprintf("%sVsMuPlusEta", m_name).c_str(),
					      fmt::sprintf("%s vs %s #eta;%s #eta;%s", m_title, lep, lep, m_ytitle).c_str(),
					      nxbins, -maxMuEta, maxMuEta,
					      nybins, valmin, valmax);
      
      m_h2_map[xAxis::LP_PHI] = fs.make<TH2F>(fmt::sprintf("%sVsMuPlusPhi", m_name).c_str(),
					      fmt::sprintf("%s vs %s #phi;%s #phi [rad];%s", m_title, lep, lep, m_ytitle).c_str(),
					      nxbins, -M_PI, M_PI,
					      nybins, valmin, valmax);
      
      m_h2_map[xAxis::LM_ETA] = fs.make<TH2F>(fmt::sprintf("%sVsMuMinusEta", m_name).c_str(),
					      fmt::sprintf("%s vs %s #eta;%s #eta;%s", m_title, lem, lem, m_ytitle).c_str(),
					      nxbins, -maxMuEta, maxMuEta,
					      nybins, valmin, valmax);
      
      m_h2_map[xAxis::LM_PHI] = fs.make<TH2F>(fmt::sprintf("%sVsMuMinusPhi", m_name).c_str(),
					      fmt::sprintf("%s vs %s #phi;%s #phi [rad];%s", m_title, lem, lem,  m_ytitle).c_str(),
					      nxbins, -M_PI, M_PI,
					      nybins, valmin, valmax);
      // clang-format on

      // flip the is booked bit
      m_isBooked = true;
    }

    //________________________________________________________________________________//
    inline void fillPlots(const float val, const std::pair<TLorentzVector, TLorentzVector>& momenta) {
      if (!m_isBooked) {
        edm::LogError("PlotsVsKinematics")
            << "In" << __FUNCTION__ << "," << __LINE__ << "trying to fill a plot not booked!" << std::endl;
        return;
      }

      m_h2_map[xAxis::Z_ETA]->Fill((momenta.first + momenta.second).Eta(), val);
      m_h2_map[xAxis::Z_PHI]->Fill((momenta.first + momenta.second).Phi(), val);
      m_h2_map[xAxis::LP_ETA]->Fill((momenta.first).Eta(), val);
      m_h2_map[xAxis::LP_PHI]->Fill((momenta.first).Phi(), val);
      m_h2_map[xAxis::LM_ETA]->Fill((momenta.second).Eta(), val);
      m_h2_map[xAxis::LM_PHI]->Fill((momenta.second).Phi(), val);
    }

  private:
    enum xAxis { Z_PHI, Z_ETA, LP_PHI, LP_ETA, LM_PHI, LM_ETA };
    const std::vector<xAxis> axisChoices = {
        xAxis::Z_PHI, xAxis::Z_ETA, xAxis::LP_PHI, xAxis::LP_ETA, xAxis::LM_PHI, xAxis::LM_ETA};

    const std::string m_name;
    const std::string m_title;
    const std::string m_ytitle;

    bool m_isBooked;
    flavour m_flav;

    std::map<xAxis, TH2F*> m_h2_map;
  };
}  // namespace DiLeptonHelp
#endif
