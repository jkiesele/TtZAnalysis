import FWCore.ParameterSet.Config as cms

PUInfo = cms.EDAnalyzer('PUInfo',
                        treeName = cms.string('PUInfo'),
                        vertexSrc=cms.InputTag('goodOfflinePrimaryVertices'),
                        PUInfo = cms.InputTag('addPileupInfo'),
                        includePDFWeights = cms.bool(False),
                        pdfWeights = cms.InputTag('')
                        )
