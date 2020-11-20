#include <TChain.h>

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
		std::cout << "[Info] Input Directory: " << dirIn << "\n";
		std::cout << "[Info] Merging files with prefix: " << filePrefix << std::endl;
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
	const char *outFile = Form("%s/%s_MERGED.root", dirIn, filePrefix);
	ch.Merge(outFile);
	gSystem->FreeDirectory(dirPointer);
	// print out result
	if (print) {
		std::cout << "[Result] Merged Files: " << k_files << ", Output file: " << outFile << std::endl;
		std::cout << "[Result] Done." << std::endl;
	}
}