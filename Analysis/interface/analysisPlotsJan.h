/*
 * analysisPlotsJan.h
 *
 *  Created on: Feb 12, 2014
 *      Author: kiesej
 */

#ifndef ANALYSISPLOTSJAN_H_
#define ANALYSISPLOTSJAN_H_

#include "analysisPlots.h"

namespace ztop{

class analysisPlotsJan : public analysisPlots{
public:
	analysisPlotsJan(size_t step):analysisPlots(step),
	Mlb(0),
	mlb(0),
	mlbmin(0),
	mlbivansbins(0),
	mlbminbsrad(0),
	leadleppt(0),
	mll(0),
	total(0),
	vistotal(0),
	mlbcombthresh_(165)
{}

	~analysisPlotsJan(){/* destruction is handled in base class! */}

	void bookPlots();
	void fillPlotsReco();
	void fillPlotsGen();

private:

	container1DUnfold
	*Mlb,
	*mlb,
	*mlbmin,
	*mlbivansbins,
	*mlbminbsrad,

	*leadleppt,
	*mll,

	*total,
	*vistotal;

	float mlbcombthresh_;


};

}
#endif /* ANALYSISPLOTSJAN_H_ */
