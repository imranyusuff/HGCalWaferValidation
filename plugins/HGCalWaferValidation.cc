// -*- C++ -*-
//
// Package:    UserCode/HGCalWaferValidation
// Class:      HGCalWaferValidation
//
/**\class HGCalWaferValidation HGCalWaferValidation.cc UserCode/HGCalWaferValidation/plugins/HGCalWaferValidation.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Imran Yusuff
//         Created:  Thu, 27 May 2021 19:47:08 GMT
//
//

// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DetectorDescription/Core/interface/DDCompactView.h"
#include "Geometry/Records/interface/IdealGeometryRecord.h"
#include "Geometry/HGCalCommonData/interface/HGCalTypes.h"

#include <fstream>
#include <regex>

//
// class declaration
//

// If the analyzer does not use TFileService, please remove
// the template argument to the base class so the class inherits
// from  edm::one::EDAnalyzer<>
// This will improve performance in multithreaded jobs.

class HGCalWaferValidation : public edm::one::EDAnalyzer<> {
public:
  explicit HGCalWaferValidation(const edm::ParameterSet&);
  ~HGCalWaferValidation();

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  // wafer coordinate is (layer, u, v)
  using WaferCoord = std::tuple<int, int, int>;
  std::string strWaferCoord(const WaferCoord& coord);

  bool DDFindHGCal(DDCompactView::GraphWalker & walker, std::string targetName);
  void DDFindWafers(DDCompactView::GraphWalker & walker);
  void ProcessWaferLayer(DDCompactView::GraphWalker & walker);

  void beginJob() override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  void endJob() override;

  // ----------member data ---------------------------
  // module parameters
  std::string geometryFileName_;

  struct WaferInfo {
    int thickClass;
    double x;
    double y;
    std::string shapeCode;
    int rotCode;
  };

  edm::ESGetToken<DDCompactView, IdealGeometryRecord> viewToken_;

  std::map<WaferCoord, struct WaferInfo> waferData;
  std::map<WaferCoord, bool> waferValidated;
};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
HGCalWaferValidation::HGCalWaferValidation(const edm::ParameterSet& iConfig)
 : geometryFileName_(iConfig.getUntrackedParameter<std::string>("GeometryFileName")) {
  viewToken_ = esConsumes<DDCompactView, IdealGeometryRecord>();
  //now do what ever initialization is needed
}

HGCalWaferValidation::~HGCalWaferValidation() {
  // do anything here that needs to be done at desctruction time
  // (e.g. close files, deallocate resources etc.)
  //
  // please remove this method altogether if it would be left empty
}

//
// member functions
//

// convert WaferCoord tuple to string representation
std::string HGCalWaferValidation::strWaferCoord(const WaferCoord& coord) {
  std::stringstream ss;
  ss << "(" << std::get<0>(coord) << "," << std::get<1>(coord) << "," << std::get<2>(coord) << ")";
  return ss.str();
}

// ----- find HGCal entry among the DD -----
bool HGCalWaferValidation::DDFindHGCal(DDCompactView::GraphWalker & walker, std::string targetName) {
  if (walker.current().first.name().name() == targetName) {
    return true;
  }
  if (walker.firstChild()) {
    do {
      if (DDFindHGCal(walker, targetName))
        return true;
    } while (walker.nextSibling());
    walker.parent();
  }
  return false;
}

// ----- find the next wafer -----
void HGCalWaferValidation::DDFindWafers(DDCompactView::GraphWalker & walker) {
  if (walker.current().first.name().fullname().rfind("hgcalwafer:", 0) == 0) {
    ProcessWaferLayer(walker);
    return;
  }
  if (walker.firstChild()) {
    do {
      DDFindWafers(walker);
    } while (walker.nextSibling());
    walker.parent();
  }
}

// ----- process the layer -----
void HGCalWaferValidation::ProcessWaferLayer(DDCompactView::GraphWalker & walker) {
  static int waferLayer = 0;
  waferLayer++;
  std::cout << "ProcessWaferLayer: Processing layer " << waferLayer << std::endl;
  do {
    if (walker.current().first.name().fullname().rfind("hgcalwafer:", 0) == 0) {
      auto wafer = walker.current();
      const std::string waferName(walker.current().first.name().fullname());
      const int copyNo = wafer.second->copyno();
      const int waferType = HGCalTypes::getUnpackedType(copyNo);
      const int waferU = HGCalTypes::getUnpackedU(copyNo);
      const int waferV = HGCalTypes::getUnpackedV(copyNo);
      const WaferCoord waferCoord(waferLayer, waferU, waferV);
      struct WaferInfo waferInfo;
      waferInfo.thickClass = waferType;
      waferInfo.x = wafer.second->translation().x();
      waferInfo.y = wafer.second->translation().y();
      const std::string waferNameData =
        std::regex_replace(waferName,
                           std::regex("(HGCal[EH]E)(Wafer[01])(Fine|Coarse[12])([a-z]*)([0-9]*)"),
                           "$1 $2-$3 $4 $5",
                           std::regex_constants::format_no_copy);
      std::stringstream ss(waferNameData);
      std::string EEorHE;
      std::string typeStr;
      std::string shapeStr;
      std::string rotStr;
      ss >> EEorHE >> typeStr >> shapeStr >> rotStr;
      if (shapeStr == "")
        shapeStr = "F";
      if (rotStr == "")
        rotStr = "0";
      const int rotCode(std::stoi(rotStr));
      //std::cout << "rotStr " << rotStr << " rotCode " << rotCode << std::endl;
      waferInfo.shapeCode = shapeStr;
      waferInfo.rotCode = rotCode;
      waferData[waferCoord] = waferInfo;
      waferValidated[waferCoord] = false;
    }
  } while (walker.nextSibling());
}

// ------------ method called for each event  ------------
void HGCalWaferValidation::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  using namespace edm;

  auto viewH = iSetup.getHandle(viewToken_);

  if (!viewH.isValid()) {
    std::cout << "Error obtaining geometry handle!" << std::endl;
    return;
  }

  std::cout << "Root is : " << viewH->root() << std::endl;
  std::cout << std::endl;

  auto eeWalker = viewH->walker();
  const bool eeFound = DDFindHGCal(eeWalker, "HGCalEE");
  if (eeFound) {
    std::cout << "HGCalEE found!" << std::endl;
    std::cout << "name     = " << eeWalker.current().first.name().name() << std::endl;
    std::cout << "fullname = " << eeWalker.current().first.name().fullname() << std::endl;
  }
  else {
    std::cout << "HGCalEE not found!" << std::endl;
  }
  std::cout << std::endl;

  auto hesilWalker = viewH->walker();
  const bool hesilFound = DDFindHGCal(hesilWalker, "HGCalHEsil");
  if (hesilFound) {
    std::cout << "HGCalHEsil found!" << std::endl;
    std::cout << "name     = " << hesilWalker.current().first.name().name() << std::endl;
    std::cout << "fullname = " << hesilWalker.current().first.name().fullname() << std::endl;
  }
  else {
    std::cout << "HGCalHEsil not found!" << std::endl;
  }
  std::cout << std::endl;

  auto hemixWalker = viewH->walker();
  const bool hemixFound = DDFindHGCal(hemixWalker, "HGCalHEmix");
  if (hemixFound) {
    std::cout << "HGCalHEmix found!" << std::endl;
    std::cout << "name     = " << hemixWalker.current().first.name().name() << std::endl;
    std::cout << "fullname = " << hemixWalker.current().first.name().fullname() << std::endl;
  }
  else {
    std::cout << "HGCalHEmix not found!" << std::endl;
  }
  std::cout << std::endl;

  if (!(eeFound || hesilFound || hemixFound)) {
    std::cout << "Nothing found. Giving up." << std::endl;
    return;
  }

  // Now walk the HGCalEE walker to find the first wafer
  std::cout << "Calling DDFindWafers(eeWalker);" << std::endl;
  DDFindWafers(eeWalker);

  // Walk the HGCalHEsilwalker to find the first wafer
  std::cout << "Calling DDFindWafers(hesilWalker);" << std::endl;
  DDFindWafers(hesilWalker);

  // Walk the HGCalHEmix walker to find the first wafer
  std::cout << "Calling DDFindWafers(hemixWalker);" << std::endl;
  DDFindWafers(hemixWalker);

  // Confirm all the DD wafers have been read
  std::cout << "Number of wafers read from DD: " << waferData.size() << std::endl;

  // Now open the geometry text file
  std::cout << "Opening geometry text file: " << geometryFileName_ << std::endl;

  std::ifstream geoTxtFile(geometryFileName_);

  if (!geoTxtFile) {
    std::cout << "Cannot open geometry text file." << std::endl;
    return;
  }

  // total processed counter
  int nTotalProcessed = 0;

  // geometry error counters
  int nMissing = 0;
  int nThicknessError = 0;
  int nPosXError = 0;
  int nPosYError = 0;
  int nShapeError = 0;
  int nRotError = 0;
  int nUnaccounted = 0;

  std::string buf;

  while (std::getline(geoTxtFile, buf)) {
    std::stringstream ss(buf);
    std::vector<std::string> tokens;
    while (ss >> buf)
      if (buf.size() > 0)
        tokens.push_back(buf);
    if (tokens.size() != 8)
      continue;

    nTotalProcessed++;

    const int waferLayer(std::stoi(tokens[0]));
    const std::string waferShapeCode(tokens[1]);
    const int waferThickness(std::stoi(tokens[2]));
    const double waferX(std::stod(tokens[3]));
    const double waferY(std::stod(tokens[4]));
    const int waferRotCode(std::stoi(tokens[5]));
    const int waferU(std::stoi(tokens[6]));
    const int waferV(std::stoi(tokens[7]));

    const WaferCoord waferCoord(waferLayer, waferU, waferV);

    if (waferData.find(waferCoord) == waferData.end()) {
      nMissing++;
      std::cout << "MISSING: " << strWaferCoord(waferCoord) << std::endl;
      continue;
    }

    const struct WaferInfo waferInfo = waferData[waferCoord];
    waferValidated[waferCoord] = true;

    if ((waferInfo.thickClass == 0 && waferThickness != 120) ||
        (waferInfo.thickClass == 1 && waferThickness != 200) ||
        (waferInfo.thickClass == 2 && waferThickness != 300)) {
      nThicknessError++;
      std::cout << "THICKNESS ERROR: " << strWaferCoord(waferCoord) << std::endl;
    }

    if (fabs(-waferInfo.x - waferX) > 0.015) {
      nPosXError++;
      std::cout << "POSITION x ERROR: " << strWaferCoord(waferCoord) << std::endl;
    }

    if (fabs(waferInfo.y - waferY) > 0.015) {
      nPosYError++;
      std::cout << "POSITION y ERROR: " << strWaferCoord(waferCoord) << std::endl;
    }

    if (waferInfo.shapeCode != waferShapeCode) {
      nShapeError++;
      std::cout << "SHAPE ERROR: " << strWaferCoord(waferCoord) << std::endl;
    }

    if (waferInfo.rotCode != waferRotCode) {
      nRotError++;
      std::cout << "ROTATION ERROR: " << strWaferCoord(waferCoord) << "  ( " << waferInfo.rotCode << " != " << waferRotCode << " )" << std::endl;
    }
  }

  geoTxtFile.close();

  // Find unaccounted wafers
  for ( auto const& accounted : waferValidated ) {
    if (!accounted.second) {
      nUnaccounted++;
      std::cout << "UNACCOUNTED: " << strWaferCoord(accounted.first) << std::endl;
    }
  }

  // Print out error counts
  std::cout << std::endl;
  std::cout << "*** ERROR COUNTS ***" << std::endl;
  std::cout << "Missing         :  " << nMissing << std::endl;
  std::cout << "Thickness error :  " << nThicknessError << std::endl;
  std::cout << "Pos-x error     :  " << nPosXError << std::endl;
  std::cout << "Pos-y error     :  " << nPosYError << std::endl;
  std::cout << "Shape error     :  " << nShapeError << std::endl;
  std::cout << "Rotation error  :  " << nRotError << std::endl;
  std::cout << "Unaccounted     :  " << nUnaccounted << std::endl;
  std::cout << std::endl;
  std::cout << "Total wafers processed from geotxtfile = " << nTotalProcessed << std::endl;
  std::cout << std::endl;
}

// ------------ method called once each job just before starting event loop  ------------
void HGCalWaferValidation::beginJob() {
  // please remove this method if not needed
}

// ------------ method called once each job just after ending the event loop  ------------
void HGCalWaferValidation::endJob() {
  // please remove this method if not needed
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void HGCalWaferValidation::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);

  //Specify that only 'tracks' is allowed
  //To use, remove the default given above and uncomment below
  //ParameterSetDescription desc;
  //desc.addUntracked<edm::InputTag>("tracks","ctfWithMaterialTracks");
  //descriptions.addWithDefaultLabel(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(HGCalWaferValidation);
