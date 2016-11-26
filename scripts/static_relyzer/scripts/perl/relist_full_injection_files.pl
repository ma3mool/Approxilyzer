#!/usr/bin/perl

@apps = ("fft", "swaptions", "lu", "blackscholes");
# @apps = ("swaptions");
foreach $curr_app (@apps) {
 	print " started listing files for $curr_app\n";
 	my $LOGS = "/home/sadve/shari2/research/relyzer/optimized_simics_files/$curr_app\_all_simics_files";
 	opendir(DIR, $LOGS);
 	my @temp_files = grep(/$curr_app.*\.simics$/,readdir(DIR));
 	my @files = sort @temp_files;
 	closedir(DIR);

	my $app_name = "";
	if($curr_app eq "blackscholes") {
		$app_name = "blackscholes_simmedium";
	}
	if($curr_app eq "swaptions") {
		$app_name = "swaptions_simsmall";
	}
	if($curr_app eq "lu") {
		$app_name = "lu_reduced";
	}
	if($curr_app eq "fft") {
		$app_name = "fft_small";
	}
	if($curr_app eq "libquantum") {
		$app_name = "libquantum_test";
	}
	if($curr_app eq "omnetpp") {
		$app_name = "omnetpp_test";
	}

 	open SIMICS_FILE_LIST, ">simics_file_list/simics_file_list_$app_name\_all.txt";
 	foreach $file (@files) {
 		print SIMICS_FILE_LIST "$LOGS/$file\n";
 	}
 	close SIMICS_FILE_LIST; 
}

