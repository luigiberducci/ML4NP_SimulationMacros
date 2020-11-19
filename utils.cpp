bool isRootFile(TString fileName, TString prefix){
	// Consider files `prefixXXXXXX.root`
	TString begin = fileName(0, prefix.Length());
	if(begin.EqualTo(prefix) & fileName.EndsWith("root"))
		return true;
	return false;
}