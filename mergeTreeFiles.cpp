//
// Created by luigi on 19/11/20.
//
#include <TChain.h>
#include <iostream>
#include <TSystem.h>
#include "utils.cpp"

using namespace std;

void mergeTreeFiles(const char * dirIn, const char * filePrefix, const char * treeName="fTree"){
	// open directory
	cout << "[Info] Input Directory: " << dirIn << ", ";
	cout << "[Info] Merging files with prefix: " << filePrefix << endl;
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
	cout << "[Result] Merged Files: " << k_files << ", Output file: " << outFile << endl;
	cout << "[Result] Done." << endl;
}