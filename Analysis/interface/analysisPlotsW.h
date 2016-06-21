/*
 * analysisPlotsW.h
 *
 *  Created on: 30 May 2016
 *      Author: jkiesele
 */

#ifndef TTZANALYSIS_ANALYSIS_INTERFACE_ANALYSISPLOTSW_H_
#define TTZANALYSIS_ANALYSIS_INTERFACE_ANALYSISPLOTSW_H_

#include "analysisPlots.h"

namespace ztop{
class analysisPlotsW: public analysisPlots{
public:
	analysisPlotsW(size_t step):analysisPlots(step),
	isiso_(false),
	asymmisofull_(0),asymmnonisofull_(0),
	mTnonisofull_(0)
	{
		etaRanges_ << 0 << 0.4 << 0.8 << 1.2 << 1.6 << 2. << 2.4 << 2.8 << 3.2 << 3.6; //just for testing use only one

	}


	void bookPlots();
	void fillPlotsReco();
	void fillPlotsGen();

	void setIsIso(const bool & is){isiso_=is;}


	template <class T, class U>
	static float asymmValue(T,U,bool invert=true);

private:
	bool isiso_;

	std::vector< histo1D* >
	asymmiso_;
	histo1D* asymmisofull_;
	std::vector< histo1D* >
	asymmnoniso_;
	histo1D* asymmnonisofull_;
	std::vector< histo1D* >
	mTnoniso_;
	histo1D* mTnonisofull_;

	static double WMASS;

	float calcAsymmReco(float& absdeta)const;
	float calcAsymmGen(float& absdeta)const;

	float calcMTReco(float& absdeta)const;


	size_t getEtaIndex(const float & deta)const;
	std::vector<float> etaRanges_;

	TString produceDEtaString(size_t i)const;

	bool checkEventContentReco()const;

};


template <class T, class U>
float analysisPlotsW::asymmValue(T muon,U jet, bool invert){

	double deltaphi=0;
	if(invert){
		float jetphi=jet->phi();
		jetphi+= M_PI;
		while(jetphi >= M_PI) jetphi-=2*M_PI;
		while(jetphi < -M_PI) jetphi+=2*M_PI;
		deltaphi=deltaPhi(muon->phi(), jetphi);
	}
	else{
		deltaphi=deltaPhi(muon->phi(),jet->phi() );
	}
	double sindphi=std::sin(deltaphi);

	double signeta=1;
	if(muon->eta()<0)
		signeta=-1;
	float chargefloat=muon->q();
	if(!muon->q())
		throw std::logic_error("analysisPlotsW::asymmValue: muon with 0 charge!");
	double asymm= muon->pt()*2 / WMASS * sindphi * chargefloat * signeta ;
	return asymm;
}


}

/*
 *
 *
 *
 *
 */


#endif /* TTZANALYSIS_ANALYSIS_INTERFACE_ANALYSISPLOTSW_H_ */
