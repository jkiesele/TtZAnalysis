/*
 * histo1DUnfold.cc
 *
 *  Created on: Aug 15, 2013
 *      Author: kiesej
 */

#include "../interface/histo1DUnfold.h"
#include "FWCore/FWLite/interface/AutoLibraryLoader.h"
#include "../interface/systAdder.h"


/*
 * multicore usage for unfolding to be implemented
 */
#include <omp.h>
#include <stdexcept>
#include "TTree.h"
#include "TFile.h"
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>
namespace ztop{


bool histo1DUnfold::debug=false;
std::vector<histo1DUnfold*> histo1DUnfold::c_list;
bool histo1DUnfold::c_makelist=false;
//bool histo1DUnfold::printinfo=false;

/*
 *
   float tempgen_,tempreco_,tempgenweight_,tempweight_;
   bool recofill_,genfill_;

   bool isMC_;
 */

histo1DUnfold::histo1DUnfold(): histo2D(std::vector<float>(),std::vector<float>(),"","","","",false),xaxis1Dname_(""), yaxis1Dname_(""),
		tempgen_(0),tempreco_(0),tempgenweight_(1),tempweight_(1),recofill_(false),
		genfill_(false),isMC_(false),flushed_(true),binbybin_(false),lumi_(1),congruentbins_(false),allowmultirecofill_(false){
	if(c_makelist){
		c_list.push_back(this);
	}
	type_=type_container1DUnfold;
}
histo1DUnfold::histo1DUnfold( std::vector<float> genbins, std::vector<float> recobins, TString name,TString xaxisname,TString yaxisname, bool mergeufof)
:histo2D( recobins /*genbins*/ , recobins , name,xaxisname+" (gen)",xaxisname+" (reco)",yaxisname,false), xaxis1Dname_(xaxisname),
 yaxis1Dname_(yaxisname),tempgen_(0),tempreco_(0),tempgenweight_(1),tempweight_(1),recofill_(false),genfill_(false),
 isMC_(false),flushed_(true),binbybin_(false),lumi_(1),congruentbins_(false),allowmultirecofill_(false){
	//bins are set, containers created, at least conts_[0] exists with all options (binomial, mergeufof etc)

	//histo2D constructed without merge. -> UF OF in x direction

	mergeufof_=mergeufof;
	genbins_=genbins; //can be changed and rebinned afterwards

	//everything in same binning for now
	std::vector<float> databins=ybins_;
	databins.erase(databins.begin());
	gencont_  = histo1D(databins,name_+"_gen",xaxis1Dname_,yaxis1Dname_,mergeufof_);
	recocont_ = histo1D(databins,name_+"_data",xaxis1Dname_,yaxis1Dname_,mergeufof_);
	unfolded_ = histo1D(databins,name_+"_unfolded",xaxis1Dname_,yaxis1Dname_,mergeufof_);
	refolded_ = histo1D(databins,name_+"_refolded",xaxis1Dname_,yaxis1Dname_,mergeufof_);
	congruentbins_=checkCongruentBinBoundariesXY();
	if(c_makelist){
		c_list.push_back(this);
	}
	type_=type_container1DUnfold;
}
histo1DUnfold::histo1DUnfold( std::vector<float> genbins, TString name,TString xaxisname,TString yaxisname, bool mergeufof){
	std::vector<float> recobins=subdivide(genbins,10);
	histo1DUnfold(genbins,recobins,name,xaxisname,yaxisname,mergeufof);
}

histo1DUnfold::~histo1DUnfold(){
	for(size_t i=0;i<c_list.size();i++){
		if(this==c_list.at(i))
			c_list.erase(c_list.begin()+i);
	}
}

void histo1DUnfold::setBinning(const std::vector<float> & genbins,const std::vector<float> &recobins){
	genbins_=genbins;
	bool temp=mergeufof_;
	mergeufof_=false;
	histo2D::setBinning(recobins,recobins);
	mergeufof_=temp;
	gencont_=conts_.at(0);
	gencont_.clear();
	gencont_.setXAxisName(xaxis1Dname_+"(gen)");
	std::vector<float> databins=ybins_;
	databins.erase(databins.begin());
	recocont_ = histo1D(databins,name_+"_data",xaxis1Dname_,yaxis1Dname_,mergeufof_);
	unfolded_ = histo1D(databins,name_+"_unfolded",xaxis1Dname_,yaxis1Dname_,mergeufof_);
	refolded_ = histo1D(databins,name_+"_refolded",xaxis1Dname_,yaxis1Dname_,mergeufof_);
	congruentbins_=checkCongruentBinBoundariesXY();
}
void histo1DUnfold::setGenBinning(const std::vector<float> & genbins){
	if(checkCongruence(genbins,xbins_)){
		genbins_=genbins;
	}
	else{
		throw std::logic_error("container1DUnfold::setGenBinning: Binning does not match bin boundaries");
	}
}

histo1DUnfold histo1DUnfold::rebinToBinning(const std::vector<float> & newbins)const{
	size_t maxbinssize=xbins_.size();
	if(newbins.size()>maxbinssize) maxbinssize=newbins.size();
	std::vector<float> sames(maxbinssize);
	std::vector<float>::iterator it=std::set_intersection(++xbins_.begin(),xbins_.end(),newbins.begin(),newbins.end(),sames.begin());//ignore UF
	sames.resize(it-sames.begin());
	//check if possible at all:
	if(sames.size()!=newbins.size()){
		std::cout << "container1DUnfold::rebinToBinning: binning not compatible" <<std::endl;
		return *this;
	}
	histo1DUnfold out=*this;
	histo2D tempc2d=out.rebinXToBinning(newbins);
	tempc2d=tempc2d.rebinYToBinning(newbins);
	out.conts_=tempc2d.conts_;
	out.xbins_=tempc2d.xbins_;
	out.ybins_=tempc2d.ybins_;

	out.recocont_=recocont_.rebinToBinning(newbins);
	out.gencont_=gencont_.rebinToBinning(newbins);
	out.unfolded_=unfolded_.rebinToBinning(newbins);
	out.refolded_=refolded_.rebinToBinning(newbins);

	return out;

}
void histo1DUnfold::clear(){
	histo2D::clear();
	gencont_.clear();
	recocont_.clear();
	unfolded_.clear();
	refolded_.clear();
}
void histo1DUnfold::reset(){
	histo2D::reset();
	gencont_.reset();
	recocont_.reset();
	unfolded_.reset();
	refolded_.reset();

}

void histo1DUnfold::setAllZero(){
	histo2D::setAllZero(); gencont_.setAllZero(); recocont_.setAllZero();
	unfolded_.setAllZero();
	refolded_.setAllZero();
}

void histo1DUnfold::removeAllSystematics(const TString& exception){
	for(size_t i=0;i<conts_.size();i++){
		conts_.at(i).removeAllSystematics(exception);
	}
	recocont_.removeAllSystematics(exception);
	unfolded_.removeAllSystematics(exception);
	gencont_.removeAllSystematics(exception);
	refolded_.removeAllSystematics(exception);
}
void histo1DUnfold::splitSystematic(const size_t & number, const float& fracadivb,
		const TString & splinamea,  const TString & splinameb){
	for(size_t i=0;i<conts_.size();i++){
		conts_.at(i).splitSystematic(number,fracadivb, splinamea, splinameb);
	}
	recocont_.splitSystematic(number,fracadivb, splinamea, splinameb);
	unfolded_.splitSystematic(number,fracadivb, splinamea, splinameb);
	gencont_.splitSystematic(number,fracadivb, splinamea, splinameb);
	refolded_.splitSystematic(number,fracadivb, splinamea, splinameb);
}
void histo1DUnfold::mergePartialVariations(const TString& identifier,bool strictpartialID){
	for(size_t i=0;i<conts_.size();i++){
		conts_.at(i).mergePartialVariations(identifier,strictpartialID);
	}
	recocont_.mergePartialVariations(identifier,strictpartialID);
	unfolded_.mergePartialVariations(identifier,strictpartialID);
	gencont_.mergePartialVariations(identifier,strictpartialID);
	refolded_.mergePartialVariations(identifier,strictpartialID);
}
void histo1DUnfold::mergeAllErrors(const TString & mergedname){
	for(size_t i=0;i<conts_.size();i++){
		conts_.at(i).mergeAllErrors(mergedname);
	}
	recocont_.mergeAllErrors(mergedname);
	unfolded_.mergeAllErrors(mergedname);
	gencont_.mergeAllErrors(mergedname);
	refolded_.mergeAllErrors(mergedname);
}

void histo1DUnfold::equalizeSystematicsIdxs(histo1DUnfold &rhs){
	if(rhs.conts_.size() == conts_.size()){
		for(size_t i=0;i<conts_.size();i++){
			conts_.at(i).equalizeSystematicsIdxs(rhs.conts_.at(i));
		}
		recocont_.equalizeSystematicsIdxs(rhs.recocont_);
		unfolded_.equalizeSystematicsIdxs(rhs.unfolded_);
		gencont_.equalizeSystematicsIdxs(rhs.gencont_);
		refolded_.equalizeSystematicsIdxs(rhs.refolded_);
	}
	else{
		throw std::logic_error("container1DUnfold::equalizeSystematicsIdxs: TBI for different conts_size()");
	}
}

void histo1DUnfold::mergeVariations(const std::vector<TString>& names, const TString & outname,bool linearly){
	try{
		histo2D::mergeVariations(names,outname,linearly);

		recocont_.mergeVariations(names,outname,linearly);
		unfolded_.mergeVariations(names,outname,linearly);
		gencont_.mergeVariations(names,outname,linearly);
		refolded_.mergeVariations(names,outname,linearly);
	}
	catch(std::exception &e){
		throw e;
	}
}
void histo1DUnfold::mergeVariationsFromFileInCMSSW(const std::string& filename){
	systAdder adder;
	adder.readMergeVariationsFileInCMSSW(filename);
	size_t ntobemerged=adder.mergeVariationsSize();
	for(size_t i=0;i<ntobemerged;i++){

		TString mergedname=adder.getMergedName(i);
		std::vector<TString> tobemerged=adder.getToBeMergedName(i);
		bool linearly=adder.getToBeMergedLinearly(i);

		mergeVariations(tobemerged,mergedname,linearly);
	}
}


void histo1DUnfold::setBinByBin(bool bbb){
	if(!congruentbins_){
		std::cout << "container1DUnfold::setBinByBin: only possible if congruent bin boundaries for reco/gen! doing nothing" <<std::endl;
		return;
	}
	binbybin_=bbb;
}

void histo1DUnfold::setRecoContainer(const ztop::histo1D &cont){
	if(cont.getBins() != recocont_.getBins()){
		std::cout << "container1DUnfold::setRecoContainer: wrong binning" <<std::endl;
		return;
	}
	recocont_=cont;
}


void histo1DUnfold::setBackground(const histo1D & cont){
	//background is supposed to be in x UF bin //no OFUF
	if(cont.getBins() != ybins_){
		std::cout << "container1DUnfold::setBackground: Binning not compatible. doing nothing" << std::endl;
		return;
	}
	if(xbins_.size()<1){
		std::cout << "container1DUnfold::setBackground: No X bins!" << std::endl;
	}
	setXSlice(0,cont,false);
	//remove gen  OF
	for(int  sys=-1;sys<(int)getSystSize();sys++){
		getBin(xbins_.size()-1,0,sys).multiply(0);
	}

}
histo1D histo1DUnfold::getBackground() const{
	if(xbins_.size()<1){
		std::cout << "container1DUnfold::getBackground: No bins!" << std::endl;
		throw std::logic_error("container1DUnfold::getBackground: no bins");
	}
	histo1D out=getXSlice(0);
	//add generator overflow
	//	for(int  sys=-1;sys<(int)getSystSize();sys++)
	//	out.getBin(out.getBins().size()-1,sys).add( getBin(xbins_.size()-1,0,sys));

	out+=getXSlice(xbins_.size()-1);
	out.setXAxisName(xaxis1Dname_);
	out.setYAxisName(yaxis1Dname_);
	return out;
}
histo1D histo1DUnfold::getVisibleSignal() const{
	if(xbins_.size()<1){
		std::cout << "container1DUnfold::getVisibleSignal: No bins!" << std::endl;
		throw std::logic_error("container1DUnfold::getVisibleSignal: no bins");
	}
	bool tmp=histoContent::subtractStatCorrelated;
	histoContent::subtractStatCorrelated=true;
	histo1D out=recocont_ - getBackground();
	histoContent::subtractStatCorrelated=tmp;
	out.setXAxisName(xaxis1Dname_);
	out.setYAxisName(yaxis1Dname_);
	return  out;//no underflow included -> only visible part of response matrix -> generated and reconstructed in visible PS
}
histo1DUnfold& histo1DUnfold::operator += (const histo1DUnfold & second){
	if((second.xbins_ != xbins_ || second.ybins_ != ybins_) && !isDummy()){
		std::cout << "container1DUnfold::operator +=: "<< name_ << " and " << second.name_<<" must have same binning! returning *this" << std::endl;
		return *this;
	}
	if(isDummy()){
		*this=second;
		return *this;
	}
	for(size_t i=0;i<conts_.size();i++){
		conts_.at(i) += second.conts_.at(i);
	}
	gencont_+=second.gencont_;
	recocont_+=second.recocont_;
	unfolded_+=second.unfolded_;
	refolded_+=second.refolded_;
	return *this;
}

histo1DUnfold histo1DUnfold::operator + (const histo1DUnfold & second)const{
	histo1DUnfold out=*this;
	out+=second;
	return out;
}
histo1DUnfold& histo1DUnfold::operator -= (const histo1DUnfold & second){
	if(second.xbins_ != xbins_ || second.ybins_ != ybins_){
		std::cout << "container1DUnfold::operator -=: "<< name_ << " and " << second.name_<<" must have same binning! returning *this"  << std::endl;
		return *this;
	}

	for(size_t i=0;i<conts_.size();i++){
		conts_.at(i) -= second.conts_.at(i);
	}
	gencont_-=second.gencont_;
	recocont_-=second.recocont_;
	unfolded_-=second.unfolded_;
	refolded_-=second.refolded_;

	return *this;
}
histo1DUnfold histo1DUnfold::operator - (const histo1DUnfold & second)const{
	histo1DUnfold out=*this;
	out-=second;
	return out;
}

histo1DUnfold& histo1DUnfold::operator /= (const histo1DUnfold & second){
	if(second.xbins_ != xbins_ || second.ybins_ != ybins_){
		std::cout << "container1DUnfold::operator /: "<< name_ << " and " << second.name_<<" must have same binning! returning *this"  << std::endl;
		return *this;
	}
	if(second.divideBinomial_!=divideBinomial_){
		std::cout << "container1DUnfold::operator /: "<< name_ << " and " << second.name_<<" must have same divide option (divideBinomial). returning *this" <<  std::endl;
		return *this;
	}
	for(size_t i=0;i<conts_.size();i++){
		conts_.at(i) /= second.conts_.at(i);
	}
	gencont_/=second.gencont_;
	recocont_/=second.recocont_;
	unfolded_/=second.unfolded_;
	refolded_/=second.refolded_;

	return *this;
}
histo1DUnfold histo1DUnfold::operator / (const histo1DUnfold & second)const{
	histo1DUnfold out=*this;
	out/=second;
	return out;
}


histo1DUnfold &histo1DUnfold::operator *= (const histo1DUnfold & second){
	if(second.xbins_ != xbins_ || second.ybins_ != ybins_){
		std::cout << "container1DUnfold::operator *: "<< name_ << " and " << second.name_<<" must have same binning! returning *this"  << std::endl;
		return *this;
	}

	for(size_t i=0;i<conts_.size();i++){
		conts_.at(i) *= second.conts_.at(i);
	}
	gencont_*=second.gencont_;
	recocont_*=second.recocont_;
	unfolded_*=second.unfolded_;
	refolded_*=second.refolded_;

	return *this;
}

histo1DUnfold histo1DUnfold::operator * (const histo1DUnfold & second)const{
	histo1DUnfold out=*this;
	out*=second;
	return out;
}

histo1DUnfold histo1DUnfold::operator * (float val)const{
	histo1DUnfold out=*this;
	out*=val;
	return out;
}
histo1DUnfold& histo1DUnfold::operator *= (float val){
	for(size_t i=0;i<conts_.size();i++){
		conts_.at(i) *= val;
	}
	gencont_*= val;
	recocont_*=val;
	unfolded_*=val;
	refolded_*=val;
	return *this;
}
histo1DUnfold histo1DUnfold::operator * (double val)const{
	return *this * (float) val;
}
histo1DUnfold& histo1DUnfold::operator *= (double val){
	*this *= (float) val;
	return *this;
}
bool histo1DUnfold::operator == (const histo1DUnfold &rhs)const{
	if(!isEqual(rhs))
		return false;
	/*
	 *
	TString xaxis1Dname_,yaxis1Dname_;

	float tempgen_,tempreco_,tempgenweight_,tempweight_;
	bool recofill_,genfill_;

	bool isMC_;
	bool flushed_;
	bool binbybin_;

	histo1D gencont_,recocont_,unfolded_,refolded_;
	std::vector<float> genbins_;

	float lumi_;

	bool congruentbins_;

	bool allowmultirecofill_;
	 */
	if(xaxis1Dname_!=rhs.xaxis1Dname_)
		return false;
	if(yaxis1Dname_!=rhs.yaxis1Dname_)
		return false;
	if(tempgen_!=rhs.tempgen_)
		return false;
	if(tempreco_!=rhs.tempreco_)
		return false;
	if(tempgenweight_!=rhs.tempgenweight_)
		return false;
	if(tempweight_!=rhs.tempweight_)
		return false;
	if(recofill_!=rhs.recofill_)
		return false;
	if(genfill_!=rhs.genfill_)
		return false;
	if(isMC_!=rhs.isMC_)
		return false;
	if(flushed_!=rhs.flushed_)
		return false;
	if(binbybin_!=rhs.binbybin_)
		return false;
	if(gencont_!=rhs.gencont_)
		return false;
	if(recocont_!=rhs.recocont_)
		return false;
	if(unfolded_!=rhs.unfolded_)
		return false;
	if(refolded_!=rhs.refolded_)
		return false;
	if(genbins_!=rhs.genbins_)
		return false;
	if(lumi_!=rhs.lumi_)
		return false;
	if(congruentbins_!=rhs.congruentbins_)
		return false;
	if(allowmultirecofill_!=rhs.allowmultirecofill_)
		return false;
	return true;
}

void histo1DUnfold::setDivideBinomial(bool binomial){
	for(size_t i=0;i<conts_.size();i++)
		conts_.at(i).setDivideBinomial(binomial);
	gencont_.setDivideBinomial(binomial);
	recocont_.setDivideBinomial(binomial);
	unfolded_.setDivideBinomial(binomial);
	refolded_.setDivideBinomial(binomial);
	divideBinomial_=binomial;
}
int histo1DUnfold::addErrorContainer(const TString & sysname,const histo1DUnfold & cont,float weight){
	if(xbins_ != cont.xbins_ || ybins_ != cont.ybins_){
		std::cout << "container1DUnfold::addErrorContainer: " << name_ << " and " << cont.name_ << " must have same x and y axis!" << std::endl;
	}
	if(debug)
		std::cout << "container1DUnfold::addErrorContainer: adding for all 2d bins" <<std::endl;
	int out=0;
	int tmp=0;
	for(size_t i=0;i<conts_.size();i++){
		tmp=conts_.at(i).addErrorContainer(sysname,cont.conts_.at(i),weight);
		if(tmp<0) out=tmp;
	}
	if(debug)
		std::cout << "container1DUnfold::addErrorContainer: adding for gen and data distr" <<std::endl;

	tmp=gencont_.addErrorContainer(sysname,cont.gencont_,weight);
	if(tmp<0) out=tmp;
	tmp=recocont_.addErrorContainer(sysname,cont.recocont_,weight);
	if(tmp<0) out=tmp;
	if(debug)
		std::cout << "container1DUnfold::addErrorContainer: adding for unfolded and refolded distr" <<std::endl;
	tmp=unfolded_.addErrorContainer(sysname,cont.unfolded_,weight);
	if(tmp<0) out=tmp;
	tmp=refolded_.addErrorContainer(sysname,cont.refolded_,weight);
	if(tmp<0) out=tmp;
	return out;
}
int histo1DUnfold::addErrorContainer(const TString & sysname,const histo1DUnfold & cont){
	return addErrorContainer(sysname,cont,1);
}

void histo1DUnfold::addGlobalRelErrorUp(TString name,float relerr){
	for(size_t i=0;i<conts_.size();i++)
		conts_.at(i).addGlobalRelErrorUp(name,relerr);
	gencont_.addGlobalRelErrorUp(name,relerr);
	recocont_.addGlobalRelErrorUp(name,relerr);
	unfolded_.addGlobalRelErrorUp(name,relerr);
	refolded_.addGlobalRelErrorUp(name,relerr);
}

void histo1DUnfold::addGlobalRelErrorDown(TString name,float relerr){
	for(size_t i=0;i<conts_.size();i++)
		conts_.at(i).addGlobalRelErrorDown(name,relerr);
	gencont_.addGlobalRelErrorDown(name,relerr);
	recocont_.addGlobalRelErrorDown(name,relerr);
	unfolded_.addGlobalRelErrorDown(name,relerr);
	refolded_.addGlobalRelErrorDown(name,relerr);
}

void histo1DUnfold::addGlobalRelError(TString name,float relerr){
	for(size_t i=0;i<conts_.size();i++)
		conts_.at(i).addGlobalRelError(name,relerr);
	gencont_.addGlobalRelError(name,relerr);
	recocont_.addGlobalRelError(name,relerr);
	unfolded_.addGlobalRelError(name,relerr);
	refolded_.addGlobalRelError(name,relerr);
}

void histo1DUnfold::getRelSystematicsFrom(const histo1DUnfold & cont){
	if(xbins_ != cont.xbins_ || ybins_ != cont.ybins_){
		std::cout << "container1DUnfold::getRelSystematicsFrom: " << name_ << " and " << cont.name_ << " must have same x and y axis!" << std::endl;
	}
	for(size_t i=0;i<conts_.size();i++)
		conts_.at(i).getRelSystematicsFrom(cont.conts_.at(i));
	gencont_.getRelSystematicsFrom(cont.gencont_);
	recocont_.getRelSystematicsFrom(cont.recocont_);
	unfolded_.getRelSystematicsFrom(cont.unfolded_);
	refolded_.getRelSystematicsFrom(cont.refolded_);
}
void histo1DUnfold::addRelSystematicsFrom(const histo1DUnfold & cont,bool ignorestat,bool strict){
	if(xbins_ != cont.xbins_ || ybins_ != cont.ybins_){
		std::cout << "container1DUnfold::addRelSystematicsFrom: " << name_ << " and " << cont.name_ << " must have same x and y axis!" << std::endl;
	}
	for(size_t i=0;i<conts_.size();i++)
		conts_.at(i).addRelSystematicsFrom(cont.conts_.at(i),ignorestat,strict);
	gencont_.addRelSystematicsFrom(cont.gencont_,ignorestat,strict);
	recocont_.addRelSystematicsFrom(cont.recocont_,ignorestat,strict);
	unfolded_.addRelSystematicsFrom(cont.unfolded_,ignorestat,strict);
	refolded_.addRelSystematicsFrom(cont.refolded_,ignorestat,strict);
}
bool histo1DUnfold::checkCongruentBinBoundariesXY() const{
	if(xbins_.size() <2|| ybins_.size() <2)
		return false; //emtpy
	return checkCongruence(xbins_,ybins_);
}
/**
 * assumes  ybins >= xbins. should be the case for histo1DUnfold
 */
histoBin histo1DUnfold::getDiagonalBin(const size_t & xbin, int layer) const{
	///NEW rewrite for symmetric internal binning (pretty simple)
	if(xbin>=xbins_.size()){
		std::cout << "container1DUnfold::getDiagonalBin: bin out of range" <<std::endl;
		throw std::out_of_range("container1DUnfold::getDiagonalBin: bin out of range");
	}
	if(!congruentbins_){
		std::cout << "container1DUnfold::getDiagonalBin: only works for congruent bin boundaries" <<std::endl;
		return histoBin();
	}
	if(xbin==xbins_.size()-1){ //overflow bin, just one
		return histoBin(getBin(xbin,ybins_.size()-1,layer));
	}

	//add up until reaches bin boundary
	size_t ybin=getBinNoY(xbins_.at(xbin));
	histoBin out(getBin(xbin,ybin,layer));
	for(ybin=getBinNoY(xbins_.at(xbin)+1);ybin<getBinNoY(xbins_.at(xbin+1)) ;ybin++){
		out.addToContent(getBin(xbin,ybin,layer).getContent());
		out.setEntries(out.getEntries() + getBin(xbin,ybin,layer).getEntries());
		out.setStat2(out.getStat2() + getBin(xbin,ybin,layer).getStat2());
	}
	return out;
}

histo1D histo1DUnfold::getDiagonal()const{
	histo1D out=getRecoContainer();
	for(int sys=-1;sys<(int)getSystSize();sys++){
		for(size_t bin=0;bin<out.getBins().size();bin++){
			out.getBin(bin,sys) = getBin(bin,bin,sys);
		}
	}
	return out;
}

bool histo1DUnfold::check(){ ///NEW

	return true;

	if(!binbybin_ && ybins_.size() == xbins_.size()){ // recobins==genbins and no binbybin -> rebin reco
		std::cout << "container1DUnfold::check: warning! rebinning (factor 4) reco part of " <<name_ << std::endl;
		std::vector<float> oldybins=ybins_;
		oldybins.erase(oldybins.begin()); //erase UF bin
		std::vector<float> newybins=subdivide(oldybins,4);
		setBinning(xbins_,newybins);
	}
	return true;
}

histo1D histo1DUnfold::fold(const histo1D& input,bool addbackground) const{
	if(debug) std::cout << "container1DUnfold::fold" <<std::endl;


	//some checks
	if(input.isDummy()){
		throw std::logic_error("container1DUnfold::fold: input is dummy");
	}
	if(isDummy()){
		throw std::logic_error("container1DUnfold::fold: *this is dummy");
	}


	histo2D normresp=getNormResponseMatrix();

	/*take care of syst layers. check whether all exist.
	 * in terms of performance improvements, assume input.layersize<*this.layersize
	 * This does not affect the fact that this function should also work if its
	 * the other way around - just slightly slower
	 */
	histo1D out=projectToY(); //to get syst layers etc right
	histo1D preparedinput=input;

	if(input.getBins() != conts_.at(0).getBins()){
		// throw std::logic_error("histo1DUnfold::fold: input bins and response matrix bins not the same");
		if(input.getNBins() < conts_.at(0).getNBins()){
			//try to rebin
			histo1DUnfold copy=*this;
			//this will only affect the 2d part... not tooo good, but .. who cares
			for(size_t i=0;i<conts_.size();i++){
				copy.conts_.at(i)=copy.conts_.at(i).rebinToBinning(preparedinput);
			}
			normresp=copy.getNormResponseMatrix();
			out.rebinToBinning(preparedinput);
		}
		else{ //input has more bins than output is supposed to have. Rebin preparedinput
			//try to rebin
			preparedinput.rebinToBinning(conts_.at(0));
		}
	}
	out.mergeLayers(preparedinput);

	std::vector<histo1D> & copies=normresp.conts_;



	// prepare new response matrix
	// normalize each column to itself (including UF/OF). This then takes care of efficiencies
	// this step includes ALL systematics


	//do the M x vector  multiplication

	for(size_t row=0;row<copies.size();row++){ //UF (efficiencies) and OF (nothing) excluded
		//get layer asso
		std::map<size_t,size_t> layerasso=copies.at(row).mergeLayers(preparedinput);

		if(debug)
			std::cout << "container1DUnfold::fold: preparedInput layers: " << preparedinput.getSystSize() << std::endl;
		for(int sys=-1;sys<(int)copies.at(row).getSystSize();sys++){
			float sum=0, sumstat2=0,fakeentries=0;

			int inputsys=sys;
			if(sys>-1){
				inputsys=layerasso[(size_t)sys];

				if(debug)
					std::cout << "container1DUnfold::fold: folding for systematic var " <<  copies.at(row).getSystErrorName(sys)
					<< " >> " <<  preparedinput.getSystErrorName(inputsys) << std::endl;
			}
			size_t minbin=0,maxbin=copies.at(row).getBins().size();
			if(true){ //add background later!
				minbin=1;// dont include UF  (visPS migrations + all backgrounds)
				maxbin--;//dont include gen OF (also PS migrations)
			}
			for(size_t bin=minbin;bin<maxbin;bin++){
				sum+= copies.at(row).getBinContent(bin,sys)*preparedinput.getBinContent(bin,inputsys);
				//assume no stat correlation between resp matrix and input
				sumstat2+=copies.at(row).getBin(bin,sys).getStat2() * preparedinput.getBinContent(bin,inputsys) * preparedinput.getBinContent(bin,inputsys);
				sumstat2+=preparedinput.getBin(bin,inputsys).getStat2() * copies.at(row).getBinContent(bin,sys)*copies.at(row).getBinContent(bin,sys);
				fakeentries+=preparedinput.getBinEntries(bin,sys);
			}
			out.getBin(row,sys).setContent(sum);
			out.getBin(row,sys).setStat2(sumstat2);
			out.getBin(row,sys).setEntries(fakeentries);
		}
	}

	if(addbackground)
		out+=getBackground();

	out.setName(input.getName()+"_folded");
	out.setXAxisName(xaxis1Dname_);
	out.setYAxisName(yaxis1Dname_);
	return out;
}

histo1D histo1DUnfold::getPurity() const{
	bool mklist=histo1D::c_makelist;
	histo1D::c_makelist=false;
	if(!congruentbins_){
		histo1D c;
		histo1D::c_makelist=mklist;
		return c;
	}
	//new impl

	//rebin to out binning
	histo1DUnfold rebinned=rebinToBinning(genbins_);
	histo1D rec=rebinned.getVisibleSignal();
	//  histo1D gen=rebinned.getGenContainer();

	histo1D recgen=rebinned.getDiagonal();

	///NEW
	/*
    for(int sys=-1;sys<(int)getSystSize();sys++)
        for(size_t bin=0;bin<recgen.getBins().size();bin++){
            recgen.getBin(bin,sys) = getDiagonalBin(bin,sys);}
	 */

	//assume a "good" response matrix with most elements being diagonal, so assume stat correlation between both
	bool temp=histoContent::divideStatCorrelated;
	histoContent::divideStatCorrelated=true;
	histo1D purity=recgen/rec;
	histoContent::divideStatCorrelated=temp;
	histo1D::c_makelist=mklist;
	purity.setXAxisName(xaxis1Dname_);
	return purity;


}

histo1D histo1DUnfold::getStability(bool includeeff) const{

	bool mklist=histo1D::c_makelist;
	histo1D::c_makelist=false;
	if(!congruentbins_){
		std::cout << "container1DUnfold::getPurity: plotting stability not possible because of non fitting bin boundaries for " << name_ << std::endl;
		histo1D c;
		histo1D::c_makelist=mklist;
		return c;
	}
	histo1DUnfold rebinned=rebinToBinning(genbins_);
	histo1D gen=rebinned.projectToX(includeeff);//UFOF? include "BG"

	histo1D recgen=rebinned.getDiagonal();



	//asume a "good" response matrix with most elements being diagonal, so assume stat correlation between both
	//anyway only affect the error bars
	bool temp=histoContent::divideStatCorrelated;
	histoContent::divideStatCorrelated=true;
	histo1D stability=recgen/gen;
	histoContent::divideStatCorrelated=temp;

	histo1D::c_makelist=mklist;
	stability.setXAxisName(xaxis1Dname_);
	return stability;

}

histo1D histo1DUnfold::getEfficiency()const{
	histo1D xprojection=projectToX(true);
	histo1D xprojectionwouf=projectToX(false);
	bool tmp=histoContent::divideStatCorrelated;
	histoContent::divideStatCorrelated=true;
	xprojectionwouf =xprojectionwouf/ xprojection;
	xprojectionwouf.setYAxisName("#epsilon");
	xprojectionwouf.transformToEfficiency();
	xprojectionwouf.setXAxisName(xaxis1Dname_ +" (gen)");
	histoContent::divideStatCorrelated=tmp;
	return xprojectionwouf;
}

histo2D histo1DUnfold::getResponseMatrix()const{
	histo2D out;

	out.conts_=conts_; //! for each y axis bin one container
	out.xbins_=xbins_;
	out.ybins_=ybins_;
	out.divideBinomial_=divideBinomial_;
	out.mergeufof_=mergeufof_;


	out.xaxisname_=xaxis1Dname_ + " (gen)";
	out.yaxisname_=xaxis1Dname_ + " (reco)";
	out.name_=name_+"_respMatrix";
	return out;
}
histo2D histo1DUnfold::getNormResponseMatrix()const{
	histo2D out;

	out.conts_=conts_; //! for each y axis bin one container
	histo1D xprojection=projectToX(true);
	bool tmp=histoContent::divideStatCorrelated;
	histoContent::divideStatCorrelated=true;
	for(size_t i=0;i<out.conts_.size();i++){
		out.conts_.at(i)/=xprojection;
	}
	histoContent::divideStatCorrelated=tmp;
	out.xbins_=xbins_;
	out.ybins_=ybins_;
	out.divideBinomial_=divideBinomial_;
	out.mergeufof_=mergeufof_;

	out.xaxisname_=xaxis1Dname_ + " (gen)";
	out.yaxisname_=xaxis1Dname_ + " (reco)";
	out.name_=name_+"_normrespMatrix";
	return out;
}



/**
 * TH2D * prepareRespMatrix(bool nominal=true,unsigned int systNumber=0)
 * -prepares matrix and does some checks for nominal or systematic variation
 *
 * uses generator binning given
 */
TH2D * histo1DUnfold::prepareRespMatrix(bool nominal,unsigned int systNumber) const{ //for each syste
	if(getNBinsX() < 1 || getNBinsY() <1)
		return 0;
	if(!nominal && systNumber>=getSystSize()){
		std::cout << "container1DUnfold::prepareRespMatrix: systNumber "<< systNumber << " out of range (" << getSystSize()-1 << ")"<< std::endl;
		return 0;
	}
	/* some consistency checks
	 * check whether everything was filled consistently
	 * if so, underflow+all conts should be the same as gencont.
	 */
	float genint=gencont_.integral(false);
	float allint=0;
	for(size_t i=0;i<conts_.size();i++){
		allint+=conts_.at(i).integral(false);
	}
	if(fabs((genint-allint)/genint) > 0.01){
		std::cout << "container1DUnfold::prepareRespMatrix: something went wrong in the filling process (wrong UF/OF?)  of " << name_ <<  "\n genint: "<< genint
				<< " response matrix int: "<< allint << std::endl;
		//return 0;
	}

	//prepare rebinned containers
	histo2D rebinned=getResponseMatrix().rebinXToBinning(genbins_);
	TH2D * h=0;
#pragma omp critical //protect new TH2D
	{
		if(nominal)
			h= rebinned.getTH2D("respMatrix_nominal",false,true);
		else
			h=rebinned.getTH2DSyst("respMatrix_"+getSystErrorName(systNumber),systNumber,false,true);
	} //critical
	return h;

}



TString histo1DUnfold::coutUnfoldedBinContent(size_t bin,const TString& units) const{
	if(!isBinByBin()){
		std::cout << "container1DUnfold::coutUnfoldedBinContent: only implemented for bin-by-bin unfolded distributions so far, doing nothing" <<std::endl;
		return "";
	}
	if(bin >= unfolded_.getBins().size()){
		std::cout << "container1DUnfold::coutUnfoldedBinContent: bin out of range, doing nothing" << std::endl;
		return "";
	}
	TString out;
	std::cout << "relative background: " <<std::endl;
	out+="relative background: \n";
	histo1D treco=getRecoContainer();
	bool temp=histoContent::divideStatCorrelated;
	histoContent::divideStatCorrelated=false;
	out+=(getBackground()/treco).coutBinContent(bin,units);
	histoContent::divideStatCorrelated=temp;
	std::cout << "unfolded: " <<std::endl;
	out+="unfolded: \n";
	out+=getUnfolded().coutBinContent(bin,units);
	return out;
}

///////////////

void histo1DUnfold::flushAllListed(){
	for(size_t i=0;i<histo1DUnfold::c_list.size();i++)
		histo1DUnfold::c_list.at(i)->flush();
}

void histo1DUnfold::setAllListedMC(bool ISMC){
	for(size_t i=0;i<histo1DUnfold::c_list.size();i++)
		histo1DUnfold::c_list.at(i)->setMC(ISMC);
}

void histo1DUnfold::setAllListedLumi(float lumi){
	for(size_t i=0;i<histo1DUnfold::c_list.size();i++)
		histo1DUnfold::c_list.at(i)->setLumi(lumi);
}
void histo1DUnfold::checkAllListed(){
	for(size_t i=0;i<histo1DUnfold::c_list.size();i++)
		histo1DUnfold::c_list.at(i)->check();
}

std::vector<float> histo1DUnfold::subdivide(const std::vector<float> & bins, size_t div){
	std::vector<float> out;
	for(size_t i=0;i<bins.size()-1;i++){
		float width=bins.at(i+1)-bins.at(i);
		if((int)i<(int)bins.size()-2){
			for(size_t j=0;j<div;j++)
				out.push_back(bins.at(i)+j*width/div);
		}
		else{
			for(size_t j=0;j<=div;j++)
				out.push_back(bins.at(i)+j*width/div);
		}
	}
	return out;
}
/**
 * b needs to be larger than a
 */
bool histo1DUnfold::checkCongruence(const std::vector<float>& a, const std::vector<float>& b) const{
	std::vector<float> sames(b.size()); //always larger than xbins
	std::vector<float>::iterator it=std::set_intersection(a.begin(),a.end(),b.begin(),b.end(),sames.begin());
	sames.resize(it-sames.begin());
	return sames.size() == a.size();
}


////////////////////////////////////////////////FILLING////////////////////////////////////////////////

void histo1DUnfold::flush(){ //only for MC
	if(!flushed_){
		if(isDummy())
			return;

		if(genfill_)
			gencont_.fill(tempgen_,tempgenweight_);
		if(recofill_)
			recocont_.fill(tempreco_,tempweight_);

		if(genfill_ && !recofill_){ //put in Reco UF bins

			fill(tempgen_,ybins_[1]-100,tempgenweight_);
		}
		else if(recofill_ && !genfill_){ //put in gen underflow bins -> goes to background

			if(mergeufof_){
				size_t binnoy=getBinNoY(tempreco_);
				if(binnoy == 0)
					fill(xbins_[1]-100,ybins_.at(1),tempweight_);//force first vis bin
				else if(binnoy == ybins_.size()-1)
					fill(xbins_[1]-100,ybins_.size()-2,tempweight_);//force last vis bin
			}
			else{
				fill(xbins_[1]-100,tempreco_,tempweight_);
			}
		}
		else if(genfill_ && recofill_){
			if(mergeufof_){
				size_t binnoy=getBinNoY(tempreco_);
				if(binnoy == 0)
					fill(tempgen_,ybins_.at(1),tempweight_);//force first vis bin
				else if(binnoy == ybins_.size()-1)
					fill(tempgen_,ybins_.size()-2,tempweight_);//force last vis bin
			}
			else{
				fill(tempgen_,tempreco_,tempweight_);
			}
			fill(tempgen_,ybins_[1]-100,(tempgenweight_-tempweight_)); // w_gen * (1 - recoweight), tempweight_=fullweight=tempgenweight_*recoweight
		}
	}
	recofill_=false;
	genfill_=false;
	flushed_=true;

}


void histo1DUnfold::fillGen(const float & val, const float & weight){
	if(genfill_){
		throw std::logic_error("container1DUnfold::fillGen: Attempt to fill gen (twice) without flushing");
	}
	tempgen_=val;
	tempgenweight_=weight;
	genfill_=true;
	flushed_=false;
}

void histo1DUnfold::fillReco(const float & val, const float & weight){ //fills and resets tempgen_
	if(recofill_ && !allowmultirecofill_){
		throw std::logic_error("container1DUnfold::fillReco: Attempt to fill reco (twice) without flushing");
	}
	tempreco_=val;
	tempweight_=weight;
	recofill_=true;
	flushed_=false;
	if(allowmultirecofill_){
		if(genfill_)
			gencont_.fill(tempgen_,tempgenweight_);
		if(recofill_)
			recocont_.fill(tempreco_,tempweight_);

		if(genfill_ && !recofill_){ //put in Reco UF bins
			fill(tempgen_,ybins_[1]-100,tempgenweight_);}
		else if(recofill_ && !genfill_){ //put in gen underflow bins -> goes to background
			fill(xbins_[1]-100,tempreco_,tempweight_);}
		else if(genfill_ && recofill_){
			fill(tempgen_,tempreco_,tempweight_);
			fill(tempgen_,ybins_[1]-100,(tempgenweight_-tempweight_)); // w_gen * (1 - recoweight), tempweight_=fullweight=tempgenweight_*recoweight
		}

		//recofill_=false;
		flushed_=true;
	}

}




//////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////IO////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void histo1DUnfold::loadFromTree(TTree *t, const TString & plotname){
	if(!t || t->IsZombie()){
		throw std::runtime_error("container1DUnfold::loadFromTree: tree not ok");
	}
	ztop::histo1DUnfold * cuftemp=0;
	if(!t->GetBranch("container1DUnfolds")){
		throw std::runtime_error("container1DUnfold::loadFromTree: branch container1DUnfolds not found");
	}
	bool found=false;
	size_t count=0;

	t->SetBranchAddress("container1DUnfolds", &cuftemp);
	for(float n=0;n<t->GetEntries();n++){
		t->GetEntry(n);
		if(cuftemp->getName()==(plotname)){
			found=true;
			count++;
			*this=*cuftemp;
		}
	}

	if(!found){
		std::cout << "searching for: " << plotname << "... error!" << std::endl;
		throw std::runtime_error("container1DUnfold::loadFromTree: no container with name not found");
	}
	if(count>1){
		std::cout << "container1DUnfold::loadFromTree: found more than one object with name "
				<< getName() << ", took the first one." << std::endl;
	}
}
void histo1DUnfold::loadFromTFile(TFile *f, const TString & plotname){
	if(!f || f->IsZombie()){
		throw std::runtime_error("container1DUnfold::loadFromTFile: file not ok");
	}
	AutoLibraryLoader::enable();
	TTree * ttemp = (TTree*)f->Get("container1DUnfolds");
	loadFromTree(ttemp,plotname);
	delete ttemp;
}
void histo1DUnfold::loadFromTFile(const TString& filename,
		const TString & plotname){
	AutoLibraryLoader::enable();
	TFile * ftemp=new TFile(filename,"read");
	loadFromTFile(ftemp,plotname);
	delete ftemp;
}

void histo1DUnfold::writeToTree(TTree *t){
	if(!t || t->IsZombie()){
		throw std::runtime_error("container1DUnfold::writeToTree: tree not ok");
	}
	ztop::histo1DUnfold * cufpointer=this;
	TBranch *b=0;
	if(t->GetBranch("container1DUnfolds")){
		t->SetBranchAddress("container1DUnfolds", &cufpointer, &b);
	}
	else{
		b=t->Branch("container1DUnfolds",&cufpointer);
	}

	t->Fill();
	t->Write(t->GetName(),TObject::kOverwrite);
}
void histo1DUnfold::writeToTFile(TFile *f){
	if(!f || f->IsZombie()){
		throw std::runtime_error("container1DUnfold::loadFromTFile: file not ok");
	}
	f->cd();
	TTree * ttemp = (TTree*)f->Get("container1DUnfolds");
	if(!ttemp || ttemp->IsZombie())//create
		ttemp = new TTree("container1DUnfolds","container1DUnfolds");
	writeToTree(ttemp);
	delete ttemp;
}
void histo1DUnfold::writeToTFile(const TString& filename){
	TFile * ftemp=new TFile(filename,"update");
	if(!ftemp || ftemp->IsZombie()){
		delete ftemp;
		ftemp=new TFile(filename,"create");
	}
	writeToTFile(ftemp);
	delete ftemp;
}

bool histo1DUnfold::TFileContainsContainer1DUnfolds(TFile *f){
	if(!f || f->IsZombie()){
		throw std::runtime_error("container1DUnfold::TFileContainsContainer1DUnfolds: file not ok");
	}
	TTree * t = (TTree*)f->Get("container1DUnfolds");
	if(!t || t->IsZombie()){
		if(t) delete t;
		return false;
	}
	if(t->GetEntries()<1){
		delete t;
		return false;
	}
	delete t;
	return true;

}
bool histo1DUnfold::TFileContainsContainer1DUnfolds(const TString & filename){
	TFile *  f = new TFile(filename, "READ");
	return TFileContainsContainer1DUnfolds(f);
}


void histo1DUnfold::writeToFile(const std::string& filename)const{
	std::ofstream saveFile;
	saveFile.open(filename.data(), std::ios_base::binary | std::ios_base::trunc | std::fstream::out );
	{
		boost::iostreams::filtering_ostream out;
		boost::iostreams::zlib_params parms;
		//parms.level=boost::iostreams::zlib::best_speed;
		out.push(boost::iostreams::zlib_compressor(parms));
		out.push(saveFile);
		{
			writeToStream(out);
		}
	}
	saveFile.close();
}
void histo1DUnfold::readFromFile(const std::string& filename){
	std::ifstream saveFile;
	saveFile.open(filename.data(), std::ios_base::binary | std::fstream::in );
	{
		boost::iostreams::filtering_istream in;
		boost::iostreams::zlib_params parms;
		//parms.level=boost::iostreams::zlib::best_speed;
		in.push(boost::iostreams::zlib_decompressor(parms));
		in.push(saveFile);
		{
			readFromStream(in);
		}
	}
	saveFile.close();


}


}


