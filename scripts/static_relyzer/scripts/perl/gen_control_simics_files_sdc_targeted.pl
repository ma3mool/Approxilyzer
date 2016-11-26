#!/usr/bin/perl

use Cwd;
use sort 'stable';
use lib 'perl/';
use common_subs;

my $write_files = 0;
my $specific_pc = 1;
my $unique_pcs = 0;

my $numArgs = $#ARGV + 1;
if($numArgs != 7) {
	print "Usage: perl gen_control_simics_files.pl <app_name> <0/1> <static_file_name> <mode_change_log> <control_patterns_log> <agen/reg> <in_sdc_file>\n";  
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
my $control_live = $ARGV[1];
my $static_file_name = $ARGV[2];
my $mode_change_file_name = $ARGV[3];
my $control_patterns_file_name = $ARGV[4];
my $agen_reg = $ARGV[5];
my $in_sdc_filename = $ARGV[6];

my $fault_type = -1;
if($agen_reg eq "reg") {
	$fault_type = 4;
} elsif($agen_reg eq "agen") {
	$fault_type = 8;
}
print "$fault_type\n";


my $prune_type = "";
if($control_live == 0) {
		$prune_type = "control";
} else {
		$prune_type = "live";
}

my $pwd = cwd();
$app_name = get_actual_app_name($app_name);

# Formula : (Z^2 * p*(1-p) ) / (E^2)
# Z :
#   99% - 2.58
#   98% - 2.33
#   95% - 1.96 
#   90% - 1.645
my $E = 0.05;
my $Z = 2.58;
my $p = 0.5;
my $E_2 = $E*$E;
my $Z_2_p_1_p = $Z*$Z*$p*(1-$p);

open IN_STATIC, $static_file_name or die "$!: $static_file_name";

my $line = "";

my %control_pilot_map = (); # pc->pilots
my %control_sample_map = (); # pilot->samples
my %pilot_population_map = (); # pilot->population size
my @population_list;
my $temp_count = 0;
my $total_pilots = 0;
open IN_CONTROL, $control_patterns_file_name or die "$!: $control_patterns_file_name";
while (<IN_CONTROL>) {
	my $d_line = $_;
	chomp($d_line);
	if($d_line ne "") {
		my @d_words = split (/:/, $d_line);
		my $priv_cycles = "";
		$d_words[0] = trim($d_words[0]);
		$d_words[1] = trim($d_words[1]);
		$d_words[2] = trim($d_words[2]);
		#if (int($d_words[2]) > 10000) {
			#print "$d_words[2]\n";
			if(exists $control_pilot_map {$d_words[0]}) { # d_words[0] is pc
				$priv_cycles = $control_pilot_map {$d_words[0]};
			} 
			$control_pilot_map {$d_words[0]} = "$priv_cycles $d_words[1]"; # d_words[1] is pilot
			$control_sample_map {$d_words[1]} = $d_words[3]; # d_words[3] has samples
			$pilot_population_map {$d_words[1]} = $d_words[2];
			$population_list [$temp_count++] = int($d_words[2]);
			$total_pilots = $total_pilots + 1;
		#}
	}
}
close IN_CONTROL;
print "Total number of pilots = $total_pilots\n";

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
	my $cycle = $_[0];
	my $diff_cycle = $_[1];

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

# Algo:
# 1. For every SDC <pc, pilot> tuple
#	1.1 If it's pruned by control-eq tech
#		1.1.1 obtain list of all samples in that eq class
#		1.1.2 adjust cycle number and print to file with some probability 

if($write_files == 1) {
	#open (SIMICS_FILE_LIST, ">simics_file_list/random_selection/simics_file_list_$app_name\_control_validation_sdcs_targeted_specific_pc_0x1000039e8_50.txt");
	open (SIMICS_FILE_LIST, ">simics_file_list/random_selection/simics_file_list_$app_name\_control_validation_sdcs_targeted_50.txt");
	#open (SIMICS_FILE_LIST, ">simics_file_list/random_selection/simics_file_list_$app_name\_control_validation_sdcs_targeted_specific_pc_0x100003a10_40.txt");
	#open (SIMICS_FILE_LIST, ">simics_file_list/random_selection/simics_file_list_$app_name\_control_validation_sdcs_targeted_specific_pc_0x100003554_10.txt");
	#open (SIMICS_FILE_LIST, ">simics_file_list/random_selection/simics_file_list_$app_name\_control_validation_sdcs_specific_pc_0x100003d74_10.txt");
	#open (SIMICS_FILE_LIST, ">simics_file_list/random_selection/simics_file_list_$app_name\_control_validation_sdcs_random._10.txt");
	#open (SIMICS_FILE_LIST, ">simics_file_list/random_selection/simics_file_list_$app_name\_control_validation_sdcs_targeted_top25_10.txt");
}
my $prob = 1.01;
print "prob = $prob\n";
my $num_pilots = 0; 
my $simics_file_counter = 0;
my %selected_pc_map = ();

my $tot_pop_control = 0;
my $tot_pop_store = 0;
my $count_i  = 0;

open (IN_SDC_FILE, "<$in_sdc_filename");
while (<IN_SDC_FILE>) {
	my $s_line = $_;
	chomp($s_line);
	my @words = split (/:/, $s_line);
	my $sdc_pop_size = int($words[0]);
	my $sdc_runname = $words[1];

	my @r_words = split(/\./, $sdc_runname);
	my $sdc_pc  = $r_words[1];

	if($specific_pc == 1)  {

		if(#($sdc_pc eq "0x1000039e8" && int($r_words[4]) == 10  && int($r_words[5]) == 26  && int($r_words[6]) == 0) || 
		   #($sdc_pc eq "0x100003a10" && int($r_words[4]) == 3  && int($r_words[5]) == 15  && int($r_words[6]) == 1) || 
		   #($sdc_pc eq "0x100003a10" && int($r_words[4]) == 2  && int($r_words[5]) == 9  && int($r_words[6]) == 0)
			($sdc_pc eq "0x1000020e8" && int($r_words[4]) == 9  && int($r_words[5]) == 57  && int($r_words[6]) == 0 && int($r_words[2]) == 384091006720)
			) {
			print "################# go forward: $s_line\n";
		} else {
			next;
		}
		# $r_words[4] = 3;
		# $r_words[5] = 15;
		# $r_words[6] = 1;

		# $r_words[4] = 5;
		# $r_words[5] = 3;
		# $r_words[6] = 0;

		# $sdc_pc = "0x1000039e8";
		# $r_words[4] = 10;
		# $r_words[5] = 26;
		# $r_words[6] = 0; 


		#if($sdc_pc ne "0x10000371c") {
		#if($sdc_pc ne "0x1000020e4") {
		#if($sdc_pc ne "0x100003d74") {
		#if($sdc_pc ne "0x100003a10") {
		#if($sdc_pc ne "0x100003554") {
		#if($sdc_pc ne "0x1000039e8" ) {
		#if($sdc_pc ne "0x100002ed4") { # b12, destination, reg23
		#if($sdc_pc ne "0x10000371c") {
		#	next;
		#}
		#print "here";
		# if(int($r_words[5]) != 15) { # bit location
		# 	next;
		# }
		# if(int($r_words[4]) != 10) { # reg 
		# 	next;
		# }
		# if(int($r_words[6]) != 0) { # source/destination
		# 	next;
		# }

		# $r_words[4] = 3; # reg 
		# $r_words[5] = 15; # bit
		# $r_words[6] = 1; # src dest
		print "here too\n ";
	}

	my $sdc_pilot = $r_words[2];
	my $sdc_other_info = "$r_words[3].$r_words[4].$r_words[5].$r_words[6].$r_words[7]";
	print "!!!!!!!!!!!!!! $sdc_pilot: $sdc_other_info\n";

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
	
		if (($dep_type == 1 || $dep_type == 2 || $dep_type == 10) && ($my_pc eq $sdc_pc)) { # CONTROL, LIVE, and COMPARE
			$proceed = 1;

			$tot_pop_control += $sdc_pop_size;
			# #print "Control += $sdc_pop_size\n";
			# $fraction = $tot_pop_control / ($tot_pop_control + $tot_pop_store);
			# print "control_fraction=$fraction: ($tot_pop_control, $tot_pop_store)\n";
			# $count_i += 1;
			#last;
		} elsif ($dep_type == 0 && ($my_pc eq $sdc_pc)) { # STORE
			$tot_pop_store += $sdc_pop_size;
			# $fraction = $tot_pop_control / ($tot_pop_control + $tot_pop_store);
			# print "control_fraction=$fraction: ($tot_pop_control, $tot_pop_store)\n";
			# #print "Store += $sdc_pop_size\n";
			# $count_i += 1;
			# last;
		}
	
		if($proceed == 1) {
			##### check for prob
			$rand_num  = rand();
			if($rand_num > $prob) {
				next;
			}

			if ($unique_pcs == 1) {
				if (exists $selected_pc_map{$sdc_pc}) {
					$selected_pc_map{$sdc_pc} += 1;
					next;
				} else {
					$selected_pc_map{$sdc_pc} = 1;
				}
			}

			$pilot_list = "";
			$pilot_list = $control_pilot_map{$dep_pc} if exists $control_pilot_map{$dep_pc};
			@pilots = split(/ /,$pilot_list);
			foreach $pilot (@pilots) {
				if($pilot eq "") {
						next;
				}

				$pilot = trim($pilot);
				my $adjusted_pilot = adjust_cycle($pilot, $diff_cycle);

				#print "$pilot: $adjusted_pilot\n";

				$sample_list = "";
				$sample_list = $control_sample_map{$pilot} if exists $control_sample_map{$pilot};
				if ($pilot_population_map{$pilot} != 2006045) {
					next;
				}

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
							#print "$found_sdc_pilot: pilot=$adjusted_pilot: cycle=$adjusted_cycle: $diff_cycle: sdc_pilot=$sdc_pilot\n";
							last;
						}
					}
				}
	

				if($#samples >= 50 && ($found_sdc_pilot == 1 || $specific_pc == 1))  { # why have || $specific_pc == 1?

#					write simics file for pilot 
					$simics_file_counter += 1;
					$num_pilots += 1;
# 					cycle, arch_reg, bit
					my $simics_filename = "$app_name.$my_pc.$adjusted_pilot.$sdc_other_info";
					print "$simics_filename\n";
					if($write_files == 1) {
						print SIMICS_FILE_LIST "$simics_filename\n";
					} else {
						print "$pilot:$adjusted_pilot:$diff_cycle:$found_sdc_pilot\n";
						print "$simics_filename\n";
					}

					foreach $cycle (@samples) {
						if($cycle ne "") {
							$adjusted_cycle = adjust_cycle($cycle,$diff_cycle);
							if($adjusted_cycle == -1) {
								print "adjusted cycle = -1 : $cycle, $diff_cycle, $my_pc";
							}
							$simics_file_counter += 1;
#						    cycle, arch_reg, bit
							my $simics_filename = "$app_name.$my_pc.$adjusted_cycle.$sdc_other_info";
						    if($write_files == 1) {
						   		print SIMICS_FILE_LIST "$simics_filename\n";
						    } else {
								#print "$simics_filename\n";
							}
						}
					}

					# print "$s_line\n";
					if($specific_pc == 0) {
						last; # no need to search in other pilots for this pc 
					}
				}
			}
		}
	}
	close IN_STATIC;

	# if($num_pilots > 10) { # for top50
	#  	if($num_pilots > 50) { # for 50-100
	# 	#if($num_pilots > 1000) { # for 50-100
	# 		last;
	# 	}
	# }
	# else {
	# 	print ".";
	# }
	# if($num_pilots %10 == 1) {
	# 	print "$num_pilots\n";
	# } 
	# if($count_i > 10000) { # for 50-100
	# 	last;
	# }
}

for $pc (keys %selected_pc_map) {
	print "$pc\t$selected_pc_map{$pc}\n";
}

#$fraction = $tot_pop_control / ($tot_pop_control + $tot_pop_store);
#print "$fraction: ($tot_pop_control, $tot_pop_store)\n";

print "Number of pilots: $num_pilots\n";
print "Number of simics files: $simics_file_counter \n";

if($write_files) {
	close SIMICS_FILE_LIST;
}

