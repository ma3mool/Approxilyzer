#!/usr/bin/perl

use Cwd;

local $| = 1;

my $write_files = 1;

my $numArgs = $#ARGV + 1;
if($numArgs != 6) {
	print "Usage: perl gen_random_simics_files.pl <app_name> <static_file_name> <mode_change_log> <heap_patterns_log> <stack_patterns_log> <control_patterns_log>\n";  
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
my $control_patterns_file_name = $ARGV[5];
#my $store_patterns_file_name = $ARGV[3];

my $prune_type = "random";

my $pwd = cwd();
my $simics_dir  = "$pwd/optimized_simics_files/$app_name\_$prune_type\_simics_files/";
print "$simics_dir \n";
if($app_name eq "blackscholes") {
    $app_name = "blackscholes_simlarge";
    #$app_name = "blackscholes_abdul";
}
if($app_name eq "fluidanimate") {
	$app_name = "fluidanimate_simsmall";
}
if($app_name eq "streamcluster") {
	$app_name = "streamcluster_simsmall";
}
if($app_name eq "swaptions") {
	$app_name = "swaptions_simsmall";
}
if($app_name eq "lu") {
	$app_name = "lu_reduced";
}
if($app_name eq "fft") {
	$app_name = "fft_small";
}
if($app_name eq "ocean") {
	$app_name = "ocean_small";
}
if($app_name eq "water") {
	$app_name = "water_small";
}
if($app_name eq "libquantum") {
	$app_name = "libquantum_test";
}
if($app_name eq "omnetpp") {
	$app_name = "omnetpp_test";
}
if($app_name eq "mcf") {
	$app_name = "mcf_test";
}
if($app_name eq "gcc") {
	$app_name = "gcc_test";
}

# Perl trim function to remove whitespace from the start and end of the string
sub trim($)
{
	my $string = shift;
	$string =~ s/^\s+//;
	$string =~ s/\s+$//;
	return $string;
}

sub arch_reg_name2num($)
{
	my $arch_regname = shift;
	my $class = -1;
	my $reg_num = -1;
	if($arch_regname =~ /([goli])([0-9]*)/) {
		$class = $1;
		$reg_num = $2;
	}
	if($class eq 'g') {
		$logical = 0;
	} elsif($class eq 'o') {
		$logical = 8;
	} elsif($class eq 'l') {
		$logical = 16;
	} elsif($class eq 'i') {
		$logical = 24;
	}
	$logical += $reg_num;
	#print "arch2logical: $arch_regname -> $logical\n";

	return $logical;
}

my %pilot_population_map = ();

my %control_pilot_map = (); # pc->pilots
my %control_sample_map = (); # pilot->samples
open IN_CONTROL, $control_patterns_file_name or die "$!: $control_patterns_file_name";
while (<IN_CONTROL>) {
	my $d_line = $_;
	chomp($d_line);
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
}
close IN_CONTROL;

my %store_pilot_map = (); # pc->pilots
my %store_sample_map = (); # pilot->samples
open IN_STORE, $heap_patterns_file_name or die "$!: $heap_patterns_file_name";
while (<IN_STORE>) {
	$d_line = $_;
	chomp($d_line);
	if($d_line ne "") {
		my @d_words = split (/:/, $d_line);
		$d_words[2] = trim($d_words[2]);
		$d_words[1] = trim($d_words[1]);
		$d_words[0] = trim($d_words[0]);
		if($d_words[2] != 0) {
			my $priv_cycles = "";
			if(exists $store_pilot_map {$d_words[0]}) { # d_words[0] is pc
				$priv_cycles = $store_pilot_map {$d_words[0]};
			} 
			$store_pilot_map {$d_words[0]} = "$d_words[1] $priv_cycles"; # d_words[1] is pilot
			$store_sample_map {$d_words[1]} = $d_words[3]; # d_words[3] has samples
			$pilot_population_map {$d_words[1]} = $d_words[2];
		}
	}
}
close IN_STORE;

open IN_STORE, $stack_patterns_file_name or die "$!: $stack_patterns_file_name";
while (<IN_STORE>) {
	$d_line = $_;
	chomp($d_line);
	if($d_line ne "") {
		my @d_words = split (/:/, $d_line);
		$d_words[2] = trim($d_words[2]);
		$d_words[1] = trim($d_words[1]);
		$d_words[0] = trim($d_words[0]);
		if($d_words[2] != 0) {
			my $priv_cycles = "";
			if(exists $store_pilot_map {$d_words[0]}) { # d_words[0] is pc
				$priv_cycles = $store_pilot_map {$d_words[0]};
			} 
			$store_pilot_map {$d_words[0]} = "$d_words[1] $priv_cycles"; # d_words[1] is pilot
			$store_sample_map {$d_words[1]} = $d_words[3]; # d_words[3] has samples
			$pilot_population_map {$d_words[1]} = $d_words[2];
		}
	}
}
close IN_STORE;


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

	my $debug = 0;
	if($cycle == -1) {
		$debug = 1;
	}
	if($debug == 1) {
		print "$cycle, $diff_cycle\n";
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
					if($debug == 1) {
						print "adjustment:$num_adjustments - $cycle\n";
					}
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

my @population_arr;
my %population_sample_map = ();
my $index = 0;
open (SIMICS_FILE_LIST, ">simics_file_list/simics_file_list_$app_name\_$prune_type.txt");

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
	if ($dep_type == 0 || $dep_type == 1 || $dep_type == 2 || $dep_type == 10) { 
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
			if($elems[0] =~ /%([gloi][0-9]*)$/)  {
				$arch_regname = $1;
				$arch_reg = arch_reg_name2num($arch_regname);
			}elsif($elems[0] =~ /%([gloi][0-9]*)\]$/)  {
				$arch_regname = $1;
				$arch_reg = arch_reg_name2num($arch_regname);
			}elsif($elems[0] =~ /%fp$/)  {
				$arch_regname = "i6";
				$arch_reg = arch_reg_name2num($arch_regname);
			}elsif($elems[0] =~ /%sp$/)  {
				$arch_regname = "o6";
				$arch_reg = arch_reg_name2num($arch_regname);
			}

			$low = 0;
			$high = $elems[1];
			$high = trim($high);
			if($elems[3] =~ /([01]+)/) {
				$src_dest = $1;
			}

			if($arch_reg == -1  || $src_dest == -1) {
					next;
			} else {
				$found = 1;
			}

			if($found == 1) { # if arch register is found in source or dest
				# get pilot list for leading branch
				$pilot_list = "";
				$pilot_list = $control_pilot_map{$dep_pc} if exists $control_pilot_map{$dep_pc}; # only one of the control_pilot or the store_pilot
				$pilot_list = $store_pilot_map{$dep_pc} if exists $store_pilot_map{$dep_pc};
				@pilots = split(/ /,$pilot_list);
				foreach $pilot (@pilots) {
					if($pilot eq "") {
						next;
					}
					$population_arr[$index] = -1;
					$population_arr[$index] = $pilot_population_map {$pilot} if exists $pilot_population_map {$pilot};
					$population_arr[$index] = $population_arr[$index]*$high; 
					$index++;

					my $sample_list = "";
					$sample_list = $store_sample_map{$pilot} if exists $store_sample_map{$pilot};
					$sample_list = $control_sample_map{$pilot} if exists $control_sample_map{$pilot};
					my @samples = split(/ /, $sample_list);
					foreach $cycle (@samples) {
						if($cycle ne "") {
							my $adjusted_cycle = adjust_cycle($cycle,$diff_cycle);
							if($adjusted_cycle == -1) {
								print "adjusted cycle = -1 : $cycle, $diff_cycle, $pc";
							}
							if(exists $population_sample_map {$population_arr[$index-1]}) { 
								$population_sample_map {$population_arr[$index-1]} .= " $adjusted_cycle:$pc:$src_dest:$arch_reg:$high"; 
							} else {
								$population_sample_map {$population_arr[$index-1]} = "$adjusted_cycle:$pc:$src_dest:$arch_reg:$high"; 
							}
						}
					}
					print ".";
				}
			}
		} 
	}
}
close IN_STATIC;

my $total_pop = 0;
for ($i=0; $i<$index; $i++) {
	$total_pop += $population_arr[$i];
}

print "total pop = $total_pop\n";
my @weight ;
for ($i=0; $i<$index; $i++) {
	$weight[$i] = $population_arr[$i]/$total_pop;
}
my @cum_weight ;
$cum_weight[0] = $weight[0];
for ($i=1; $i<$index; $i++) {
	$cum_weight[$i] = $weight[$i]+$cum_weight[$i-1];
}

my @random_array;
my $num_injections = 5000;
for ($i=0; $i<$num_injections-1; $i++) {
	#generate random number
	#find the pop 
	#find a randomly selected sample
	my $random_number = rand();
	$random_array[$i] = $random_number;
}
@random_array  = sort @random_array;
$random_number = $random_array[0];
$i=1;
for ($j=0; $j<$index; $j++) {
	my $selected_pop_index = -1;
	if($random_number < $cum_weight[$j]) {
		$selected_pop_index = $j;
		$j--;

		$random_number = $random_array[$i];
		$i++;
		if($i>4999) {
			last;
		}

		#print "selected pop index = $selected_pop_index \n";
		#print "selected pop = $population_arr[$selected_pop_index] \n";
		my $temp_sample_list = $population_sample_map { $population_arr[$selected_pop_index] };
		my @temp_samples = split(/ /,$temp_sample_list);
	
		$new_random_number = rand($#temp_samples);
		my $selected_fault = $temp_samples[$new_random_number];
	
		my @fault_info = split(/:/,$selected_fault);
		my $cycle = $fault_info[0];
		my $pc = $fault_info[1];
		my $src_dest = $fault_info[2];
		my $arch_reg = $fault_info[3];
		my $high = $fault_info[4];
		my $bit = int(rand($high));
		#my $simics_filename = "$simics_dir/$app_name.pc$pc.p$cycle.s$i.r$arch_reg.b$bit.sd$src_dest.simics";
		my $simics_filename = "$app_name.$pc.$cycle.$i.$arch_reg.$bit.$src_dest";
		if($write_files == 1) {
			print SIMICS_FILE_LIST "$simics_filename\n";
			open OUT_SIMICS_FILE, ">$simics_filename";
			print OUT_SIMICS_FILE "\@sys.path.append(\".\")\n";
			print OUT_SIMICS_FILE "\@import simpoint\n";
			print OUT_SIMICS_FILE "\@simpoint.run_new_all(\'$app_name\',\'p0\',\'s$i\',\'$pc\',0,4,$bit,4,$arch_reg,$src_dest,$cycle,$cycle)\n";
			print OUT_SIMICS_FILE "quit\n";
			close OUT_SIMICS_FILE;
		} else {
			#print "$simics_filename\n";
		}
		print ".";
	}
}
	
close SIMICS_FILE_LIST;
