#!/usr/bin/perl

use Cwd;
use sort 'stable';
use lib 'perl/';
use common_subs;

my $BIT_GAP = 8;
my $write_files = 1;

my $numArgs = $#ARGV + 1;
if($numArgs != 6) {
	print "Usage: perl gen_store_simics_files.pl <app_name> <static_file_name> <mode_change_log> <heap_patterns_log> <stack_patterns_log> <agen/reg>\n";  
	print "$ARGV[0] \n";
	print "$ARGV[1] \n";
	print "$ARGV[2] \n";
	print "$ARGV[3] \n";
	print "$ARGV[4] \n";
	print "$ARGV[5] \n";
	die;
}
my $app_name = $ARGV[0];
my $static_file_name = $ARGV[1];
my $mode_change_file_name = $ARGV[2];
my $heap_patterns_file_name = $ARGV[3];
my $stack_patterns_file_name = $ARGV[4];
my $agen_reg = $ARGV[5];

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
open (SIMICS_FILE_LIST, ">new_store_simics_file_list/simics_file_list_$app_name\_$prune_type\_$agen_reg.txt");

my $total_pilots = 0;
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
	my $dep_type = $words[2];
	$dep_type = int($dep_type);
	my $fault_list = $words [4];
	my $dep_pc = "";
	if($words[3] =~ /\{([0-9a-z]*)\, (-*[0-9]+)\}/) {
		$dep_pc = $1;
	}

	if ($dep_type == 0) { # STORE
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
			$low = $elems[2];
			$low = trim($low);
			$high = trim($high);
			if($elems[3] =~ /([01]+)/) {
				$src_dest = $1;
			}

			if($arch_reg == -1  || $src_dest == -1) {
					next;
			} else {

				$pilot_list = "";
				$pilot_list = $store_pilot_map{$dep_pc} if exists $store_pilot_map{$dep_pc};
				@pilots = split(/ /,$pilot_list);
				foreach $pilot (@pilots) {
					if($pilot eq "") {
							next;
					}
					$pilot = trim($pilot);
					# if($temp_count != 0) {
					# 	my $temp_val = $population_list[$temp_count];
					# 	my $temp_val1 = $pilot_population_map{$pilot};
					# 	print "\n -$temp_val- -$temp_val1- -$pilot-";
					# }
					$population_list [$temp_count++] = $pilot_population_map{$pilot};
					$total_pilots = $total_pilots + 1;
				}
			}
		} 
	}
}
close IN_STATIC;

print "Total number of pilots = $total_pilots\n";
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
	if($temp_count >= 10000) {
		last;
	}
	$pop_sum += $pop;
}
$cutoff_pop = $sorted_pop_list[$temp_count-1];

print "cutoff cleared = $temp_count\n";
my $prob = 1; 
if ($temp_count >= 2) {
	if($agen_reg eq "reg") {
	 	$prob = 5.0/$temp_count;
	} elsif($agen_reg eq "agen") {
		$prob = 5.0/$temp_count;
	}
}

print "prob = $prob\n";


open IN_STATIC, $static_file_name or die "$!: $static_file_name";
while (<IN_STATIC>) { # for every line
	my $line = $_;

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
	if ($dep_type == 0) { # STORE
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

			$high = $elems[1];
			$low = $elems[2];
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
				$pilot_list = $store_pilot_map{$dep_pc} if exists $store_pilot_map{$dep_pc};
				@pilots = split(/ /,$pilot_list);
				foreach $pilot (@pilots) {
					if($pilot eq "") {
							next;
					}

					$pilot = trim($pilot);
					#print "$pilot_population_map{$pilot} : $cutoff_pop\n";
					my $selected_pop = $pilot_population_map{$pilot};
					if ($selected_pop < $cutoff_pop) {
							next;
					}

					my $adjusted_pilot = adjust_cycle($pilot, $diff_cycle);

					#print "$adjusted_pilot\n";

					$sample_list = "";
					$sample_list = $store_sample_map{$pilot} if exists $store_sample_map{$pilot};
					@samples = split(/ /, $sample_list);

					if($#samples >= 2)  {
						##### check for prob
						$rand_num  = rand();
						if($rand_num > $prob) {
							next;
						}

#						write simics file for pilot 
						my $c = 0;
						for($i = $low; $i < $high; $i=$i+$BIT_GAP) {
							$simics_file_counter += 1;
							$num_pilots += 1;
# 							cycle, arch_reg, bit
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
#								    cycle, arch_reg, bit
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
close IN_STATIC;

print "Number of pilots: $num_pilots\n";
print "Number of simics files: $simics_file_counter \n";

close SIMICS_FILE_LIST;
