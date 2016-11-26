#!/usr/bin/perl

use Cwd;
use sort 'stable';
use lib 'perl/';
use common_subs;

my $write_files = 1;
my $check_num_pilots = 0;
my $specific_pc = 1;

my $numArgs = $#ARGV + 1;
if($numArgs != 7) {
	print "Usage: perl gen_store_simics_files.pl <app_name> <static_file_name> <mode_change_log> <heap_patterns_log> <stack_patterns_log> <agen/reg> <in_sdc_file>\n";  
	print "$ARGV[0] \n";
	print "$ARGV[1] \n";
	print "$ARGV[2] \n";
	print "$ARGV[3] \n";
	print "$ARGV[4] \n";
	print "$ARGV[5] \n";
	print "$ARGV[6] \n";
	die;
}
my $app_name = $ARGV[0];
my $static_file_name = $ARGV[1];
my $mode_change_file_name = $ARGV[2];
my $heap_patterns_file_name = $ARGV[3];
my $stack_patterns_file_name = $ARGV[4];
my $agen_reg = $ARGV[5];
my $in_sdc_filename = $ARGV[6];

my $fault_type = -1;
if($agen_reg eq "reg") {
	$fault_type = 4;
} elsif($agen_reg eq "agen") {
	$fault_type = 8;
}
print "$fault_type\n";

my $prune_type = "store";

my $pwd = cwd();
$app_name = get_actual_app_name($app_name);

my %pilot_population_map = ();
my @population_list;
my $temp_count = 0;

my %store_pilot_map = (); # pc->pilots
my %store_sample_map = (); # pilot->samples
open IN_STORE, $stack_patterns_file_name or die "$!: $stack_patterns_file_name";
while (<IN_STORE>) {
	my $d_line = $_;
	chomp($d_line);
	if($d_line ne "") {
		my @d_words = split (/:/, $d_line);
		$d_words[2] = trim($d_words[2]);
		$d_words[1] = trim($d_words[1]);
		$d_words[0] = trim($d_words[0]);
		if($d_words[2] > 0) {
			my $priv_cycles = "";
			if(exists $store_pilot_map {$d_words[0]}) { # d_words[0] is pc
				$priv_cycles = $store_pilot_map {$d_words[0]};
			} 
			$store_pilot_map {$d_words[0]} = "$d_words[1] $priv_cycles"; # d_words[1] is pilot

			# fix - if d_words[2] is < 700 then increment it with 1 and add the pilot in to the samples
    		# or simply ignore this becuse we already inject in pilot seperately
			
			$store_sample_map {$d_words[1]} = $d_words[3]; # d_words[3] has samples
			$pilot_population_map {$d_words[1]} = $d_words[2];
		}
	}
}
close IN_STORE;

print "loaded stack file\n";

open IN_STORE, $heap_patterns_file_name or die "$!: $heap_patterns_file_name";
while (<IN_STORE>) {
	my $d_line = $_;
	chomp($d_line);
	if($d_line ne "") {
		my @d_words = split (/:/, $d_line);
		$d_words[2] = trim($d_words[2]);
		$d_words[1] = trim($d_words[1]);
		$d_words[0] = trim($d_words[0]);
		if($d_words[2] > 0) {
			if(exists $store_pilot_map {$d_words[0]}) { # d_words[0] is pc
				$store_pilot_map {$d_words[0]} .= " $d_words[1]"; # d_words[1] is pilot
			} else {
				$store_pilot_map {$d_words[0]} = "$d_words[1]"; # d_words[1] is pilot
			}

			# fix - if d_words[2] is < 700 then increment it with 1 and add the pilot in to the samples
    		# or simply ignore this becuse we already inject in pilot seperately
			
			$store_sample_map {$d_words[1]} = $d_words[3]; # d_words[3] has samples
			$pilot_population_map {$d_words[1]} = $d_words[2];
		}
	}
}
close IN_STORE;

print "loaded heap file\n";

#my @mode_file;
# read the mode info (ranges) in to arrays now and keep it in memory
# This will help not to open the file every time you want to access the mode info
my @range;
my $m_file_index = 0;
open IN_MODE_FILE, $mode_change_file_name or die "$!: $mode_change_file_name";
while (<IN_MODE_FILE>) {
	my $m_line = $_;
	chomp($m_line);
	#$mode_file[$m_file_index] = $m_line;
	chomp($m_line);
	my @temp_range = split (/-/, $m_line);
	$range[$m_file_index][0] = $temp_range[0]; 
	$range[$m_file_index][1] = $temp_range[1]; 
	$m_file_index++;
}
close IN_MODE_FILE;

sub adjust_cycle {
	my $cycle = int($_[0]);
	my $diff_cycle = int($_[1]);

	if ($cycle <100) {
		print "Strange, cycle<100";
	}

	$cycle = $cycle - $diff_cycle;
	my $temp_cycle = $cycle;
	if($diff_cycle != 0) {

		my $num_adjustments = 0;
		my $need_adjustment = 1; # need to test if the cycle is found in the priv mode again
		while($need_adjustment == 1) {

			my $start = 0;
			my $end = $m_file_index;
			my $adjusted = 0;
			# first step towards binary search
			while ($end-$start > 100) { 
				$middle_point = $start+int(($end-$start)/2);
				if ($cycle <= $range[$middle_point][1]) {
					$end = $middle_point;
				} else {
					$start = $middle_point+1; 
				}
			}
			#print "done:start=$start, end=$end\n";

			for ($i=$start; $i<$end; $i++) {
				if($cycle > $range[$i][0] && $cycle < $range[$i][1])  { # cycle falls in the range of priv mode
					$cycle = $cycle - ($range[$i][1]-$range[$i][0]) - 1;
					$adjusted = 1;  # cycle is adusted in this iteration 
					$num_adjustments++;
				} elsif($range[$i][0] > $cycle) {
					last;
				}
			}
			if($adjusted == 0) { # check if the cycle is adusted in this iteration
				$need_adjustment = 0; # if not then no need to check again
			}
		}
	}

	return $cycle;
}

my %store_pruned_pc = ();
open IN_STATIC, $static_file_name or die "$!: $static_file_name";
while (<IN_STATIC>) { # for every line
		my $line = $_;
		my $proceed = 0;
		my $cycle_list = "";
	
		my @words = split (/:/, $line);
		# frequecy[]:pc:dep_type:{dep_pc,diff_cycle}:faults
		# 0- frequency
		# 1 - pc
		# 2 - dep_type
		# 3 - dep_pc
		# 4 - faults
		my $frequency =  $words[0];
		my $count = 0;
		if($frequency =~ /(.*)\[\]/) {
			$count = $1;
		}
		my $my_pc = $words[1];
		my $dep_type = $words[2];
		$dep_type = int($dep_type);
		my $fault_list = $words [4];
		my $dep_pc = "";
		my $diff_cycle = "";
		if($words[3] =~ /\{([0-9a-z]*)\, (-*[0-9]+)\}/) {
			$dep_pc = $1;
			$diff_cycle = $2;
		}
	
		if ($dep_type == 0) { # STORE
			$store_pruned_pc{$my_pc} = 1; 
		}
}


# Algo:
# 1. For every SDC <pc, pilot> tuple
#	1.1 If it's pruned by store-eq tech
#		1.1.1 obtain list of all samples in that eq class
#		1.1.2 adjust cycle number and print to file with some probability 
if($write_files == 1) {	
	open (SIMICS_FILE_LIST, ">simics_file_list/random_selection/simics_file_list_$app_name\_store_validation_sdcs_targeted_specific_pc_0x100003a10.txt");
	#open (SIMICS_FILE_LIST, ">simics_file_list/random_selection/simics_file_list_$app_name\_store_validation_sdcs_top50.txt");
}
my $prob = 1.007;
print "prob = $prob\n";
my $num_pilots = 0; 
my $simics_file_counter = 0;
my $represented_pop = 0;
my $other_pop = 0;

open (IN_SDC_FILE, "<$in_sdc_filename");
while (<IN_SDC_FILE>) {
	my $s_line = $_;
	chomp($s_line);
	my @words = split (/:/, $s_line);
	my $sdc_pop_size = int($words[0]);
	my $sdc_runname = $words[1];

	my @r_words = split(/\./, $sdc_runname);
	my $sdc_pc  = $r_words[1];
	my $sdc_pilot = $r_words[2];
	my $sdc_other_info = "$r_words[3].$r_words[4].$r_words[5].$r_words[6].$r_words[7]";

	if($specific_pc == 1)  {
		#if($sdc_pc ne "0x10000371c") {
		#if($sdc_pc ne "0x1000020e4") {
		if($sdc_pc ne "0x100003a0c") {
			next;
		}
		print "here";
		if(int($r_words[5]) != 15) {
			next;
		}
		if(int($r_words[4]) != 5) {
			next;
		}
		print "here too";
	}

	if ($check_num_pilots == 1) {
		if(exists $store_pruned_pc{$sdc_pc}) {
			print "$s_line\n";
			$num_pilots += 1;
			$represented_pop += $sdc_pop_size;
		} else {
			$other_pop += $sdc_pop_size;
		}

	} else {
		open IN_STATIC, $static_file_name or die "$!: $static_file_name";
		while (<IN_STATIC>) { # for every line
			my $line = $_;
			my $proceed = 0;
			my $cycle_list = "";
		
			my @words = split (/:/, $line);
			# frequecy[]:pc:dep_type:{dep_pc,diff_cycle}:faults
			# 0- frequency
			# 1 - pc
			# 2 - dep_type
			# 3 - dep_pc
			# 4 - faults
			my $frequency =  $words[0];
			my $count = 0;
			if($frequency =~ /(.*)\[\]/) {
				$count = $1;
			}
			my $my_pc = $words[1];
			my $dep_type = $words[2];
			$dep_type = int($dep_type);
			my $fault_list = $words [4];
			my $dep_pc = "";
			my $diff_cycle = "";
			if($words[3] =~ /\{([0-9a-z]*)\, (-*[0-9]+)\}/) {
				$dep_pc = $1;
				$diff_cycle = $2;
			}
		
			if ($dep_type == 0 && ($my_pc eq $sdc_pc)) { # STORE
				$proceed = 1;
			}
		
			if($proceed == 1) {
				##### check for prob
				$rand_num  = rand();
				if($rand_num > $prob) {
					next;
				}

				$pilot_list = "";
				$pilot_list = $store_pilot_map{$dep_pc} if exists $store_pilot_map{$dep_pc};
				@pilots = split(/ /,$pilot_list);
				foreach $pilot (@pilots) {
					if($pilot eq "") {
							next;
					}

					$pilot = trim($pilot);
					my $adjusted_pilot = adjust_cycle($pilot, $diff_cycle);

					#print "$adjusted_pilot\n";

					$sample_list = "";
					$sample_list = $store_sample_map{$pilot} if exists $store_sample_map{$pilot};

					@samples = split(/ /, $sample_list);

					# find sdc pilot inthe sample list
					my $found_sdc_pilot = 0;
					foreach $cycle (@samples) {
						if($cycle ne "") {
							$adjusted_cycle = adjust_cycle($cycle,$diff_cycle);
							if($adjusted_cycle == -1) {
								print "adjusted cycle = -1 : $cycle, $diff_cycle, $my_pc";
							}

							if ($adjusted_cycle == int($sdc_pilot)) {
								$found_sdc_pilot = 1;
								last;
							}
						}
					}
		

					if($#samples >= 2 && $found_sdc_pilot == 1  || $specific_pc == 1)  {
#						write simics file for pilot 
						$simics_file_counter += 1;
						$num_pilots += 1;
# 						cycle, arch_reg, bit
						if($write_files == 1) {
							my $simics_filename = "$app_name.$my_pc.$adjusted_pilot.$sdc_other_info";
							print SIMICS_FILE_LIST "$simics_filename\n";
						} else {
							#print "$simics_filename\n";
						}

						foreach $cycle (@samples) {
							if($cycle ne "") {
								$simics_file_counter += 1;

							    if($write_files == 1) {
									$adjusted_cycle = adjust_cycle($cycle,$diff_cycle);
									if($adjusted_cycle == -1) {
										print "adjusted cycle = -1 : $cycle, $diff_cycle, $my_pc";
									}
#								    cycle, arch_reg, bit
									my $simics_filename = "$app_name.$my_pc.$adjusted_cycle.$sdc_other_info";
							   		print SIMICS_FILE_LIST "$simics_filename\n";
							    } else {
									#print "$simics_filename\n";
								}
							}
						}

						print "$s_line\n";
						if($specific_pc == 0) {
							last; # no need to search in other pilots for this pc 
						}
					}
				}
			}
		}
		close IN_STATIC;
	}
	if($specific_pc == 1) {
		last;
	}
	if($num_pilots > 50) {
		last;
		if($num_pilots > 100) {
			#last;
		}
	} else {
		#print ".";
	}
	# if($num_pilots %10 ==1 ) {
	# 	print "num pilots so far = $num_pilots\n";
	# } 
}


print "Number of pilots: $num_pilots\n";
print "Number of simics files: $simics_file_counter \n";
if($check_num_pilots == 1) {
	my $r_f = $represented_pop / ($represented_pop+$other_pop);
	print "Represented pop size: $represented_pop ($r_f)\n";
	my $o_f = $other_pop / ($represented_pop+$other_pop);
	print "Other pop size: $other_pop ($o_f)\n";
}

if($write_files == 1) {
	close SIMICS_FILE_LIST;
}
