/*
 * wAnalyzer.h
 *
 *  Created on: 23 May 2016
 *      Author: kiesej
 */

#ifndef TTZANALYSIS_ANALYSIS_INTERFACE_WANALYZER_H_
#define TTZANALYSIS_ANALYSIS_INTERFACE_WANALYZER_H_

#include "basicAnalyzer.h"
#include "TString.h"

namespace ztop{

class tTreeHandler;

class wAnalyzer : public basicAnalyzer{
public:
	wAnalyzer():basicAnalyzer(){}
	~wAnalyzer(){}

	fileForker::fileforker_status  start();

	void addWeightIndex(size_t idx){weightindicies_.push_back(idx);}

private:

	fileForker::fileforker_status writeOutput();

	void analyze(size_t id);

	void createNormalizationInfo(tTreeHandler* );

	std::vector<size_t> weightindicies_;
};

}


#endif /* TTZANALYSIS_ANALYSIS_INTERFACE_WANALYZER_H_ */