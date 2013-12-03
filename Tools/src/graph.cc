/*
 * graph.cc
 *
 *  Created on: Nov 14, 2013
 *      Author: kiesej
 */

#include "../interface/graph.h"
#include "TGraphAsymmErrors.h"
#include <algorithm>
#include "../interface/container.h"
namespace ztop{
namespace graphhelperfunctions{
struct sort_pairbyfirst {
	bool operator()(const std::pair<float,size_t> &left, const std::pair<float,size_t> &right) {
		return left.first < right.first;
	}
};

}

std::vector<ztop::graph *> graph::g_list;
bool graph::g_makelist=false;
bool graph::debug=false;

graph::graph(const TString &name):chi2definition(symmetrize),name_(name),labelmultiplier_(1){
	if(g_makelist) addToList();
}

graph::graph(const size_t & npoints,const TString &name):chi2definition(symmetrize),name_(name), xcoords_(npoints),ycoords_(npoints),labelmultiplier_(1){
	if(g_makelist) addToList();
}

graph::~graph(){
	for(size_t i=0;i<g_list.size();i++){
		if(g_list.at(i) == this)
			g_list.erase(g_list.begin()+i);
	}
}
/**
 * deletes all content and sets N points to be filled
 * Better performance than addPoint.
 */
void graph::setNPoints(const size_t & npoints){
	xcoords_=histoContent(npoints);
	ycoords_=histoContent(npoints);
}

/**
 * don't use if you need performance. Better directly construct with number of needed points
 */
size_t graph::addPoint(const float& x, const float & y){
	histoContent newxcoords(xcoords_.size()+1);
	histoContent newycoords(xcoords_.size()+1);
	size_t oldsize=xcoords_.size();
	for(int sysl=-1;sysl<(int)xcoords_.layerSize();sysl++){
		if(sysl>-1){ //add layer
			newxcoords.addLayer(xcoords_.getLayerName(sysl));
			newycoords.addLayer(ycoords_.getLayerName(sysl));
		}

		for(size_t i=0;i<oldsize;i++){ //fill content
			newxcoords.getBin(i,sysl)=xcoords_.getBin(i,sysl);
			newycoords.getBin(i,sysl)=ycoords_.getBin(i,sysl);
		}
	}
	//add new content
	for(int sysl=-1;sysl<(int)newxcoords.layerSize();sysl++){
		newxcoords.getBin(oldsize,sysl).setContent(x);
		newycoords.getBin(oldsize,sysl).setContent(y);
	}

	xcoords_=newxcoords;
	ycoords_=newycoords;

	return oldsize;
}

void graph::setPointContents(const size_t & point, bool nomstat, const float & xcont, const float & ycont, const int & syslayer){
	if(nomstat){
		getPointX(point,syslayer).setContent(xcont);
		getPointY(point,syslayer).setContent(ycont);
	}
	else{
		getPointX(point,syslayer).setStat(xcont);
		getPointY(point,syslayer).setStat(ycont);
	}
}
void graph::setPointXContent(const size_t & point, const float & xcont, const int & syslayer){
	getPointX(point,syslayer).setContent(xcont);
}
void graph::setPointYContent(const size_t & point, const float & ycont, const int & syslayer){
	getPointY(point,syslayer).setContent(ycont);
}
void graph::setPointXStat(const size_t & point, const float & xcont, const int & syslayer){
	getPointX(point,syslayer).setStat(xcont);
}
void graph::setPointYStat(const size_t & point, const float & ycont, const int & syslayer){
	getPointY(point,syslayer).setStat(ycont);
}

//wrappers
const float &  graph::getPointXContent(const size_t & point, const int & syslayer) const{
	return getPointX(point,syslayer).getContent();
}
const float &  graph::getPointYContent(const size_t & point, const int & syslayer) const{
	return getPointY(point,syslayer).getContent();
}
float   graph::getPointXStat(const size_t & point, const int & syslayer) const{
	return getPointX(point,syslayer).getStat();
}
float   graph::getPointYStat(const size_t & point, const int & syslayer) const{
	return getPointY(point,syslayer).getStat();
}



/**
 * return index of new layer
 * needs same indices for points
 */
size_t graph::addErrorGraph(const TString &name, const graph & errg){
	if(errg.getNPoints()!=getNPoints()){
		std::cout << "graph::addErrorGraph: graphs need to have same number of points" <<std::endl;
		throw std::logic_error("graph::addErrorGraph: graphs need to have same number of points");
	}
	size_t sysidx=xcoords_.addLayer(name);
	if(sysidx!=xcoords_.layerSize()-1){//layer not new!!
		std::cout << "graph::addErrorGraph: error already added, what should I do??? I decide to leave" <<std::endl;
		throw std::logic_error("graph::addErrorGraph: error already added, what should I do??? I decide to leave");
	}
	xcoords_.getLayer(sysidx)=errg.xcoords_.getNominal();
	ycoords_.getLayer(sysidx)=errg.ycoords_.getNominal();
	return sysidx;
}
graph  graph::getSystGraph(const size_t sysidx) const{
	if(sysidx >=ycoords_.layerSize()){
		std::cout << "graph::getSystGraph: out of range " <<std::endl;
		throw std::out_of_range("graph::getSystGraph: out of range ");
	}
	graph out=*this;
	out.clear();
	out.xcoords_.getNominal()=xcoords_.getLayer(sysidx);
	out.ycoords_.getNominal()=ycoords_.getLayer(sysidx);
	return out;
}

void graph::clear(){
	xcoords_.clear();
	ycoords_.clear();
}
graph graph::getNominalGraph()const{
	graph out=*this;
	out.xcoords_.removeAdditionalLayers();
	out.ycoords_.removeAdditionalLayers();
	return out;
}
const TString &graph::getSystErrorName(const size_t &number) const{
	if(number>=xcoords_.layerSize()){
		std::cout << "graph::getSystErrorName: " << number << " out of range(" << (int)getSystSize()-1 << ")" << std::endl;
		return name_;
	}
	return xcoords_.getLayerName(number);
}
const size_t & graph::getSystErrorIndex(const TString & name) const{
	return xcoords_.getLayerIndex(name);
}
void graph::removeYErrors(){
	for(int sys=-1;sys<(int)getSystSize();sys++){
		for(size_t p=0;p<getNPoints();p++){ //stat 0 all sys equal content
			setPointYStat(p,0,sys);
			float nomcont=getPointYContent(p,-1);
			setPointYContent(p,nomcont,sys);
		}
	}
}
/**
 * changes indices of coordinates according to their x coordinates (increasing order)
 *
 */
void graph::sortPointsByX(){
	std::vector<std::pair<float,size_t> >tx;
	for(size_t i=0;i<getNPoints();i++){
		float cont=getPointXContent(i);
		std::pair<float,size_t> pair(cont,i);
		tx.push_back(pair);
	}
	std::sort(tx.begin(),tx.end(),graphhelperfunctions::sort_pairbyfirst());
	histoContent newxcoords=xcoords_;
	histoContent newycoords=ycoords_;
	for(int sys=-1;sys<(int)getSystSize();sys++){
		for(size_t i=0;i<getNPoints();i++){
			size_t oldidx=tx.at(i).second;
			newxcoords.getBin(i,sys)=xcoords_.getBin(oldidx,sys);
			newycoords.getBin(i,sys)=ycoords_.getBin(oldidx,sys);
		}
	}
	xcoords_=newxcoords;
	ycoords_=newycoords;
}
graph graph::addY(const graph & rhs) const{
	if(rhs.getNPoints() != getNPoints()){
		std::cout << "graph::addY: number of points does not match" <<std::endl;
		throw std::runtime_error("graph::addY: number of points does not match");
	}
	//not yet implemented

	graph grhs=rhs;
	graph out=*this;
	out.sortPointsByX();
	grhs.sortPointsByX();
	//all x need to be the same, xerr the same
	//later: allow more or less errors
	for(int sys=-1;sys<(int)getSystSize();sys++){
		sys++;
	}
	out.ycoords_ += grhs.ycoords_;

	return out;
}


/**
 * defines binwidth as stat of nominal ypoints
 * ignores underflow and overflow
 */
void graph::import(const container1D * cont,bool dividebybinwidth){
	xcoords_=histoContent(cont->getNBins());
	ycoords_=histoContent(cont->getNBins());
	int syssize=(int)cont->getSystSize();
	for(int sys=-1;sys<syssize;sys++){
		if(sys>-1){
			xcoords_.addLayer(cont->getSystErrorName(sys));
			ycoords_.addLayer(cont->getSystErrorName(sys));
		}
		for(size_t point=0;point<cont->getNBins();point++){
			ycoords_.getBin(point,sys)=cont->getBin(point+1,sys);
			if(dividebybinwidth)
				ycoords_.getBin(point,sys).multiply(1/cont->getBinWidth(point+1));
			xcoords_.getBin(point,sys).setContent(cont->getBinCenter(point+1));
			if(sys<0){
				xcoords_.getBin(point,sys).setStat(cont->getBinWidth(point+1)/2);
			}
			else{
				xcoords_.getBin(point,sys).setStat(0);
			}
		}
	}
	yname_=cont->getYAxisName();
	xname_=cont->getXAxisName();
}

/**
 * chi2 ...
 * needs exactly same x bins without errors!
 *
 * think about restricting it to specific points
 */
graph graph::getChi2Points(const graph & gcompare)const{
	if(getNPoints()!=gcompare.getNPoints()){
		std::cout << "graph::getChi2: needs same number of points" << std::endl;
		throw std::logic_error("graph::getChi2: needs same number of points" );
	}
	//only perfoming loose x check
	for(size_t i=0;i<getNPoints();i++){
		if(getPointXContent(i) != gcompare.getPointXContent(i)){
			std::cout << "graph::getChi2: needs same x coordinates" << std::endl;
			throw std::logic_error("graph::getChi2: needs same x coordinates" );
		}
	}

	//needs some thinking...


	graph out=*this;//get number of points
	out.clear();//clear
	//return out;//////////
	size_t nans=0;
	//compare systematics by systematics if present -> later
	//first ignore syst correlations
	//treat everything as uncorrelated
	for(size_t p=0;p<getNPoints();p++){
		float diff=getPointYContent(p)-gcompare.getPointYContent(p);
		float err2=0;
		if(chi2definition==swap){
			if(diff>0){ //use left down, right up
				float lerr=getPointYErrorDown(p,false);
				float rerr=gcompare.getPointYErrorUp(p,false);
				err2=lerr*lerr+rerr*rerr;
			}
			else{
				float lerr=getPointYErrorUp(p,false);
				float rerr=gcompare.getPointYErrorDown(p,false);
				err2=lerr*lerr+rerr*rerr;
			}
		}
		else if(chi2definition==symmetrize){
			float lerr=getPointYError(p,false);
			float rerr=gcompare.getPointYError(p,false);
			err2=lerr*lerr+rerr*rerr;
		}
		out.setPointXContent(p,getPointXContent(p));
		float chi2point=diff*diff/err2;
		if(chi2point!=chi2point){
			chi2point=0;
			nans++;
		}
		out.setPointYContent(p,chi2point);
	}
	if(nans>0){
		std::cout << "graph::getChi2Points: nans created. ";
		if(nans == out.getNPoints())
			std::cout << " All chi2=0, keep in mind for ndof "<<std::endl;
		else
			std::cout << " Only some chi2=0, WARNING!!! Result will not be useful!" <<std::endl;
	}

	out.setYAxisName("#chi^{2}");
	return out;
}

TGraphAsymmErrors * graph::getTGraph(TString name,bool onlystat) const{
	if(name=="") name=name_;
	if(getNPoints()<1)
		return 0;
	std::vector<float> x,y,xeh,xel,yeh,yel;

	for(size_t i=0;i<getNPoints();i++){

		x.push_back(xcoords_.getBinContent(i));
		xeh.push_back(getPointError(i,false,true,onlystat));
		xel.push_back(getPointError(i,false,false,onlystat));
		y.push_back(ycoords_.getBinContent(i));
		yeh.push_back(getPointError(i,true,true,onlystat));
		yel.push_back(getPointError(i,true,false,onlystat));

	}
	TGraphAsymmErrors * g = new TGraphAsymmErrors(getNPoints(),&x.at(0),&y.at(0),&xel.at(0),&xeh.at(0),&yel.at(0),&yeh.at(0));
	g->SetName(name);
	g->SetTitle(name);
	g->GetYaxis()->SetTitleSize(0.06*labelmultiplier_);
	g->GetYaxis()->SetLabelSize(0.05*labelmultiplier_);
	g->GetYaxis()->SetTitleOffset(g->GetYaxis()->GetTitleOffset() / labelmultiplier_);
	g->GetYaxis()->SetTitle(yname_);
	g->GetXaxis()->SetTitleSize(0.06*labelmultiplier_);
	g->GetXaxis()->SetLabelSize(0.05*labelmultiplier_);
	g->GetYaxis()->SetNdivisions(510);
	g->GetXaxis()->SetTitle(xname_);
	g->SetMarkerStyle(20);
	std::sort(x.begin(),x.end());
	g->GetXaxis()->SetRangeUser(x.at(0)-(0.1 * (x.at(getNPoints()-1)-x.at(0))) , x.at(getNPoints()-1)+(0.1 * (x.at(getNPoints()-1)-x.at(0))));

	return g;

}
float graph::getPointError(const size_t & point, bool yvar, bool updown, bool onlystat,const TString &limittosys)const {
	float fullerr2=0;
	const histoContent * content=&xcoords_;
	if(yvar) content=&ycoords_;

	if(point<getNPoints()){
		fullerr2=content->getBin(point).getStat2(); //stat
		if(onlystat)
			return sqrt(fullerr2);
		if(limittosys==""){
			// make vector of all sys stripped
			std::vector<TString> sources;
			for(size_t i=0;i<content->layerSize();i++){ //there might be room for improvement here...
				if(debug){
					std::cout << "graph::getpointErrorUp: checking variation with index: " <<i<< "("<< content->layerSize() << ")"<<std::endl;
					std::cout << "graph::getpointErrorUp: stripping variation with name: " <<content->getLayerName(i)<<std::endl;
				}
				TString source=stripVariation(content->getLayerName(i));
				if(-1==isIn(source,sources)){
					sources.push_back(source);
					float var=getDominantVariation(source,point,yvar,updown);
					fullerr2 += var*var;
				}
			}
		}
		else{
			float var=getDominantVariation(limittosys,point,yvar,updown);
			fullerr2 += var*var;
		}
		return sqrt(fullerr2);
	}
	else{
		std::cout << "graph::getPointError: point out of range" << std::endl;
		throw std::out_of_range("graph::getPointError: point out of range");
	}
}
////some wrappers

float graph::getPointXErrorUp(const size_t & point, bool onlystat,const TString &limittosys)const{
	return getPointError(point, false,true,onlystat,limittosys);
}
float graph::getPointXErrorDown(const size_t & point, bool onlystat,const TString &limittosys)const{
	return getPointError(point, false,false,onlystat,limittosys);
}
float graph::getPointYErrorUp(const size_t & point, bool onlystat,const TString &limittosys)const{
	return getPointError(point, true,true,onlystat,limittosys);
}
float graph::getPointYErrorDown(const size_t & point, bool onlystat,const TString &limittosys)const{
	return getPointError(point, true,false,onlystat,limittosys);
}
/**
 * symmetrize to maximum
 */
float graph::getPointYError(const size_t & point, bool onlystat,const TString &limittosys)const{
	float up=getPointYErrorUp(point,  onlystat,limittosys);
	float down=getPointYErrorDown(point,  onlystat,limittosys);
	if(up>down) return up;
	else return down;
}

/**
 * symmetrize to maximum
 */
float graph::getPointXError(const size_t & point, bool onlystat,const TString &limittosys)const{
	float up=getPointXErrorUp(point,  onlystat,limittosys);
	float down=getPointXErrorDown(point,  onlystat,limittosys);
	if(up>down) return up;
	else return down;
}



float  graph::getXMax() const{
	float max=-999999999999.;
	for(size_t i=0;i<getNPoints();i++){
		const float & c=getPointXContent(i);
		if(max < c) max = c;
	}
	return max;
}
float  graph::getXMin() const{
	float min=999999999999.;
	for(size_t i=0;i<getNPoints();i++){
		const float & c=getPointXContent(i);
		if(min > c) min = c;
	}
	return min;
}
float  graph::getYMax() const{
	float max=-999999999999.;
	for(size_t i=0;i<getNPoints();i++){
		const float & c=getPointYContent(i);
		if(max < c) max = c;
	}
	return max;
}
float  graph::getYMin() const{
	float min=999999999999.;
	for(size_t i=0;i<getNPoints();i++){
		const float & c=getPointYContent(i);
		if(min > c) min = c;
	}
	return min;
}


///////////////PROTECTED////////

float graph::getDominantVariation( TString  sysname, const size_t& bin, bool yvar,bool getup) const{ //TString copy on purpose
	float up=0,down=0;
	const histoContent * contents=&ycoords_;
	if(!yvar) contents=&xcoords_;
	const float & cont=contents->getBin(bin).getContent();
	size_t idx=contents->getLayerIndex(sysname+"_up");
	if(idx >= contents->layerSize())
		std::cout << "graph::getDominantVariation: serious error: " << sysname << "_up not found" << std::endl;
	up=contents->getBin(bin,idx).getContent()-cont;
	idx=contents->getLayerIndex(sysname+"_down");
	if(idx >= contents->layerSize())
		std::cout << "graph::getDominantVariation: serious error: " << sysname << "_down not found" << std::endl;
	down=contents->getBin(bin,idx).getContent()-cont;
	if(getup && up>down)down=up;
	else if(!getup && up<down)down=up;
	return down;
}

TString graph::stripVariation(const TString &in) const{
	TString out=in;
	//out.Resize(in.Last('_'));
	out.ReplaceAll("_up","");
	out.ReplaceAll("_down","");
	return out;
}

void graph::addToList(){
	g_list.push_back(this);
}


histoBin & graph::getPointX(const size_t & pointidx,const int & layer){
	if(pointidx>=xcoords_.size() || layer >=(int)xcoords_.layerSize()){
		std::cout << "graph::getPointX: out of range " <<std::endl;
		throw std::out_of_range("graph::getPointX: out of range ");
	}
	return xcoords_.getBin(pointidx,layer);
}
const histoBin & graph::getPointX(const size_t & pointidx,const int & layer) const{
	if(pointidx>=xcoords_.size() || layer >=(int)xcoords_.layerSize()){
		std::cout << "graph::getPointX: out of range " <<std::endl;
		throw std::out_of_range("graph::getPointX: out of range ");
	}
	return xcoords_.getBin(pointidx,layer);
}

histoBin & graph::getPointY(const size_t & pointidx,const int & layer){
	if(pointidx>=ycoords_.size() || layer >=(int)ycoords_.layerSize()){
		std::cout << "graph::getPointY: out of range " <<std::endl;
		throw std::out_of_range("graph::getPointY: out of range ");
	}
	return ycoords_.getBin(pointidx,layer);
}
const histoBin & graph::getPointY(const size_t & pointidx,const int & layer) const{
	if(pointidx>=ycoords_.size() || layer >=(int)ycoords_.layerSize()){
		std::cout << "graph::getPointY: out of range " <<std::endl;
		throw std::out_of_range("graph::getPointY: out of range ");
	}
	return ycoords_.getBin(pointidx,layer);
}

}//namespace


