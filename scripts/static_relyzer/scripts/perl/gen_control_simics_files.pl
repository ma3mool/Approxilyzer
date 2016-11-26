#!/usr/bin/perl

use Cwd;
use sort 'stable';
use lib 'perl/';
use common_subs;

my $BIT_GAP = 8;
my $write_files = 1;

my $numArgs = $#ARGV + 1;
if($numArgs != 6) {
	print "Usage: perl gen_control_simics_files.pl <app_name> <0/1> <static_file_name> <mode_change_log> <control_patterns_log> <agen/reg>\n";  
	print "$ARGV[0] \n";
	print "$ARGV[1] \n";
	print "$ARGV[2] \n";
	print "$ARGV[3] \n";
	print "$ARGV[4] \n";
	print "$ARGV[5] \n";
	die;
}
my $app_name = $ARGV[0];
my $control_live = $ARGV[1];
my $static_file_name = $ARGV[2];
my $mode_change_file_name = $ARGV[3];
my $control_patterns_file_name = $ARGV[4];
my $agen_reg = $ARGV[5];

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
		if(exists $control_pilot_map {$d_words[0]}) { # d_words[0] is pc
			$priv_cycles = $control_pilot_map {$d_words[0]};
		} 
		$control_pilot_map {$d_words[0]} = "$d_words[1] $priv_cycles"; # d_words[1] is pilot
		$control_sample_map {$d_words[1]} = $d_words[3]; # d_words[3] has samples
		$pilot_population_map {$d_words[1]} = $d_words[2];
		$population_list [$temp_count++] = int($d_words[2]);
		$total_pilots = $total_pilots + 1;
	}
}
close IN_CONTROL;
print "Total number of pilots = $total_pilots\n";

open IN_STATIC, $static_file_name or die "$!: $static_file_name";
while (<IN_STATIC>) { # for every line
	my $line = $_;
	my $proceed = 0;
	my $cycle_list = "";

	my @words = split (/:/, $line);
	my $dep_type = $words[2];
	$dep_type = int($dep_type);
	my $fault_list = $words [4];
	my $dep_pc = "";
	if($words[3] =~ /\{([0-9a-z]*)\, (-*[0-9]+)\}/) {
		$dep_pc = $1;
	}

	if ($dep_type == 1 || $dep_type == 2 || $dep_type == 10) { # CONTROL, LIVE, and COMPARE
		$proceed = 1;
	}
	chomp($fault_list);
	if($fault_list eq "") {
		$proceed = 0;
	}
	if($proceed == 1) {
		my @faults =  split(/\) \(/, $fault_list);
		foreach $fault (@faults) {
			my $arch_reg =  -1;
			my $high = -1;
			my $low = -1;
			my $src_dest = -1;
			@elems = split(/,/, $fault);
			if( $agen_reg eq "reg") {
				if($elems[0] =~ /%([gloi][0-9]*)$/)  {
					$arch_regname = $1;
					$arch_reg = arch_reg_name2num($arch_regname);
				}elsif($elems[0] =~ /%(fp)$/)  {
					$arch_regname = "i6";
					$arch_reg = arch_reg_name2num($arch_regname);
				}elsif($elems[0] =~ /%(sp)$/)  {
					$arch_regname = "o6";
					$arch_reg = arch_reg_name2num($arch_regname);
				} 
			} elsif( $agen_reg eq "agen") {
				if($elems[0] =~ /^\[.*\]$/)  {
					$arch_reg = 1; # anything other than -1
				}
			}

			if($elems[3] =~ /([01]+)/) {
				$src_dest = $1;
			}

			if($arch_reg == -1  || $src_dest == -1) {
					next;
			} else {

				$pilot_list = "";
				$pilot_list = $control_pilot_map{$dep_pc} if exists $control_pilot_map{$dep_pc};
				@pilots = split(/ /,$pilot_list);
				foreach $pilot (@pilots) {
					if($pilot eq "") {
							next;
					}
					$pilot = trim($pilot);
					$population_list [$temp_count++] = $pilot_population_map{$pilot};
					#print "adding $pilot_population_map{$pilot}\n";
				}
			}
		}
	}
}
close IN_STATIC;

my $cutoff_pop = 1000;
my $pop_sum = 0;
my @sorted_pop_list = sort { $b <=> $a } @population_list;
$temp_count = 0; 
foreach $pop (@sorted_pop_list) { 
	$temp_count++;
	#print "$temp_count\t$sorted_pop_list[$temp_count]\n";
	if($pop <= $cutoff_pop) {
		last;
	}
	if($temp_count > 1000)  {
		last;
	}
	$pop_sum += $pop;
}
$cutoff_pop = $sorted_pop_list[$temp_count-1];
print "cutoff cleared = $temp_count\n";
print "cutoff = $cutoff_pop\n";
my $prob = 0;
if($agen_reg eq "reg") {
 	$prob = 10.0/$temp_count;
} elsif($agen_reg eq "agen") {
	$prob = 20.0/$temp_count;
}
print "prob = $prob\n";

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

my $simics_file_counter = 0;
my $num_pilots = 0;
open (SIMICS_FILE_LIST, ">new_control_simics_file_list/simics_file_list_$app_name\_$prune_type\_$agen_reg.txt");

open IN_STATIC, $static_file_name or die "$!: $static_file_name";
while (<IN_STATIC>) { # for every line
	$line = $_;

	my $found = 0;
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
	my $pc = $words[1];
	my $dep_type = $words[2];
	$dep_type = int($dep_type);
	my $fault_list = $words [4];
	my $dep_pc = "";
	my $diff_cycle = "";
	if($words[3] =~ /\{([0-9a-z]*)\, (-*[0-9]+)\}/) {
		$dep_pc = $1;
		$diff_cycle = $2;
	}

	my $proceed = 0;
	if ($dep_type == 1 || $dep_type == 2 || $dep_type == 10) { # CONTROL, LIVE, and COMPARE
		$proceed = 1;
	}

	chomp($fault_list);
	if($fault_list eq "") {
		$proceed = 0;
	}

	if($proceed == 1) {
		if($diff_cycle != 0) {
			print "diff cycle is $diff_cycle at pc=$pc, dep_pc=$dep_pc\n";
		}

		my @faults =  split(/\) \(/, $fault_list);
		foreach $fault (@faults) {
			my $arch_reg =  -1;
			my $high = -1;
			my $low = -1;
			my $src_dest = -1;
			@elems = split(/,/, $fault);
			if($agen_reg eq "reg") {
				if($elems[0] =~ /%([gloi][0-9]*)$/)  {
					$arch_regname = $1;
					$arch_reg = arch_reg_name2num($arch_regname);
				}elsif($elems[0] =~ /%(fp)$/)  {
					$arch_regname = "i6";
					$arch_reg = arch_reg_name2num($arch_regname);
				}elsif($elems[0] =~ /%(sp)$/)  {
					$arch_regname = "o6";
					$arch_reg = arch_reg_name2num($arch_regname);
				}
			} elsif( $agen_reg eq "agen") {
				if($elems[0] =~ /^\[.*\]$/)  {
					$arch_reg = 1; # anything other than -1
				}
			}

			$high = $elems[1];
			$low = 0; 
			$low = trim($low);
			$high = trim($high);
			if($elems[3] =~ /([01]+)/) {
				$src_dest = $1;
			}

			if($arch_reg == -1  || $src_dest == -1) {
					next;
			} else {
				$found = 1;
			}

			if( $agen_reg eq "agen") {
				$src_dest = -1;
			}
			if($found == 1) { # if arch register is found in source or dest
				# get pilot list for leading branch
				$pilot_list = "";
				$pilot_list = $control_pilot_map{$dep_pc} if exists $control_pilot_map{$dep_pc};
				@pilots = split(/ /,$pilot_list);
				foreach $pilot (@pilots) {
					if($pilot eq "") {
							next;
					}
					$pilot = trim($pilot);
					#print "$pilot_population_map{$pilot} : $cutoff_pop\n";
					my $selected_pop = $pilot_population_map{$pilot};
					if ($pilot_population_map{$pilot} < $cutoff_pop) {
# TODO: UNcomment this line!!!!
							next;
					}

					my $adjusted_pilot = adjust_cycle($pilot, $diff_cycle);
					if ($adjusted_pilot != $pilot) {
							print "Adjustement done to pilot $pilot at $dep_pc \n";
					}
				
					my $sample_list = "";
					$sample_list = $control_sample_map{$pilot} if exists $control_sample_map{$pilot};
					my @samples = split(/ /, $sample_list);
					#print "allowed pop = $selected_pop: $#samples\n";
					if($#samples >= 2)  {
						##### check for prob
						$rand_num  = rand();
						if($rand_num > $prob) {
# TODO: UNcomment this line!!!!
							next;
						}
						print "$pc\n";

						#print "Selected pop = $selected_pop\n";
						#print "$adjusted_pilot\n";


						if($num_pilots < 2000) {

#							write simics file for pilot 
							my $c = 0;
							for($i = $low; $i < $high; $i=$i+$BIT_GAP) {
								$simics_file_counter += 1;
								$num_pilots += 1;
# 								cycle, arch_reg, bit
								my $simics_filename = "$app_name.$pc.$adjusted_pilot.$c.$arch_reg.$i.$src_dest.$fault_type";
								if($write_files == 1) {
									print SIMICS_FILE_LIST "$simics_filename\n";
								} else {
									#print "$simics_filename\n";
								}
							}


							my $s = 1;
							foreach $cycle (@samples) {
								if($cycle ne "") {
									$adjusted_cycle = adjust_cycle($cycle,$diff_cycle);
									if($adjusted_cycle == -1) {
										print "adjusted cycle = -1 : $cycle, $diff_cycle, $pc";
									}
									for ($i=$low; $i<$high;$i+=$BIT_GAP) {
										$simics_file_counter += 1;
#									    cycle, arch_reg, bit
										my $simics_filename = "$app_name.$pc.$adjusted_cycle.$s.$arch_reg.$i.$src_dest.$fault_type";
									    if($write_files == 1) {
									   		print SIMICS_FILE_LIST "$simics_filename\n";
									    } else {
											#print "$simics_filename\n";
										}
									}
									$s += 1;
								}
							}
						}
					}
				}
			}
		} 
	}
}
close IN_STATIC;

print "Number of pilots: $num_pilots\n";
print "Number of simics files: $simics_file_counter \n";


close SIMICS_FILE_LIST;
