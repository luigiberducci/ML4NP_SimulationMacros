#include <set>
#include <TString.h>
#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <TSystem.h>
#include "utils.cpp"

using namespace std;

struct selection {
    double minGeDep;
    double maxGeDep;
    double minLArDep;
    double maxLArDep;
    int minGeMultiplicity;
    int maxGeMultiplicity;

    selection(double minGeE=-1, double maxGeE=-1, double minLArE=-1, double maxLArE=-1, int minGeMult=-1, int maxGeMult=-1){
        minGeDep = minGeE;
        maxGeDep = maxGeE;
        minLArDep = minLArE;
        maxLArDep = maxLArE;
        minGeMultiplicity = minGeMult;
        maxGeMultiplicity = maxGeMult;
    }
    template<typename Number>
    bool checkIfInRange(Number value, Number min, Number max){
        bool lowerbound = (min<0) || (value>=min);
        bool upperbound = (max<0) || (value<=max);    
        return lowerbound && upperbound;
    }
    bool check(double totalGeEnergy, double totalArgonEnergy, int geMultiplicity){
        return checkIfInRange(totalGeEnergy, minGeDep, maxGeDep) &
               checkIfInRange(geMultiplicity, minGeMultiplicity, maxGeMultiplicity) &
               checkIfInRange(totalArgonEnergy, minLArDep, maxLArDep);
    }

};

struct io {
    const char* dirIn;
    const char* dirOut;
    const char* inPrefix;
    const char* outPrefix;
    const char* treeName;

    io(const char *dirInput=".", const char *dirOutput=".", const char * inputPrefix="", const char * outputPrefix="", const char * tName="fTree"){
        dirIn = dirInput;
        dirOut = dirOutput;
        inPrefix = inputPrefix;
        outPrefix = outputPrefix;
        treeName = tName;
    }
};

TString make_output_filename(const char * file, io confIO, selection confSelect){
    TString gePref = Form("GeDep_%.1f_%.1f", confSelect.minGeDep, confSelect.maxGeDep);
    TString larPref = Form("LArDep_%.1f_%.1f", confSelect.minLArDep, confSelect.maxLArDep);
    TString multPref = Form("GeMult_%d_%d", confSelect.minGeMultiplicity, confSelect.maxGeMultiplicity);
    TString filename = Form("%s/%s_%s_%s_%s_%s", confIO.dirOut, confIO.outPrefix, gePref.Data(), larPref.Data(), multPref.Data(), file);
    return filename;
}

int processSingleFileGeCoincidence(const char * file, selection confSelect, io confIO, int kIterativeCalls=0){
    // load input file    
    TFile *input = new TFile(Form("%s/%s", confIO.dirIn, file),"READ");
    TTree *fTree = (TTree*) input->Get(confIO.treeName);
    int entries = fTree->GetEntries();
    cout << kIterativeCalls << " - File: " << file << "\n";
    cout << "\tEntries: " << entries << "\n";
    // initialize branch variables
    double energydeposition = 0;
    int pedetected = 0;
    int eventnumber = 0;
    int detectornumber = 0;
    int storedeventnumber = -1;
    double totalArgonEnergy = 0;
    double totalGeEnergy = 0;
    set<int> hitGeDetectors;
    string *material = NULL;
    // connect branches
    fTree->SetBranchAddress("energydeposition", &energydeposition);
    fTree->SetBranchAddress("eventnumber", &eventnumber);
    fTree->SetBranchAddress("material", &material);
    if(confSelect.minGeMultiplicity>=0 & confSelect.maxGeMultiplicity>=0) {
        fTree->SetBranchAddress("detectornumber", &detectornumber);
    }
    // load first entry to store first event number
    fTree->GetEntry(0);
    storedeventnumber = eventnumber;
    // initialize set of events
    set<int> eventnumbers;
    for (int i=0;i<entries;i++){
        fTree->GetEntry(i);
        if(eventnumber!=storedeventnumber){
            //Event is complete, begin energy summing of another event
            if(confSelect.check(totalGeEnergy, totalArgonEnergy, (int) hitGeDetectors.size())){
                cout << "\tEvent: " << storedeventnumber << ": GeEnergy: " << totalGeEnergy << " KeV ";
                cout << "(mult=" << hitGeDetectors.size() << "),";
                cout << "ArgonEnergy: " << totalArgonEnergy << " KeV\n";
                eventnumbers.insert(storedeventnumber);
            }
            // Reset energies counter
            totalGeEnergy = 0;
            totalArgonEnergy = 0;
            hitGeDetectors.clear();
            storedeventnumber = eventnumber;
        }

        if(strstr(material->c_str(),"ArgonLiquid")) {
            totalArgonEnergy += energydeposition;
        }
        if(strstr(material->c_str(),"GermaniumEnriched")) {
            totalGeEnergy += energydeposition;
            hitGeDetectors.insert(detectornumber);
        }
    }
    // check last event
    if(confSelect.check(totalGeEnergy, totalArgonEnergy, (int) hitGeDetectors.size())){
        cout << "\tEvent: " << storedeventnumber << ": GeEnergy: " << totalGeEnergy << " KeV ";
        cout << "(mult=" << hitGeDetectors.size() << "),";
        cout << "ArgonEnergy: " << totalArgonEnergy << " KeV\n";
        eventnumbers.insert(storedeventnumber);
    }
    // export critical events to separate output files
    if(eventnumbers.size()>0){
        gSystem->Exec(Form("mkdir -p %s", confIO.dirOut));
        TString outFilename = make_output_filename(file, confIO, confSelect);
        TFile *outFile = new TFile(outFilename, "RECREATE");
        TTree *outTree = fTree->CloneTree(0);
        for (int i=0;i<entries;i++){
            fTree->GetEntry(i);
            if (eventnumbers.find(eventnumber) != eventnumbers.end()){
                outTree->Fill();
            }        
        }
        outTree->Write();
        outFile->Close();
    }
    input->Close();
    return eventnumbers.size();
}

void extractEventsWtGeCoincidence(const char * dirIn="input/",
                                  const char * inPrefix="output",
                                  double minGeDep=1839, double maxGeDep=2239, double minLArDep=0, double maxLArDep=20000,
                                  int minGeMultiplicity=1, int maxGeMultiplicity=1,
                                  const char * dirOut="output/", const char * outPrefix = "ExportCriticalEvent"){
    selection confSelect(minGeDep, maxGeDep, minLArDep, maxLArDep, minGeMultiplicity, maxGeMultiplicity);
    io confIO(dirIn, dirOut, inPrefix, outPrefix);

    void *dirp = gSystem->OpenDirectory(confIO.dirIn);
    const char *direntry;
    int k_critical_events = 0, i = 0;
    while((direntry = (char*) gSystem->GetDirEntry(dirp))){
        TString fileName = direntry;
        if(!isRootFile(fileName, confIO.inPrefix))    continue;
        i++;
        k_critical_events += processSingleFileGeCoincidence(direntry, confSelect, confIO, i);

    }
    // printout result
    cout << "\n[Result] Critical Events: " << k_critical_events << endl;
    // merging files into a single one
    // mergeTreeFiles(dirOut, outPrefix, "fTree", true);
    gSystem->FreeDirectory(dirp);
}
