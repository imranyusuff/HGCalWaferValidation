import FWCore.ParameterSet.Config as cms

process = cms.Process("TEST")

process.load("Geometry.CMSCommonData.cmsExtendedGeometry2026D83XML_cfi")

process.source = cms.Source("EmptySource")

process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(1))

process.test = cms.EDAnalyzer("HGCalWaferValidation",
  GeometryFileName = cms.untracked.string("/afs/cern.ch/user/i/iyusuff/hgcalgeo/geomnew_corrected_360.txt")
)

process.p = cms.Path(process.test)
