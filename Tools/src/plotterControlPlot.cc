/*
 * plotterControlPlot.cc
 *
 *  Created on: Jan 15, 2014
 *      Author: kiesej
 */



#include "../interface/plotterControlPlot.h"
#include "../interface/containerStack.h"
#include "../interface/fileReader.h"
#include "../interface/plot.h"

namespace ztop{

/**
 * expects entries:
 * [plotterControlPlot] defines divideat
 * [textBoxes - boxes]
 * [containerStyle - DataUpper]
 * [containerStyle - MCUpper]
 * [containerStyle - DataRatio]
 * [containerStyle - MCRatio]
 * [plotStyle - Upper]
 * [plotStyle - Ratio]
 */
void plotterControlPlot::readStyleFromFile(const std::string& infile){


    fileReader fr;
    fr.setComment("$");
    fr.setDelimiter(",");
    fr.setStartMarker("[plotterControlPlot]");
    fr.setEndMarker("[end plotterControlPlot]");
    fr.readFile(infile);
    if(fr.nLines()<1){
        throw std::runtime_error("plotterControlPlot::readStyleFromFile: no [plotterControlPlot] found");
    }

    divideat_  = fr.getValue<float>("divideat");
    invertplots_  = fr.getValue<bool>("invertplots");
    if(debug) std::cout <<"plotterControlPlot::readStyleFromFile" << std::endl;

    upperstyle_.readFromFile(infile, "Upper");
    ratiostyle_.readFromFile(infile, "Ratio");
    datastyleupper_.readFromFile(infile, "DataUpper");
    datastyleratio_.readFromFile(infile, "DataRatio");
    mcstyleupper_.readFromFile(infile, "MCUpper");
    mcstyleratio_.readFromFile(infile, "MCRatio");
    //FIXME
    mcstylepsmig_=mcstyleupper_;
    mcstylepsmig_.readFromFile(infile, "MCUpperMigrations",false);
    //text boxes

    textboxes_.readFromFile(infile,"boxes");

}

/**
 * clears plots, clears styles
 */
void plotterControlPlot::clear(){
    for(size_t i=0;i<tempplots_.size();i++){
        if(tempplots_.at(i)) delete tempplots_.at(i);
    }
    tempplots_.clear();
}
void plotterControlPlot::clearPlots(){
    for(size_t i=0;i<tempplots_.size();i++){
        if(tempplots_.at(i)) delete tempplots_.at(i);
    }
    tempplots_.clear();
}

void plotterControlPlot::cleanMem(){
    plotterBase::cleanMem();
    for(size_t i=0;i<tempplots_.size();i++){
        if(tempplots_.at(i)) delete tempplots_.at(i);
    }
    tempplots_.clear();
}

///plotting

void plotterControlPlot::preparePad(){
    if(debug) std::cout <<"plotterControlPlot::preparePad" << std::endl;
    if(!stackp_){
        throw std::logic_error("plotterControlPlot::preparePad: containerStack not set");
    }
    if(!stackp_->checkDrawDimension()){
        if(debug) std::cout << "plotterControlPlot::draw: only available for 1dim stacks!" <<std::endl;
        return;
    }
    cleanMem();
    TVirtualPad * c = getPad();
    c->Clear();
    c->Divide(1,2);
    c->cd(1)->SetPad(0,divideat_,1,1);
    c->cd(2)->SetPad(0,0,1,divideat_);
    upperstyle_.applyPadStyle(c->cd(1));
    ratiostyle_.applyPadStyle(c->cd(2));
}
void plotterControlPlot::drawPlots(){
    TVirtualPad * c = getPad();
    c->cd(1);
    drawControlPlot();
    c->cd(2);
    drawRatioPlot();

}
// void drawTextBoxes(); // by base class
void plotterControlPlot::drawLegends(){
    if(debug) std::cout <<"plotterCompare::drawLegends" << std::endl;
    getPad()->cd(1);
    legstyle_.applyLegendStyle(templegp_);
    templegp_->Draw("same");
    //use templegps_

    /* ... */
}
/**
 * returns ordered pointers to objects to add to lagends
 */
void plotterControlPlot::drawControlPlot(){
    if(debug) std::cout << "plotterControlPlot::drawControlPlot " << stackp_->getName()<<std::endl;
    getPad()->cd(1);
    TH1::AddDirectory(false);
    //make axis histo
    //draw axis
    size_t dataentry= std::find(stackp_->legends_.begin(),stackp_->legends_.end(),stackp_->dataleg_) - stackp_->legends_.begin();

    if(debug)std::cout <<  "found dataentry at position "<< dataentry<< " of " <<  stackp_->size()-1<< std::endl;
    tempdataentry_=dataentry;
    if(dataentry == stackp_->size()){
        std::cout <<  "plotterControlPlot::drawControlPlot: no data entry found for " << stackp_->getName() <<std::endl;
        throw std::runtime_error("plotterControlPlot::drawControlPlot: no data entry found");
    }
    bool divbbw=true;
    if(stackp_->hasTag(taggedObject::dontDivByBW_tag)) divbbw=false;
    TH1 * axish=addObject(stackp_->getContainer(dataentry).getTH1D("",divbbw,false,false));
    plotStyle upperstyle=upperstyle_;
    upperstyle.absorbYScaling(getSubPadYScale(1));
    upperstyle.applyAxisStyle(axish);
    axish->Draw("AXIS");


    //prepare data
    plot* dataplottemp =  new plot(&stackp_->getContainer(dataentry),divbbw);
    tempplots_.push_back(dataplottemp);
    datastyleupper_.applyContainerStyle(dataplottemp);

    //set up legend here....
    templegp_=addObject(new TLegend((Double_t)0.65,(Double_t)0.50,(Double_t)0.95,(Double_t)0.90));
    templegp_->Clear();
    templegp_->SetFillStyle(0);
    templegp_->SetBorderSize(0);

    templegp_->AddEntry(dataplottemp->getSystGraph(),stackp_->getLegend(dataentry),"ep");

    std::vector<size_t> signalidxs=stackp_->getSignalIdxs();


    if(stackp_->size()>1){ //its not only data

        std::vector<TObject *> sortedout;
        std::vector<size_t> sorted=stackp_->getSortedIdxs(invertplots_); //invert
        std::vector<size_t> nisorted=stackp_->getSortedIdxs(!invertplots_); //invert
        std::vector<TH1 *> stackedhistos;

        container1D sumcont=stackp_->getContainer(dataentry);
        sumcont.clear();
        bool tmpaddStatCorrelated=histoContent::addStatCorrelated;
        histoContent::addStatCorrelated=false;
        size_t firstmccount=0;
        if(dataentry==0) firstmccount++;

        std::vector<TString> legendentries;
        bool foundPSmig=false;

        for(size_t it=0;it<stackp_->size();it++){ //it is the right ordering
            size_t i=sorted.at(it);
            if(i != dataentry){
                container1D tempcont;
                if(stackp_->is1DUnfold()){ //special treatment
                    tempcont = stackp_->getContainer1DUnfold(i).getBackground();
                    tempcont *=stackp_->getNorm(i);
                    sumcont+=tempcont;
                    //is not signal


                    if(std::find(signalidxs.begin(),signalidxs.end(),i)==signalidxs.end()){//is not signal
                        TH1D * h=addObject(sumcont.getTH1D(stackp_->getLegend(i)+" "+stackp_->getName()+"_stack_h",divbbw,true,true)); //no errors
                        if(!h)
                            continue;
                        mcstyleupper_.applyContainerStyle(h,false);
                        h->SetFillColor(stackp_->colors_.at(i));
                        stackedhistos.push_back(h);
                        legendentries.push_back(stackp_->getLegend(i));
                    }
                    else{//this is signal but PS migrations!
                        TH1D * h=addObject(sumcont.getTH1D(stackp_->getLegend(i)+" PSmig "+stackp_->getName()+"_stack_h",divbbw,true,true)); //no errors
                        if(!h)
                            continue;
                        mcstylepsmig_.applyContainerStyle(h,false);
                        h->SetFillColor(1);//stackp_->colors_.at(i)+5);
                        stackedhistos.push_back(h);
                        legendentries.push_back("");
                        if(tempcont.integral(true)!=0)
                            foundPSmig=true;
                        tempcont = stackp_->getContainer1DUnfold(i).getVisibleSignal();
                        sumcont+=tempcont;
                        TH1D * hsig=addObject(sumcont.getTH1D(stackp_->getLegend(i)+" "+stackp_->getName()+"_stack_h",divbbw,true,true));
                        mcstyleupper_.applyContainerStyle(hsig,false);
                        hsig->SetFillColor(stackp_->colors_.at(i));
                        stackedhistos.push_back(hsig);
                        legendentries.push_back(stackp_->getLegend(i));

                    }



                }
                else{
                    tempcont = stackp_->getContainer(i);
                    tempcont *=stackp_->getNorm(i);
                    sumcont+=tempcont;
                    TH1D * h=addObject(sumcont.getTH1D(stackp_->getLegend(i)+" "+stackp_->getName()+"_stack_h",divbbw,true,true)); //no errors
                    if(!h)
                        continue;
                    mcstyleupper_.applyContainerStyle(h,false);
                    h->SetFillColor(stackp_->colors_.at(i));
                    stackedhistos.push_back(h);
                    legendentries.push_back(stackp_->getLegend(i));
                }


            }
            else{
                stackedhistos.push_back(0);
            }
        }
        //draw in inverse order


        for(size_t i=stackedhistos.size()-1; i+1>0;i--){

            if(stackedhistos.at(i)){ //not data

                stackedhistos.at(i)->Draw(mcstyleupper_.rootDrawOpt+"same");
                if(legendentries.at(i)!="")
                    templegp_->AddEntry(stackedhistos.at(i),legendentries.at(i),"f");
            }

        }
        //make errors (use sumcont)
        TG * mcerr=addObject(sumcont.getTGraph(stackp_->getName()+"mcerr_cp",divbbw,false,false,false));
        mcstyleupper_.applyContainerStyle(mcerr,true);

        templegp_->AddEntry(mcerr,"MC syst+stat","f");
        if(stackp_->is1DUnfold() && foundPSmig){ //
            TH1D * dummy=addObject(new TH1D());
            mcstylepsmig_.applyContainerStyle(dummy,false);
            dummy->SetFillColor(1);
            templegp_->AddEntry(dummy,"PS migrations","f");
        }
        mcerr->Draw("same"+mcstyleupper_.sysRootDrawOpt);

        histoContent::addStatCorrelated=tmpaddStatCorrelated;
    }
    //plot data now
    dataplottemp->getSystGraph()->Draw(datastyleupper_.sysRootDrawOpt+"same");
    dataplottemp->getStatGraph()->Draw(datastyleupper_.rootDrawOpt+"same");

    getPad()->cd(1)->RedrawAxis();


}
void plotterControlPlot::drawRatioPlot(){
    getPad()->cd(2);
    if(stackp_->size() < 2)
        return;
    container1D fullmc=stackp_->getFullMCContainer(); //dont div by bw
    fullmc=fullmc.getRelErrorsContainer();

    plot * mcerr=new plot(&fullmc,false);
    tempplots_.push_back(mcerr);

    TH1* axish=addObject(stackp_->getContainer(tempdataentry_).convertToGraph(false).getAxisTH1(false,true));
    plotStyle ratiostyle=ratiostyle_;
    ratiostyle.absorbYScaling(getSubPadYScale(2));
    ratiostyle.applyAxisStyle(axish);
    axish->Draw("AXIS");

    mcstyleratio_.applyContainerStyle(mcerr);
    TG* g =mcerr->getStatGraph();
    TG* gs=mcerr->getSystGraph();

    gs->Draw("same"+mcstyleratio_.sysRootDrawOpt);
    g->Draw("same"+mcstyleratio_.rootDrawOpt);

    ///data
    container1D datac=stackp_->getContainer(tempdataentry_);

    datac.normalizeToContainer(stackp_->getFullMCContainer());

    plot * datar=new plot(&datac,false);
    tempplots_.push_back(datar);
    datar->removeXErrors();
    datastyleratio_.applyContainerStyle(datar);
    g =datar->getStatGraph();
    gs=datar->getSystGraph();

    gs->Draw("same"+datastyleratio_.sysRootDrawOpt);
    g->Draw("same"+datastyleratio_.rootDrawOpt);


    TLine * l = addObject(new TLine(datac.getXMin(),1,datac.getXMax(),1));
    l->Draw("same");
    getPad()->cd(2)->RedrawAxis();

}


}
