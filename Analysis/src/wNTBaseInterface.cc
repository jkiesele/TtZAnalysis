/*
 * wNTBaseInterface.cc
 *
 *  Created on: 25 May 2016
 *      Author: kiesej
 */




/*
 * flatBranchesWrapper.cc
 *
 *  Created on: 23 May 2016
 *      Author: kiesej
 */


#include "../interface/wNTBaseInterface.h"

namespace ztop{

bool wNTBaseInterface::debug=false;

wNTBaseInterface::wNTBaseInterface():read_(false),firstread_(false),enabled_(false),t_(0){

}
wNTBaseInterface::wNTBaseInterface(tTreeHandler * t, bool en):read_(false),firstread_(false),enabled_(en),t_(t){


}


wNTBaseInterface::~wNTBaseInterface(){
	for(size_t i=0;i<associatedBranches_.size();i++)
		if(associatedBranches_.at(i)){
			if(debug)
				std::cout << "wNTBaseInterface::~wNTBaseInterface: deleting branch" <<std::endl;
			delete associatedBranches_.at(i);
			if(debug)
				std::cout << "wNTBaseInterface::~wNTBaseInterface: branch deleted" <<std::endl;
			associatedBranches_.at(i)=0;
		}
}


void wNTBaseInterface::disable(){
	enabled_=false;
	for(size_t i=0;i<associatedBranches_.size();i++)
		if(associatedBranches_.at(i)){
			delete associatedBranches_.at(i);
			associatedBranches_.at(i)=0;
		}
	associatedBranches_.clear();
}

bool wNTBaseInterface::isNewEntry()const{
	if(!enabled_)
		return false; //never new if disabled
	if(associatedBranches_.size()<1)
		throw std::logic_error("flatBranchesWrapper: can't read entry with no branches associated");
	for(size_t i=0;i<associatedBranches_.size();i++)
		if(associatedBranches_.at(i)){
			if(associatedBranches_.at(i)->gotEntry())
				return false;
			else
				return true;
		}
	return true;
}

size_t wNTBaseInterface::getBranchBuffer(const size_t & idx)const{
	if(idx>associatedBranches_.size())
		throw std::out_of_range("wNTBaseInterface::getBranchBuffer: index out of range");

	return associatedBranches_.at(idx)->getBufMax();
}

}
