#include <set>
#include <TString.h>
#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <TSystem.h>
#include "utils.cpp"

using namespace std;

int processSingleFileGeCoincidence(double minGeDep, double maxGeDep, double minLArDep, double maxLArDep,
								   int minGeMultiplicity, int maxGeMultiplicity,
								   const char* dirIn, const char* startFile, const char* dirOut, const char * outPrefix){
	// load input file
    TFile *input = new TFile(Form("%s/%s", dirIn, startFile),"READ");
    TTree *fTree = (TTree*) input->Get("fTree");
    int entries = fTree->GetEntries();
	cout << "File: " << startFile << "\n";
	cout << "\tEntries: " << entries << "\n";
	// initialize branch variables
	double energydeposition = 0;
    int pedetected = 0;
    int eventnumber = 0;
	int detectornumber = 0;
	int storedeventnumber = -1;
    double totalNPE = 0;
    double totalArgonEnergy = 0;
    double totalGeEnergy = 0;
    set<int> hitGeDetectors;
    string *material = NULL;
	// connect branches
    fTree->SetBranchAddress("energydeposition", &energydeposition);
    fTree->SetBranchAddress("eventnumber", &eventnumber);
	fTree->SetBranchAddress("material", &material);
	fTree->SetBranchAddress("detectornumber", &detectornumber);
	// load first entry to store first event number
    fTree->GetEntry(0);
    storedeventnumber = eventnumber;
    // initialize set of events
    set<int> eventnumbers;
    for (int i=0;i<entries;i++){
        fTree->GetEntry(i);
        if(eventnumber!=storedeventnumber){
        	//Event is complete, begin energy summing of another event
            if(totalGeEnergy>=minGeDep & totalGeEnergy<=maxGeDep &
               hitGeDetectors.size() >= minGeMultiplicity & hitGeDetectors.size() <= maxGeMultiplicity &
               totalArgonEnergy>=minLArDep & totalArgonEnergy<=maxLArDep){
                cout << "\tEvent: " << storedeventnumber << ": GeEnergy: " << totalGeEnergy << " KeV, NPE: " << totalNPE << endl;
                eventnumbers.insert(storedeventnumber);
            }
			// Reset energies counter
            totalGeEnergy = 0;
            totalNPE = 0;
            totalArgonEnergy = 0;
            storedeventnumber = eventnumber;
        }

        if(strstr(material->c_str(),"ArgonLiquid")) {
	        totalArgonEnergy += energydeposition;
        }
        if(strstr(material->c_str(),"GermaniumEnriched")) {
	        totalGeEnergy += energydeposition;
	        hitGeDetectors.insert(detectornumber)
        }
        totalNPE+=pedetected;       // Ge entries have 0 PE detected
    }
    // check last event
    if(totalGeEnergy>=minGeDep & totalGeEnergy<=maxGeDep & totalArgonEnergy<=maxLArDep){
        cout << "\tEvent: " << storedeventnumber << ": GeEnergy: " << totalGeEnergy << " KeV, NPE: " << totalNPE << endl;
        eventnumbers.insert(storedeventnumber);
    }
    // export critical events to separate output files
    int i_e = 0;
    for(auto critical_event: eventnumbers){
        i_e++;
        TFile *out = new TFile(Form("%s/%s%d_%d.root", dirOut, outPrefix, i_e, critical_event),
							   "RECREATE");
        TTree* oTree = fTree->CopyTree(Form("eventnumber==%d", critical_event));
        oTree->Write();
        out->Close();
    }
    input->Close();
    return eventnumbers.size();
}

void extractEventsWtGeCoincidence(const char * dirIn="/home/data/muons-25-july-2020/PostProc/",
                                  const char * filePrefix="SlicedDetections",
								  double minGeDep=1839, double maxGeDep=2239, double minLArDep=0, double maxLArDep=20000,
								  int minGeMultiplicity=1, int maxGeMultiplicity=1,
                                  const char * dirOut="data/", const char * outPrefix = "ExportCriticalEvent"){
    void *dirp = gSystem->OpenDirectory(dirIn);
    const char *direntry;
    int k_critical_events = 0;
    while((direntry = (char*) gSystem->GetDirEntry(dirp))){
        TString fileName = direntry;
        if(!isRootFile(fileName, filePrefix))    continue;
        k_critical_events += processSingleFileGeCoincidence(minGeDep, maxGeDep, minLArDep, maxLArDep,
                                                            minGeMultiplicity, maxGeMultiplicity,
                                                            dirIn, direntry, dirOut, outPrefix);
    }
    // printout result
    cout << "\n[Result] Critical Events: " << k_critical_events << endl;
    // merging files into a single one
    mergeTreeFiles(dirOut, outPrefix, "fTree", true);
    gSystem->FreeDirectory(dirp);
}
