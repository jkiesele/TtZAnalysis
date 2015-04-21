/*
 * tTreeHandler.cc
 *
 *  Created on: Apr 20, 2015
 *      Author: kiesej
 */




#include "../interface/tTreeHandler.h"
#include "../interface/tBranchHandler.h"
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ztop{
tTreeHandler::tTreeHandler(const TString & filename, const TString &treename):file_(0), t_(0),entry_(0),entries_(0){
	load(filename,treename);
}
tTreeHandler::tTreeHandler(): file_(0), t_(0),entry_(0),entries_(0){

}
tTreeHandler::~tTreeHandler(){
	clear();
}

void tTreeHandler::load(const TString & filename, const TString &treename){
	clear();
	std::ifstream FileTest(filename.Data());
	bool exists = FileTest;
	FileTest.close();
	if(!exists){
		std::string errstr="tTreeHandler::load: file does not exist: ";
		errstr+=filename.Data();
		throw std::runtime_error(errstr);
	}
	TFile* f=new TFile(filename,"READ");
	if(!f || f->IsZombie()){
		throw std::runtime_error("tTreeHandler::load: file not ok.");
	}
	TTree * t = (TTree*)f->Get(treename);
	if(!t || t->IsZombie()){
		throw std::runtime_error("tTreeHandler::load: tree not ok");
	}
	//all ok
	t_=t;
	file_=f;

	entries_=t_->GetEntries();
}
void tTreeHandler::clear(){

	if(t_)
		delete t_;
	t_=0;
	if(file_){
		file_->Close();
		delete file_;
		file_=0;
	}
	assobranches_.clear(); //just remove asso
}
void tTreeHandler::associate( tBranchHandlerBase*tb){
	assobranches_.push_back(tb);
}
void tTreeHandler::removeAsso( tBranchHandlerBase*tb){
	if(tb){
		for(size_t i=0;i<assobranches_.size();i++){
			if(assobranches_.at(i) == tb){
				std::vector<tBranchHandlerBase*>::iterator it=assobranches_.begin()+i;
				assobranches_.erase(it);
			}
		}
	}
}

void tTreeHandler::setEntry(const Long64_t& in){

	entry_ = in;
	for(size_t i=0;i<assobranches_.size();i++)
		assobranches_.at(i)->newEntry();
	tree()->LoadTree(entry_);
}
void tTreeHandler::setPreCache(){
	struct stat filestatus;
	stat(file_->GetPath(), &filestatus );

	tree()->SetCacheSize(filestatus.st_size/10);
	if(tree()->GetCacheSize() > 100e6)
		tree()->SetCacheSize(100e6);
	tree()->SetCacheLearnEntries(1000);

}

}