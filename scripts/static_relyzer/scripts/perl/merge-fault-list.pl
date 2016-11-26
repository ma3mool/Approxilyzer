#!/usr/bin/perl

use Cwd;
#use lib '/home/sadve/shari2/research/relyzer/perl/';
use lib '/shared/workspace/amahmou2/relyzer/perl/';
use common_subs;

my $use_random_pilot = 1;

my $numArgs = $#ARGV + 1;
if($numArgs != 7) {
	print "Usage: perl merge-fault-list.pl <program_name> <static_file_name> <dynamic_heap_file_name> <dynamic_stack_file_name> <output_file_name> <mode_change_log> <control_patterns_log>\n"; 
	die;
}
my $program_name = $ARGV[0];
my $static_file_name = $ARGV[1];
my $dynamic_heap_file_name = $ARGV[2];
my $dynamic_stack_file_name = $ARGV[3];
my $output_file_name = $ARGV[4];
my $mode_change_file_name = $ARGV[5];
my $control_patterns_file_name= $ARGV[6];

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

#my $sample_size = 16641; # sample size for 99% confidence level for an error of 1%
#my $sample_size = 666; # sample size for 99% confidence level for an error of 5%
#my $sample_size = 13573; # sample size for 98% confidence level for an error of 1%
#my $sample_size = 543; # sample size for 98% confidence level for an error of 5%
#my $sample_size = 9604; # sample size for 95% confidence level for an error of 1%
#my $sample_size = 385; # sample size for 95% confidence level for an error of 5%



# For obtaining combined prediction rate

my @prediction_rate = ();

my $app_name = $program_name;
if($app_name eq "blackscholes_abdul") {
	@prediction_rate = (98.04,	99.28,	92.49,	89.02); 
}
if($app_name eq "blackscholes") {
	@prediction_rate = (98.04,	99.28,	92.49,	89.02); 
}
if($app_name eq "fluidanimate") {
	@prediction_rate = (99.94,	94.93,	99.60,	93.55);
}
if($app_name eq "streamcluster") {
	@prediction_rate = (99.81,	93.48,	99.85,	92.94);
}
if($app_name eq "swaptions") {
	@prediction_rate = (94.00,	99.55,	95.13,	95.12);
}
if($app_name eq "lu") {
	@prediction_rate = (100.0,	99.81,	99.66,	93.97);
}
if($app_name eq "fft") {
	@prediction_rate = (91.36,	98.10,	98.51,	98.35);
}
if($app_name eq "ocean") {
	@prediction_rate = (88.39,	86.79,	84.18,	82.63);
}
if($app_name eq "water") {
	@prediction_rate = (99.38,	96.89,	97.93,	99.13);
}
if($app_name eq "libquantum") {
	@prediction_rate = (93.48,	93.90,	98.28,	98.89);
}
if($app_name eq "omnetpp") {
	@prediction_rate = (93.35,	96.29,	83.85,	85.70);
}
if($app_name eq "mcf") {
	@prediction_rate = (95.16,	89.66,	97.02,	85.98);
}
if($app_name eq "gcc") {
	@prediction_rate = (95.27,	96.07,	87.79,	96.41);
}

# 0 - reg-control
# 1 - reg-store
# 2 - agen-control
# 3 - agen-store

my @fault_weights = (0,0,0,0);

#read top of bb.h
sub dep_type_string() {
	my $dep_type = $_;
	if ($dep_type == 0) { 
		return "STORE";
	} elsif ($dep_type == 1) { 
		return "CONTROL";
	} elsif ($dep_type == 2) { 
		return "LIVE";
	} elsif ($dep_type == 10) { 
		return "COMPARE";
	} elsif ($dep_type >= 3) { 
		return "OTHERS";
	} 

	return "";
}

open IN_STATIC, $static_file_name or die "$!: $static_file_name";
open OUT_FILE, ">$output_file_name" or die "$!: $output_file_name";

$NUM_CATS = 4;
my @pruned_all_categories;
my @pruned_reg_categories;
my @pruned_int_categories;
my @pruned_int_agen_categories;
my @categories;
my @int_categories;
for ($local_i = 0; $local_i < $NUM_CATS; $local_i++) {
	$all_categories[$local_i] = 0;
	$reg_categories[$local_i] = 0;
	$int_categories[$local_i] = 0;

	$pruned_all_categories[$local_i] = 0;
	$pruned_reg_categories[$local_i] = 0;
	$pruned_int_categories[$local_i] = 0;
	$pruned_int_agen_categories[$local_i] = 0;
}
my $total_merged_all_count = 0;
my $total_merged_reg_count = 0;
my $total_merged_int_count = 0;
my $total_all_count = 0;
my $total_reg_count = 0;
my $total_int_count = 0;
my $line = "";

#open BRK_FILE, ">>brk_file.txt" ;

my $num_store = 0;
my $num_control = 0;

$time1=time();
my %dynamic_heap_map = ();
open IN_DYNAMIC_HEAP, $dynamic_heap_file_name or die "$!: $dynamic_heap_file_name";
while (<IN_DYNAMIC_HEAP>) {
	my $d_line = $_;
	chomp($d_line);
	if($d_line ne "") {
		my @d_words = split (/:/, $d_line);
		$dynamic_heap_map {$d_words[0]} = $d_words[1];

	}
}
close IN_DYNAMIC_HEAP;

# $time2 = time();
# $diff = $time2 - $time1;
# print "read dynamic heap: $diff\n";

my %dynamic_stack_map = ();
open IN_DYNAMIC_STACK, $dynamic_stack_file_name or die "$!: $dynamic_stack_file_name";
while (<IN_DYNAMIC_STACK>) {
	my $d_line = $_;
	chomp($d_line);
	if($d_line ne "") {
		my @d_words = split (/:/, $d_line);
		$dynamic_stack_map {$d_words[0]} = $d_words[1];
	}
}
close IN_DYNAMIC_STACK;

my %control_map = ();
open IN_CONTROL, $control_patterns_file_name or die "$!: $control_patterns_file_name";
while (<IN_CONTROL>) {
	my $d_line = $_;
	chomp($d_line);
	if($d_line ne "") {
		my @d_words = split (/:/, $d_line);
		if (!(defined $d_words[1])) {
			$d_words[1] = "";
		}
		my $priv_cycles = "";
		if(exists $control_map {$d_words[0]}) {
			$priv_cycles = $control_map {$d_words[0]};
			print "you should not see this!! pc=$d_words[0]-\n";
		} 
		$control_map {$d_words[0]} = "$d_words[1] $priv_cycles";
	}
}
close IN_CONTROL;


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

my $total_pr = 0.0;
my $total_pr_denom = 0.0;

$static_line_no = 0;
while (<IN_STATIC>) {
	$line = $_;
	# if($static_line_no%100 == 0) {
	# 	$time2 = time();
	# 	$diff = $time2 - $time1;
	#  	print "$static_line_no:$diff \n";
	# }
	$static_line_no++;
	# print "$static_line_no\n";

	my $found = 0;
	my $cycle_list = "";
	# frequecy[]:pc:dep_type:{store_pc}:faults
	# 0- frequency
	# 1 - pc
	# 2 - dep_type
	# 3 - dep_pc
	# 4 - faults
	my @words = split (/:/, $line);
	my $frequency =  $words[0];
	my $count = 0;
	if($frequency =~ /(.*)\[\]/) {
		$count = $1;
	}
	my $pc = $words[1];
	my $dep_type = $words[2];
	$dep_type = int($dep_type);
	#print "dep_type = -$dep_type-\n";
	my $faults = $words [4]; 
	my $dep_pc = "";
	my $diff_cycle = "";
	if($words[3] =~ /\{([0-9a-z]*)\, (-*[0-9]+)\}/) {
		$dep_pc = $1;
		$diff_cycle = $2;
	}

	# get number of bits in the fault
	my $all_bits = 0;
	my $int_agen_bits = 0;
	my $reg_bits = 0;
	my $int_bits = 0;
	@fault_list = split(/ \(/, $faults);
	# fault list : (reg, max_bits, is_address, is_def, extra_count) separated by spaces
	foreach (@fault_list) {
		$fault = $_;
		chomp($fault);
		if($fault ne "") {
			if ($fault =~ /(.*)\)$/) {
				$fault = $1;
			}
			# print "$fault\n";
			my @elements = split(/\, /, $fault);
			$all_bits = $all_bits + $elements[1];
			if(&common_subs::is_fp_register( $elements[0] ) == 0) { # if not fp register
				$int_agen_bits = $int_agen_bits + $elements[1];
			}
			if(&common_subs::is_int_register( $elements[0] ) == 1) {
				$int_bits = $int_bits + $elements[1];
			}
			if(&common_subs::is_register( $elements[0] ) == 1) {
				$reg_bits = $reg_bits + $elements[1];
			}
		}
	}

	$total_all_count += $count*$all_bits;
	$total_reg_count += $count*$reg_bits;
	$total_int_count += $count*$int_bits;
	my $orig_count = $count;

	my $check = 0; # used to make $count to the sample size

	if($dep_type == 0)  { # STORE
		$cycle_list1 = "";
		$cycle_list1 = $dynamic_heap_map{$dep_pc} if exists $dynamic_heap_map{$dep_pc};

		$cycle_list2 = "";
		$cycle_list2 = $dynamic_stack_map{$dep_pc} if exists $dynamic_stack_map{$dep_pc};
		if($cycle_list1 eq "") {
			$cycle_list  = $cycle_list2;
		} elsif($cycle_list2 eq "") {
			$cycle_list  = $cycle_list1;
		} else {
			$cycle_list1 = common_subs::trim($cycle_list1);
			$cycle_list  = "$cycle_list1 $cycle_list2";
			#print "$cycle_list\n";
		}
		#print "$dep_pc: $cycle_list\n";
		@cycles = split(/ /, $cycle_list);

		# print "$count : $#cycles\n";
		if($#cycles != -1) {
			$count = $#cycles;
		} else {
			print " store not found pc=$pc, dep_pc=$dep_pc: $count\n";
			$count = 0;
		}
		$num_store++;
	} elsif ($dep_type == 1) { # CONTROL
		$cycle_list = "empty";
		$cycle_list = $control_map{$dep_pc} if exists $control_map{$dep_pc};
		if ( $cycle_list ne "empty" ) {
			@cycles = split(/ /, $cycle_list);
			if($#cycles != -1) {
				$count = $#cycles;
			} else {
				print " control found pc=$pc, dep_pc=$dep_pc but count=0\n";
				$count = 0;
			}
		} else {
			print " control not found pc=$pc, dep_pc=$dep_pc -$cycle_list-\n";
			$count = 0;
			$cycle_list="";
		}
		$num_control++;

	} elsif ($dep_type == 2) { # LIVE
		$cycle_list = "empty";
		$cycle_list = $control_map{$dep_pc} if exists $control_map{$dep_pc};
		@cycles = split(/ /, $cycle_list);
		if ( $cycle_list ne "empty" ) {
			if($#cycles != -1) {
				$count = $#cycles;
			} else {
				print " live found at pc=$pc, dep_pc=$dep_pc but count=0\n";
				$count = 0;
			}
		} else {
			print " LIVE not found pc=$pc, dep_pc=$dep_pc -$cycle_list-\n";
			$count = 0;
			$cycle_list="";
		}
		$num_control++;
	} elsif ($dep_type == 3) { # CALL_SITE
		$count = 0;
	} elsif ($dep_type == 4) { # ILLTRAP
		$count = 0;
	} elsif ($dep_type == 5) { # RET_SITE
		$count = 0;
	} elsif ($dep_type == 6) { # SAVE_SITE
		$count = 0; 
	} elsif ($dep_type == 7) { # DEAD
		$count = 0; 
	} elsif ($dep_type == 8) { # Read and write state registers
		$count = 0; 
	} elsif ($dep_type == 9) { # STORE_AND_LIVE
		$cycle_list = "empty";
		$cycle_list = $control_map{$dep_pc} if exists $control_map{$dep_pc};
		if ( $cycle_list ne "empty" ) {
			@cycles = split(/ /, $cycle_list);
			if($#cycles != -1) {
				$count = $#cycles;
			} else {
				print " control found pc=$pc, dep_pc=$dep_pc but count=0\n";
				$count = 0;
			}
		} else {
			print " control not found pc=$pc, dep_pc=$dep_pc -$cycle_list-\n";
			$count = 0;
			$cycle_list="";
		}
		$num_control++;

	} elsif ($dep_type == 10) { # COMPARE
		$cycle_list = "empty";
		$cycle_list = $control_map{$dep_pc} if exists $control_map{$dep_pc};
		if ( $cycle_list ne "empty" ) {
			@cycles = split(/ /, $cycle_list);
			if($#cycles != -1) {
				$count = $#cycles;
			} else {
				print " compare found pc=$pc, dep_pc=$dep_pc but count=0\n";
				$count = 0;
			}
		} else {
			print " compare not found pc=$pc, dep_pc=$dep_pc -$cycle_list-\n";
			$count = 0;
			$cycle_list="";
		}
		$num_control++;
	} elsif ($dep_type == 11) { # Remaining
		$check = 1;
	} else { 
		# do this for best
		$check = 1;
		if($count > 1000000) {
			#$count = 100;
			# $str = &dep_type_string($dep_type);
			# print "$count: ";
			# print "$str:$cycle_list:$pc:$faults";
			# open DIS_FILE, "apps/optimized/$program_name/$program_name.dis" or die;
			# while(<DIS_FILE>) {
			# 	my $dis_line = $_;
			# 	if($dis_line =~ /$pc:/) {
			# 		#$count = 100;
		      	# 	print $dis_line;
			# 		last;
			# 	}
			# }
			# close DIS_FILE;

		}
	}

	if($check == 1) {
		if($count > 100) { 
			my $sample_size = ($count * $Z_2_p_1_p) / (($E_2*($count -1)) + $Z_2_p_1_p);
			$sample_size = int($sample_size+0.99);
			# $count = $sample_size;
		}
	}

	# branch instructions
	# my $is_branch = 0;
	# if($count > 10000 ) {
	# 	open DIS_FILE, "apps/$program_name/$program_name.dis" or die;
	# 	while(<DIS_FILE>) {
	# 		my $dis_line = $_;
	# 		if($dis_line =~ /$pc/) {
	# 			if($dis_line =~ /[0-9a-f]\s\sb/) {
	# 				$is_branch = 1;
	# 				#$count = 100;
	# 				last;
	# 			}
	# 		}
	# 	}
	# 	close DIS_FILE;
	# }

	# if($dep_type != 0) {
	# 	print BRK_FILE "list_pc_$program_name.append($pc)\n";
	# 	print BRK_FILE "list_sample_size_$program_name\.append($count)\n";
	# 	print BRK_FILE "list_population_$program_name.append($orig_count)\n";
	# }

	if( $dep_type==0 || $dep_type==1 || $dep_type == 2 || $dep_type == 10) { # CONTROL

		@fault_list = split(/ \(/, $faults);
		foreach (@fault_list) {
			$fault = $_;
			chomp($fault);
			if($fault ne "") {
				if ($fault =~ /(.*)\)$/) {
					$fault = $1;
				}
				# print "$fault\n";
				my @elements = split(/\, /, $fault);
				$all_bits = $all_bits + $elements[1];
	# 0 - reg-control
	# 1 - reg-store
	# 2 - agen-control
	# 3 - agen-store
				my $pr_index = -4;
				if($dep_type == 0)  { # STORE
					$pr_index = 1;
				} elsif($dep_type==1 || $dep_type == 2 || $dep_type == 10) { # CONTROL
					$pr_index = 0;
				}
				my $is_agen = 0;
				if($elements[0] =~ /^\[.*\]$/)  {
					$is_agen = 1; 
				}
				if($is_agen == 1) {
					$pr_index += 2;
				}
				$fault_weights[$pr_index] += 1; 
				$total_pr += ($prediction_rate[$pr_index]/100) * ($elements[1] + $elements[1]*$elements[4]);
				$total_pr_denom += ($elements[1] + $elements[1]*$elements[4]);
				$fault_weights[$pr_index] += $count*($elements[1] + $elements[1]*($elements[4]>=1));
				#$fault_weights[$pr_index] += $count*($elements[1]);
				#print "$total_pr / $total_pr_denom\n";
			}
		}
	}

	if($dep_type==0) {
		$all_categories[0] += ($count*$all_bits);
		$reg_categories[0] += ($count*$reg_bits);
		$int_categories[0] += ($count*$int_bits);

		$pruned_all_categories[0] += (($orig_count - $count)*$all_bits);
		$pruned_reg_categories[0] += (($orig_count - $count)*$reg_bits);
		$pruned_int_categories[0] += (($orig_count - $count)*$int_bits);
		$pruned_int_agen_categories[$dep_type] += (($orig_count - $count)*$int_agen_bits);
	} elsif($dep_type==1 || $dep_type == 2 || $dep_type == 10) {
		$all_categories[1] += ($count*$all_bits);
		$reg_categories[1] += ($count*$reg_bits);
		$int_categories[1] += ($count*$int_bits);

		$pruned_all_categories[1] += (($orig_count - $count)*$all_bits);
		$pruned_reg_categories[1] += (($orig_count - $count)*$reg_bits);
		$pruned_int_categories[1] += (($orig_count - $count)*$int_bits);
		$pruned_int_agen_categories[1] += (($orig_count - $count)*$int_agen_bits);
	} else {
		my $curr_cat = 2;
		if ($check == 1 ) { 
			$curr_cat = 3;
		} else {
			$curr_cat = 2;
		}
		$all_categories[$curr_cat] += ($count*$all_bits);
		$reg_categories[$curr_cat] += ($count*$reg_bits);
		$int_categories[$curr_cat] += ($count*$int_bits);

		$pruned_all_categories[$curr_cat] += (($orig_count - $count)*$all_bits);
		$pruned_reg_categories[$curr_cat] += (($orig_count - $count)*$reg_bits);
		$temp = (($orig_count - $count)*$int_bits)/1000000;
		$pruned_int_categories[$curr_cat] += (($orig_count - $count)*$int_bits);
		$pruned_int_agen_categories[$curr_cat] += (($orig_count - $count)*$int_agen_bits);
		if($dep_type < 3 || $dep_type > 8) {
			if ($temp > 1) {
				print "$pc: $dep_type : $count: ";
				print "$temp \n";
			}
		}
	}

	$total_merged_all_count += $count*$all_bits;
	$total_merged_reg_count += $count*$reg_bits;
	$total_merged_int_count += $count*$int_bits;
	# use this print to check any discripancy in the no. remaining faults
	# print "$pc:$total_merged_int_count\n";

	my $new_cycle_list = "";
	if( $cycle_list ne "") {
		$cycle_list = common_subs::trim($cycle_list);
		@cycles = split(/ /, $cycle_list);
		foreach $cycle (@cycles) {
			if($cycle eq "") {
				next;
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
						#print "start=$start, end=$end, middle_point=$middle_point: $cycle:$range[$middle_point][1]\n";
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

						# my $m_line = $mode_file[$i];
						# chomp($m_line);
						# my @range = split (/-/, $m_line);
						# if($range[0] > $cycle) {
						# 	if($pc eq "0x100002848") {
						# 		print "$cycle: {$range[0]}\n";
						# 	}
						# 	last;
						# }
						# if($cycle > $range[0] && $cycle < $range[1])  { # cycle falls in the range of priv mode
						# 	$cycle = $cycle - ($range[1]-$range[0]) - 1;
						# 	$adjusted = 1;  # cycle is adusted in this iteration 
						# 	$num_adjustments++;
						# 	if($pc eq "0x100002848") {
						# 		print "$cycle: {$range[0], $range[1]}\n";
						# 	}
						# } 
					}
					if($adjusted == 0) { # check if the cycle is adusted in this iteration
						$need_adjustment = 0; # if not then no need to check again
					}
				}

				if($num_adjustments > 0) {
					#$cycle = $cycle - $num_adjustments;
					# print "$temp_cycle -> $cycle\n";
				}
			}
	
			$new_cycle_list = $new_cycle_list . " $cycle";
		}
	}
	if($count*$all_bits > 1000000) {
		#print "$dep_type:$count:$pc:$faults";
	}

	print OUT_FILE "$dep_type:$count:$new_cycle_list:$pc:$faults";	
}
close IN_STATIC;
close OUT_FILE;

$time2 = time();
$diff = $time2 - $time1;
print "Total time: $diff \n";


$total_all_count = $total_all_count/1000000;
$total_reg_count = $total_reg_count/1000000;
$total_int_count = $total_int_count/1000000;
$total_merged_all_count = $total_merged_all_count/1000000;
$total_merged_reg_count = $total_merged_reg_count/1000000;
$total_merged_int_count = $total_merged_int_count/1000000;

print "$total_all_count M -> $total_merged_all_count M\n";
print "$total_reg_count M -> $total_merged_reg_count M\n";
print "$total_int_count M -> $total_merged_int_count M\n";

open DUMP_FILE, ">>dyn_all_pruned.txt";
print  DUMP_FILE "$total_merged_all_count\n";
close DUMP_FILE;
open DUMP_FILE, ">>dyn_reg_pruned.txt";
print  DUMP_FILE "$total_merged_reg_count\n";
close DUMP_FILE;
open DUMP_FILE, ">>dyn_int_pruned.txt";
print  DUMP_FILE "$total_merged_int_count\n";
close DUMP_FILE;

open CAT_FILE, ">>pruned_all_cat_file.txt" ;
print CAT_FILE "$program_name\t";
for ($local_i = 0; $local_i < $NUM_CATS; $local_i++) {
	print CAT_FILE "$pruned_all_categories[$local_i]\t";
}
print CAT_FILE "\n";
close CAT_FILE;

open CAT_FILE, ">>pruned_reg_cat_file.txt" ;
print CAT_FILE "$program_name\t";
for ($local_i = 0; $local_i < $NUM_CATS; $local_i++) {
	print CAT_FILE "$pruned_reg_categories[$local_i]\t";
}
print CAT_FILE "\n";
close CAT_FILE;

open CAT_FILE, ">>pruned_int_cat_file.txt" ;
print CAT_FILE "$program_name\t";
for ($local_i = 0; $local_i < $NUM_CATS; $local_i++) {
	print CAT_FILE "$pruned_int_categories[$local_i]\t";
}
print CAT_FILE "\n";
close CAT_FILE;

open CAT_FILE, ">>pruned_int_agen_cat_file.txt" ;
print CAT_FILE "$program_name\t";
for ($local_i = 0; $local_i < $NUM_CATS; $local_i++) {
	print CAT_FILE "$pruned_int_agen_categories[$local_i]\t";
}
print CAT_FILE "\n";
close CAT_FILE;
#
#
open CAT_FILE, ">>all_cat_file.txt" ;
print CAT_FILE "$program_name\t";
for ($local_i = 0; $local_i < $NUM_CATS; $local_i++) {
	print CAT_FILE "$all_categories[$local_i]\t";
}
print CAT_FILE "\n";
close CAT_FILE;
open CAT_FILE, ">>reg_cat_file.txt" ;
print CAT_FILE "$program_name\t";
for ($local_i = 0; $local_i < $NUM_CATS; $local_i++) {
	print CAT_FILE "$reg_categories[$local_i]\t";
}
print CAT_FILE "\n";
close CAT_FILE;
open CAT_FILE, ">>int_cat_file.txt" ;
print CAT_FILE "$program_name\t";
for ($local_i = 0; $local_i < $NUM_CATS; $local_i++) {
	print CAT_FILE "$int_categories[$local_i]\t";
}
print CAT_FILE "\n";
close CAT_FILE;
#close BRK_FILE;

open CAT_FILE, ">>static_purning_stats.txt" ;
print CAT_FILE "$num_store\t$num_control\n";
close CAT_FILE; 

my $total_pr_f = $total_pr/$total_pr_denom;
print "$total_pr_f\t$total_pr\t$total_pr_denom\n";
print "$fault_weights[0]\t$fault_weights[1]\t$fault_weights[2]\t$fault_weights[3]\n";

