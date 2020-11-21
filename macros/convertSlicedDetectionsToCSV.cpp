#include<assert.h>
#include <iostream>
#include <TFile.h>
#include <TParameter.h>
#include <TSystem.h>
#include "utils.cpp"

using namespace std;

void produceSingleExport(TString inFilepath, int nInnerSlices, int nOuterSlices, bool writeOnlyNZeroDep){
    // files
    TFile fileIn(inFilepath);
    TString outFileName(TString(inFilepath(0, inFilepath.Length()-5)) + ".csv");
    // tree and params
    TTree* fTree = (TTree*) fileIn.Get("fTree");
    if(nInnerSlices<0 || nOuterSlices<0) {
        // if not given as input (input<0), they will be inferred by the root file
        TParameter<Int_t> *NInnerSlicesParam = (TParameter<Int_t> *) fileIn.Get("NInnerSlices");
        TParameter<Int_t> *NOuterSlicesParam = (TParameter<Int_t> *) fileIn.Get("NOuterSlices");
        nInnerSlices = NInnerSlicesParam->GetVal();
        nOuterSlices = NOuterSlicesParam->GetVal();
    }
    // Branches
    Double_t x, y, z, r, time, energydeposition, detectionefficiency, quantumefficiency;
    Int_t eventnumber, PID, pedetected;
    string * material = 0;
    vector<Int_t> innerSlices, outerSlices;
    for(int j=0; j<nInnerSlices; j++)
        innerSlices.push_back(0);
    for(int j=0; j<nOuterSlices; j++)
        outerSlices.push_back(0);
    // Connect branches
    fTree->SetBranchAddress("eventnumber", &eventnumber);
    fTree->SetBranchAddress("PID", &PID);
    fTree->SetBranchAddress("time", &time);
    fTree->SetBranchAddress("x", &x);
    fTree->SetBranchAddress("y", &y);
    fTree->SetBranchAddress("z", &z);
    fTree->SetBranchAddress("r", &r);
    fTree->SetBranchAddress("material", &material);
    fTree->SetBranchAddress("energydeposition", &energydeposition);
    fTree->SetBranchAddress("pedetected", &pedetected);
    fTree->SetBranchAddress("detectionefficiency", &detectionefficiency);
    fTree->SetBranchAddress("quantumefficiency", &quantumefficiency);
    for(int j=0; j<nInnerSlices; j++){
        TString branchName = "InnerSlice";
        branchName += j;
        fTree->SetBranchAddress(branchName, &innerSlices[j]);
    }
    for(int j=0; j<nOuterSlices; j++){
        TString branchName = "OuterSlice";
        branchName += j;
        fTree->SetBranchAddress(branchName, &outerSlices[j]);
    }
    // Loop over entries
    std::ofstream out;
    for(int i=0; i<fTree->GetEntries(); i++){
        fTree->GetEntry(i);
        // Check if we have a new event and reached the max num in curr file
        if(i == 0){
            cout << "\t[Info] Creating " << outFileName << endl;
            if(i > 0)    out.close();
            out.open(outFileName);
            //Print header
            out << "eventnumber,PID,time,x,y,z,r,material,";
            out << "energydeposition,pedetected,detectionefficiency,quantumefficiency,";
            for(int j=0; j<nInnerSlices; j++)
                out << "InnerSlice" << j << ",";
            for(int j=0; j<nOuterSlices; j++)
                out << "OuterSlice" << j << ",";
            out << "\n";
        }
        if(i % (fTree->GetEntries()/10) == 0)
            cout << i << "/" << fTree->GetEntries() << endl;
        if(!writeOnlyNZeroDep || energydeposition>0){
            out << eventnumber << "," << PID << "," << time << "," << x << "," << y << "," << z << "," << r << ",";
            out << *material << "," << setprecision(15) << energydeposition << "," << pedetected << ",";
            out << detectionefficiency << "," << quantumefficiency << ",";
            for(int j=0; j<nInnerSlices; j++)
                out << innerSlices[j] <<  ",";
            for(int j=0; j<nOuterSlices; j++)
                out << outerSlices[j] <<  ",";
            out << "\n";
        }
    }
}

void convertSlicedDetectionsToCSV(const char * dirIn="./", const char * prefixIn="SlicedDetections",
                                  int nInnerSlices=-1, int nOuterSlices=-1, bool writeOnlyNZeroDep=true){
    // Loop files in input directory
    char* fullDirIn = gSystem->ExpandPathName(dirIn);
    void* dirp = gSystem->OpenDirectory(fullDirIn);
    const char* entry;
    while((entry = (char*)gSystem->GetDirEntry(dirp))) {
        TString fileName = entry;
        if(!isRootFile(fileName, prefixIn))   continue;
        cout << "\t[Info] Loading " << fullDirIn + fileName << endl;   // Debug
        produceSingleExport(fullDirIn + fileName, nInnerSlices, nOuterSlices, writeOnlyNZeroDep);
    }
}