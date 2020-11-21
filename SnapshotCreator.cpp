#include <iostream>
#include <TEntryList.h>
#include <TParameter.h>
#include <TFile.h>
#include <TString.h>
#include <TTree.h>
#include <TBranch.h>
#include <TChain.h>
#include <TH2D.h>
#include <TH3D.h>
#include <TROOT.h>
#include <TSystem.h>
#include <TRandom.h>
#include <array>
#include <vector>
#include <set>
#include <assert.h>

// Params
#define RNDSEED 123456789
#define PI 3.14159265
#define INF 999999999

#define ARGONMATERIAL "ArgonLiquid"

using namespace std;

bool isRootFile(TString fileName, TString prefix){
    // Consider files `prefixXXXXXX.root`
    TString begin = fileName(0, prefix.Length());
    if(begin.EqualTo(prefix) & fileName.EndsWith("root"))
        return true;
    return false;
}

bool isSimulationFile(TString fileName, TString prefix="output"){
    // Consider files `prefixXXXXXX.root`, where prefix=output in CJ simulations
    return isRootFile(fileName, prefix);
}

TString createDatasetFilename(const char * dirOut, const char * prefixOut, Double_t DeltaTns,
                              Int_t nGroupedEvents, Int_t filePartID){
    char* fullDirOut = gSystem->ExpandPathName(dirOut);
    TString outFileName;
    outFileName.Form("%s/%s_T%f_Grp%d_Seed%d_Part%d.csv",
                            fullDirOut, prefixOut,
                            DeltaTns, nGroupedEvents,
                            RNDSEED, filePartID);
    return outFileName;
}

map<Int_t, Double_t> getFirstDetectionInLArPerEvents(TTree * fTree){
    // Assumption: entries with other material (!=LAr) have NPE==0
    Int_t eventnumber, pedetected;
    Double_t time;
    fTree->SetBranchAddress("eventnumber", &eventnumber);
    fTree->SetBranchAddress("time", &time);
    fTree->SetBranchAddress("pedetected", &pedetected);
    // Collect distinct event numbers
    set<Int_t> eventnumbers;
    map<Int_t, Double_t> map_event_t0;
    for(Long64_t i = 0; i < fTree->GetEntries(); i++){
        fTree->GetEntry(i);
        if(pedetected <= 0)
            continue;
        map<Int_t, Double_t>::iterator it = map_event_t0.find(eventnumber);
        if (it == map_event_t0.end()){
            map_event_t0.insert(make_pair(eventnumber, time));
        }else{
            if (it->second > time)
                it->second = time;	//eventually update time with min val
        }
    }
    return map_event_t0;
}

map<Int_t, Double_t> getRndOffsetPerEvents(map<Int_t, Double_t> map_event_t0, Double_t min_offset, Double_t max_offset){
    map<Int_t, Double_t> map_event_offset;
    TRandom rnd = TRandom(RNDSEED);
    for(auto event_t0 : map_event_t0){
        Double_t offset = rnd.Uniform(min_offset, max_offset);    // unif. rnd [minoffset, maxoffset]
        map_event_offset[event_t0.first] = offset;
    }
    return map_event_offset;
}

vector<Long64_t> newDatasetEventInstance(Int_t nSiPM){
    vector<Long64_t> snapshot(nSiPM);
    return snapshot;
}

TEntryList* getEntryListOfEvent(TTree *fTree, Int_t eventnumber){
    // Select entries of event
    TString selection = "eventnumber==";
    selection += eventnumber;
    fTree->Draw(">>entries", selection, "entrylist");
    TEntryList *elist = (TEntryList*)gDirectory->Get("entries");
    return elist;
}

void write_header(ofstream& outCSV, Double_t DeltaT, Int_t filePartID, Int_t nInnerSlices, Int_t nOuterSlices){
    // Write CSV header: number of Dt, Dt in ns, resulting time T
    outCSV << "# File Part: " << filePartID << "\n";
    outCSV << "# T: " << DeltaT << "\n";
    outCSV << "# Each event is a row. Before each row, the id of original events are reported.\n";
    outCSV << "# Row Format: ID, Edep, NPE, InnerSlice0, ..., InnerSlice(N-1), OuterSlice0, ..., OuterSlice(N-1)\n";
    outCSV << "#\n# Columns:\n";
    outCSV << "eventnumber,energydeposition,pedetected,";
    for(Int_t i=0; i < nInnerSlices; i++){
        outCSV << "InnerSlice" << i << ",";
    }
    for(Int_t i=0; i < nOuterSlices; i++){
        outCSV << "OuterSlice" << i << ",";
    }
    outCSV << "\n";
}

void writeProducedEventInOutfile(ofstream& outCSV, Int_t prodEventID,
                                 vector<Long64_t> TSiPMEvent_inner, vector<Long64_t> TSiPMEvent_outer,
                                 vector<Int_t> TSiPMEvent_events, vector<Double_t> TSiPMEvent_offsets,
                                 int PEDetectedInDt, double EDepositedInDt){
    // Write output
    outCSV << "# Grouped Events: ";
    for(auto event_id : TSiPMEvent_events)
        outCSV << event_id << ",";
    outCSV << " Event Offsets: ";
    for(auto offset : TSiPMEvent_offsets)
        outCSV << offset << ",";
    outCSV << endl;
    outCSV << prodEventID << ",";
    outCSV << EDepositedInDt << ",";
    outCSV << PEDetectedInDt << ",";
    for(auto sipm : TSiPMEvent_inner){
        outCSV << sipm << ",";
    }
    for(auto sipm : TSiPMEvent_outer){
        outCSV << sipm << ",";
    }
    outCSV << endl;
}

void produceTimeDataset(const char * dirIn, const char * dirOut, TString prefixIn, TString prefixOut,
                        const int group_events, const int min_npe, int max_npe,
                        const double deltaT=10000, int nInnerSlices=-1, int nOuterSlices=-1){
    // Parameters
    const int min_shifting = 0;	        // Interval shifting (min)
    const int max_shifting = 0;         // No shift -> Trigger on first non-zero detection
    const int max_event_x_file = 250000;  // Max number of output events (snapshots) per file
    // Debug
    cout << "[Info] From files " << dirIn << "/" << prefixIn << "*root" <<  endl;
    cout << "[Info] Create snapshot of T=" << deltaT << " ns, ";
    cout << "grouping " << group_events << " events\n";
    // IO management
    ofstream outCSV;
    Int_t file_part_id = 0;
    Int_t skipped_entries = 0;

    // Loop files in input directory
    char* fullDirIn = gSystem->ExpandPathName(dirIn);
    void* dirp = gSystem->OpenDirectory(fullDirIn);
    const char* entry;
    while((entry = (char*)gSystem->GetDirEntry(dirp))) {
        TString filePath = Form("%s/%s", fullDirIn, entry);
        TString fileName = entry;
        if(!isRootFile(fileName, prefixIn))   continue;
        cout << "\t[Debug] File: " << filePath << endl;	// Debug
        // Open file, get tree and number of sipm
        TFile *f = TFile::Open(filePath);
        TTree *fTree = (TTree*) f->Get("fTree");
        // if nr inner/outer slices not given, it will be inferred by the input file (expected parameter)
        if(nInnerSlices<0 || nOuterSlices<0){
            TParameter<Int_t> *NInnerSlicesParam= (TParameter<Int_t>*) f->Get("NInnerSlices");
            TParameter<Int_t> *NOuterSlicesParam= (TParameter<Int_t>*) f->Get("NOuterSlices");
            if(!NInnerSlicesParam || !NOuterSlicesParam){
                throw "number of inner/outer slices not defined";
            }
            nInnerSlices = NInnerSlicesParam->GetVal();
            nOuterSlices = NOuterSlicesParam->GetVal();
        }
        cout << "\t[Debug] Loaded Tree: nentries " << fTree->GetEntries() << endl;

        // Compute time of first deposit in lar, and random offset for event-shifting
        map<Int_t, Double_t> mapEventToFirstTime = getFirstDetectionInLArPerEvents(fTree);
        cout << "\t[Debug] Computed T0 for nevents " << mapEventToFirstTime.size() << endl;
        map<Int_t, Double_t> mapEventOffset = getRndOffsetPerEvents(mapEventToFirstTime, min_shifting, max_shifting);
        cout << "\t[Debug] Computed Rnd Offsets for nevents " << mapEventOffset.size() << endl;

        // Connect branches
        Int_t eventnumber;
        Double_t time, energydeposition;
        fTree->SetBranchAddress("eventnumber", &eventnumber);
        fTree->SetBranchAddress("energydeposition", &energydeposition);
        fTree->SetBranchAddress("time", &time);
        vector<Int_t> innerSlices(nInnerSlices);
        vector<Int_t> outerSlices(nOuterSlices);
        for(int sipm = 0; sipm < nInnerSlices; sipm++){
            TString branchName = "InnerSlice";
            branchName += sipm;
            fTree->SetBranchAddress(branchName, &innerSlices[sipm]);
        }
        for(int sipm = 0; sipm < nOuterSlices; sipm++){
            TString branchName = "OuterSlice";
            branchName += sipm;
            fTree->SetBranchAddress(branchName, &outerSlices[sipm]);
        }

        int nEvents = mapEventToFirstTime.size();
        int eventCounter = 0, lastStoredEvent = -1, producedEventCounter=0, writtenEventsCounter=0;
        // Create data struct
        vector<Long64_t> TSiPMEvent_inner = newDatasetEventInstance(nInnerSlices);
        vector<Long64_t> TSiPMEvent_outer = newDatasetEventInstance(nOuterSlices);
        double EDepositedInDt = 0;     // cumulative Edep in Lar, over each Dt
        double PEDetectedInDt = 0;     // cumulative Nr of PE detected, over each Dt
        vector<Int_t> TSiPMEvent_events;
        vector<Double_t> TSiPMEvent_offsets;
        Long64_t nEntries = fTree->GetEntries();
        for(Int_t i=0; i < nEntries; i++){
            fTree->GetEntry(i);

            if(eventnumber != lastStoredEvent){	// New Event
                lastStoredEvent = eventnumber;
                eventCounter++;
                // Note: we have to check group_event==1 every time
                if((eventCounter % group_events == 1) || (group_events == 1)){
                    if((eventCounter > 1) || (group_events == 1)){
                        producedEventCounter++;
                        if (producedEventCounter % max_event_x_file == 1) {
                            outCSV.close();
                            file_part_id++; // Increment the id of file part (part1, part2, ..)
                            TString outFile(createDatasetFilename(dirOut, prefixOut, deltaT, group_events, file_part_id));
                            outCSV.open(outFile);
                            write_header(outCSV, deltaT, file_part_id, nInnerSlices, nOuterSlices);
                            cout << "\t[Debug] Start writing in " << outFile << "\n";
                        }

                        // Check filter on Tot NPE
                        if(PEDetectedInDt >= min_npe && PEDetectedInDt <= max_npe){
                            writtenEventsCounter++;
                            writeProducedEventInOutfile(outCSV, writtenEventsCounter, TSiPMEvent_inner,
                                                        TSiPMEvent_outer,
                                                        TSiPMEvent_events, TSiPMEvent_offsets, PEDetectedInDt,
                                                        EDepositedInDt);
                        }
                    }
                    TSiPMEvent_inner = newDatasetEventInstance(nInnerSlices);
                    TSiPMEvent_outer = newDatasetEventInstance(nOuterSlices);
                    TSiPMEvent_events.clear();
                    TSiPMEvent_offsets.clear();
                    PEDetectedInDt = 0;
                    EDepositedInDt = 0;
                }
                TSiPMEvent_events.push_back(eventnumber);
                TSiPMEvent_offsets.push_back(mapEventOffset[eventnumber]);
            }
            // Debug
            if(i % (nEntries/10) == 0)
                cout << "\r\tentry: " << i << "/" << nEntries << std::flush;
            Double_t shiftedTime = time - mapEventToFirstTime[eventnumber] + mapEventOffset[eventnumber];
            if(shiftedTime < 0 || shiftedTime > deltaT){
                // if shifted time overflow <0, or if shifted time>deltaT
                skipped_entries++;
                continue;   // all the others are bigger than time T (eventually overflow)
            }
            // Integrate in the time bin
            for(int sipm = 0; sipm < nInnerSlices; sipm++){
                TSiPMEvent_inner[sipm] += innerSlices[sipm];
                PEDetectedInDt += innerSlices[sipm];
            }
            for(int sipm = 0; sipm < nOuterSlices; sipm++){
                TSiPMEvent_outer[sipm] += outerSlices[sipm];
                PEDetectedInDt += outerSlices[sipm];
            }
            EDepositedInDt += energydeposition;
        }
        // Close file and tree
        fTree->Delete();
        f->Close();
        f->Delete();
        cout << endl;
        cout << "[Info] Produced snapshots: " << writtenEventsCounter << ", ";
        cout << "skipped entries for time: " << skipped_entries << "\n";
    }
    gSystem->FreeDirectory(dirp);
}

void printOutInfos(const char * dirIn, const char * dirOut,
                   const char * inputPrefix, const char * outputPrefix,
                   int groupEvents, int minNPE, int maxNPE, int nInnerSlices, int nOuterSlices){
    cout << "[Info] Snapshot Creator: create time snapshots from sliced simulation data\n";
    cout << "\n";
    cout << "[Info] Input files: " << Form("%s/%s*root", dirIn, inputPrefix) << "\n";
    cout << "[Info] Output files: " << Form("%s/%s*csv", dirOut, outputPrefix) << "\n";
    cout << "[Info] Snapshot Config:\n";
    cout << "\tNr Shroud Slices: ";
    if(nInnerSlices<0 || nOuterSlices<0){
        cout << "inferred by input files";
    } else {
        cout << nInnerSlices << " Inner Slices, ";
        cout << nOuterSlices << " Outer Slices, ";
    }
    cout << "\n";
    cout << "\tNr Event per Snapshot: " << groupEvents << "\n";
    cout << "\tProduced events: min NPE: " << minNPE << ", max NPE: " << maxNPE << "\n";
    cout << "\n";
}

void SnapshotCreator(const char * dirIn, const char * dirOut,
                    const char * inputPrefix="SlicedDetections", const char * outputPrefix="Snapshot",
                    int groupEvents=1, int minNPE=1, int maxNPE=INF, double deltaT=10000,
                    int nInnerSlices=-1, int nOuterSlices=-1){
    printOutInfos(dirIn, dirOut, inputPrefix, outputPrefix, groupEvents, minNPE, maxNPE, nInnerSlices, nOuterSlices);
    try {
        produceTimeDataset(dirIn, dirOut, inputPrefix, outputPrefix, groupEvents, minNPE, maxNPE, deltaT, nInnerSlices, nOuterSlices);
        cout << "[Info] End.\n";
    } catch (const char* msg) {
        cerr << "[**Error**] " << msg << endl;
    }

}