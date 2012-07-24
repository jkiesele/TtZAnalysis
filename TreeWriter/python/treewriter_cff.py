import FWCore.ParameterSet.Config as cms

PFTree   = cms.EDAnalyzer('TreeWriter',
                          treeName = cms.string('pfTree'),
                          muonSrc = cms.InputTag('patMuonsWithTrigger'),
                          elecSrc = cms.InputTag('patElectronsWithTrigger'),
                          jetSrc = cms.InputTag('treeJets'),
                          btagAlgo = cms.string('combinedSecondaryVertexBJetTags'),
                          metSrc = cms.InputTag('patMETs'), #
                          vertexSrc = cms.InputTag('goodOfflinePrimaryVertices'),
                          
                          includeReco = cms.bool(False),
                          recoMuonSrc = cms.InputTag('muons'),
                          isPFMuonCand = cms.bool(False),
                          recoElecSrc  = cms.InputTag('gsfElectrons'),
                          isPFElecCand = cms.bool(False),
                          recoTrackSrc = cms.InputTag('generalTracks'),
                          recoSuCluSrc = cms.InputTag('superClusters'),
                          
                                  
                          includeTrigger = cms.bool(True),
                          triggerResults = cms.InputTag('TriggerResults','','HLT'),
                          
                          PUInfo = cms.InputTag('addPileupInfo'),
                          rhoIso = cms.InputTag("kt6PFJets","rho", "RECO"),
                          includeRho2011 = cms.bool(True),
                          rhoJetsIsoNoPu = cms.InputTag("kt6PFJets"), #
                          rhoJetsIso = cms.InputTag("kt6PFJets"), #
                          useGsfElecs = cms.bool(False),
                          
                          includePDFWeights = cms.bool(False),
                          pdfWeights = cms.InputTag('')
                          )