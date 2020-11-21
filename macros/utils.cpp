#include <TChain.h>

using namespace std;

bool isRootFile(TString fileName, TString prefix){
    // Consider files `prefixXXXXXX.root`
    TString begin = fileName(0, prefix.Length());
    if(begin.EqualTo(prefix) & fileName.EndsWith("root"))
        return true;
    return false;
}

void mergeTreeFiles(const char * dirIn, const char * filePrefix, const char * treeName="fTree", bool print=true){
    // open directory
    if (print) {
        cout << "[Info] Input Directory: " << dirIn << "\n";
        cout << "[Info] Merging files with prefix: " << filePrefix << "*root" << endl;
    }
    void *dirPointer = gSystem->OpenDirectory(dirIn);
    const char *dirEntry;
    int k_files = 0;
    // create chain of tree names `treeName`
    TChain ch(treeName);
    while((dirEntry = (char*) gSystem->GetDirEntry(dirPointer))){
        TString fileName = dirEntry;
        if(!isRootFile(fileName, filePrefix))    continue;
        ch.Add(Form("%s/%s", dirIn, dirEntry));
        k_files++;
    }
    // merging trees
    if(ch.GetNtrees()>0) {
        const char *outFile = Form("%s/%s_MERGED.root", dirIn, filePrefix);
        ch.Merge(outFile);
        // print out result
        if (print) {
            cout << "[Result] Merged Files: " << k_files << ", Output file: " << outFile << endl;
            cout << "[Result] Done." << endl;
        }
    }else{
        assert(k_files==0);
    }
    gSystem->FreeDirectory(dirPointer);
}