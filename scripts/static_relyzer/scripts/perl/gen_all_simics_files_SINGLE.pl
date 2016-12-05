#!/usr/bin/perl

use Cwd;
use FileHandle;
#use lib '/home/sadve/shari2/research/relyzer/perl';
#use lib '/shared/workspace/amahmou2/relyzer/perl';
use lib "$ENV{APPROXILYZER}/scripts/static_relyzer/scripts/perl"; #TODO MAKE SCALABLE
use common_subs;

my $BIT_GAP = 1;
my $write_files = 1; # 0 means do not overwrite existing file.. should make input to file eventually TODO
my $use_random_selection = 1;
my $additional_pilots = 1; # 1=no additional pilots, 10=9 more pilots per eq. group. Should NEVER be 0
my $actual_app_coverage = 1;
my $metric_pc_based = 0;

my $numArgs = $#ARGV + 1;
if($numArgs != 7) {
	print "Usage: perl gen_all_simics_files.pl <app_name> <static_file_name> <mode_change_log> <heap_patterns_log> <stack_patterns_log> <control_patterns_log> <agen/reg/both>\n";  
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
my $control_patterns_file_name = $ARGV[5];
my $agen_reg = $ARGV[6];

# 	Static file name: $static_file_name 
# 	Mode change file: $mode_change_file_name
# 	Heap patterns: $heap_patterns_file_name
# 	Stack patterns: $stack_patterns_file_name
# 	Control patterns: $control_patterns_file_name
# 	Fault model: $agen_reg \n";

my $prune_type = "all";

my $pwd = cwd();
$app_name = common_subs::get_actual_app_name($app_name);
print "\nActual app name: $app_name \n";


# Loading metrics file 
# Format: PC \t sdc \t life \t fanout \t av.life \t av.fan \t di \t metrics_covered
%fanout_map = ();
%lifetime_map = ();
%av_fanout_map = ();
%av_lifetime_map = ();
%dyn_inst_map = ();
$fanout_count = 0;
$lifetime_count = 0;
$av_fanout_count = 0;
$av_lifetime_count = 0;
$dyn_inst_count = 0;

if ($metric_pc_based == 1) {
	my $metrics_filename ="/home/sadve/venktgr2/from_collab/simulator/GEMS/simics/home/dynamic_relyzer/veena_dynamic_profile_output/correct_life_def/metric_results/live_across_save_result/sort/$app_name\_tot_life_sort.txt";
	open IN_METRICS_FILE, $metrics_filename or die "$!: $metrics_filename";
	while (<IN_METRICS_FILE>) {
		my $m_line = $_;
		chomp($m_line);
		if($m_line ne "") {
			@words = split(/\t/, $m_line);
			if ($#words == 7 && $words[0] =~ /1000/) {
	
				$lifetime_map{"0x$words[0]"} = $words[2];
				$lifetime_count += $words[2];
	
				$fanout_map{"0x$words[0]"} = $words[3];
				$fanout_count += $words[2];
	
				$av_lifetime_map{"0x$words[0]"} = $words[4];
				$av_lifetime_count += $words[4];
	
				$av_fanout_map{"0x$words[0]"} = $words[5];
				$av_fanout_count += $words[5];
	
				$dyn_inst_map{"0x$words[0]"} = $words[6];
				$dyn_inst_map += $words[6];
	
				# print "\n";
				# foreach $word (@words) {
				# 	print "$word ";
				# }
			}
		}
	}
	close IN_METRICS_FILE;
}

sub get_metric_pc_list {
	my $coverage = $_[0];

	my $total_cov = 0;
	my %pc_map = ();

# %fanout_map = ();
	foreach $key (sort { $fanout_map{$b} <=> $fanout_map{$a} } keys %fanout_map) {
        $total_cov += $fanout_map{$key};
		if ($total_cov > $coverage) {
			last;
		}
		if (!exists $pc_map{$key}) {
            #print "$key:$fanout_map{$key}\n";
			$pc_map{$key} = 1; 
            #print "$key: $pc_map{$key}\n";
		}
	}
# %lifetime_map = ();
	$total_cov = 0;
	foreach $key (sort { $lifetime_map{$b} <=> $lifetime_map{$a} } keys %lifetime_map) {
		$total_cov += $lifetime_map{$key};
		if ($total_cov > $coverage) {
			last;
		}
		if (!exists $pc_map{$key}) {
			$pc_map{$key} = 1; 
		}
	}
# %av_fanout_map = ();
	$total_cov = 0;
	foreach $key (sort { $av_fanout_map{$b} <=> $av_fanout_map{$a} } keys %av_fanout_map) {
		$total_cov += $av_fanout_map{$key};
		if ($total_cov > $coverage) {
			last;
		}
		if (!exists $pc_map{$key}) {
			$pc_map{$key} = 1; 
		}
	}
# %av_lifetime_map = ();
	$total_cov = 0;
	foreach $key (sort { $av_lifetime_map{$b} <=> $av_lifetime_map{$a} } keys %av_lifetime_map) {
		$total_cov += $av_lifetime_map{$key};
		if ($total_cov > $coverage) {
			last;
		}
		if (!exists $pc_map{$key}) {
			$pc_map{$key} = 1; 
		}
	}
# %dyn_inst_map = ();
	$total_cov = 0;
	foreach $key (sort { $dyn_inst_map{$b} <=> $dyn_inst_map{$a} } keys %dyn_inst_map) {
		$total_cov += $dyn_inst_map{$key};
		if ($total_cov > $coverage) {
			last;
		}
		if (!exists $pc_map{$key}) {
			$pc_map{$key} = 1; 
		}
	}
	return %pc_map;
}

print " loaded metrics info file\n";


# exit(1);



my %pilot_population_map = ();
my %control_pilot_map = (); # pc->pilots
open IN_CONTROL, $control_patterns_file_name or die "$!: $control_patterns_file_name";
while (<IN_CONTROL>) {
	my $d_line = $_;
	chomp($d_line);
	my @d_words = split (/:/, $d_line);
   	my $my_pilot = int(common_subs::trim($d_words[1]));
	my $population = int(common_subs::trim($d_words[2]));
	my $additional_pilots_t = $additional_pilots;
	if ($population < 1000000 && $additional_pilots > 1) {
		$additional_pilots_t = 0;
	}

	my @list_p;
	if($use_random_selection == 1) {
		$d_words[3] = common_subs::trim($d_words[3]);
		# $d_words[3] has samples. Select a pilot randomly from the sample space.
		if($d_words[3] ne "") {

			for($i=0; $i<$additional_pilots_t; $i+=1) {
				my @samples = split(/ /,$d_words[3]);
				my $rand_index = int(rand($#samples));
				# if sample at rand_index is "" then pick the next sample
				while($samples[$rand_index] eq "") {
					$rand_index++;
					if($rand_index > $#samples) {
						$rand_index = 0;
					}
				}
				# update pilot with a randomly selected sample
				$my_pilot = int($samples[$rand_index]);
				push(@list_p, $my_pilot);
			}
		}
	}
    else {
	    push(@list_p, $my_pilot);
    }

	foreach $list_e (@list_p) {
		$my_pilot = $list_e;

		my $priv_cycles = "";
		if(exists $control_pilot_map {$d_words[0]}) { # d_words[0] is pc
			$priv_cycles = $control_pilot_map {$d_words[0]};
		} 
		$control_pilot_map {$d_words[0]} = "$my_pilot $priv_cycles"; # d_words[1] is pilot

		if (exists $pilot_population_map{$my_pilot}) {
			print "Check: control: $d_words[0], $my_pilot: prev_pop=$pilot_population_map{$my_pilot}, new_pop=$population\n";
		}
		$pilot_population_map {$my_pilot} = $population/$additional_pilots_t;
	}
}
close IN_CONTROL;


#cross-check population per pc



print " loaded control_patterns_file\n";

my %store_pilot_map = (); # pc->pilots
open IN_STORE, $heap_patterns_file_name or die "$!: $heap_patterns_file_name";
while (<IN_STORE>) {
	my $d_line = $_;
	#$d_line = substr $d_line, 0, 40;
	chomp($d_line);
	if($d_line ne "") {
		my @d_words = split (/:/, $d_line);
		$d_words[0] = common_subs::trim($d_words[0]);
		$d_words[1] = common_subs::trim($d_words[1]);
		$d_words[2] = common_subs::trim($d_words[2]);
		my $pc = $d_words[0];
		my $pilot = int($d_words[1]);
		my $population = int($d_words[2]);

		if($use_random_selection == 1) {
			$d_words[3] = common_subs::trim($d_words[3]);
			# $d_words[3] has samples. Select a pilot randomly from the sample space.
			if($d_words[3] ne "") {
				my @samples = split(/ /,$d_words[3]);
				my $rand_index = int(rand($#samples));
				# if sample at rand_index is "" then pick the next sample
				while($samples[$rand_index] eq "") {
					print "$d_words[3]-";
					$rand_index++;
					if($rand_index > $#samples) {
						$rand_index = 0;
					}
				}
				# update pilot with a randomly selected sample
				$pilot = int($samples[$rand_index]);
			}
		}


		if ($additional_pilots == 1) {
			if(exists $store_pilot_map {$pc}) {
				$store_pilot_map {$pc} .= " $pilot";
			} else {
				$store_pilot_map {$pc} = "$pilot";
			}

			#if (exists $pilot_population_map{$pilot}) {
				#print "Check: load: $d_words[0], $pilot: prev_pop=$pilot_population_map{$pilot}, new_pop=$population\n";
			#}
			$pilot_population_map {$pilot} = $population+1;
		}
	}
}
close IN_STORE;

print " loaded heap store_patterns_file\n";

open IN_STORE, $stack_patterns_file_name or die "$!: $stack_patterns_file_name";
while (<IN_STORE>) {
	my $d_line = $_;
	chomp($d_line);
	if($d_line ne "") {
		my @d_words = split (/:/, $d_line);
		$d_words[2] = common_subs::trim($d_words[2]);
		$d_words[1] = common_subs::trim($d_words[1]);
		my $pc = $d_words[0];
		my $pilot = int($d_words[1]);
		my $population = int($d_words[2]);

		if($use_random_selection == 1) {
			$d_words[3] = common_subs::trim($d_words[3]);
			# $d_words[3] has samples. Select a pilot randomly from the sample space.
			if($d_words[3] ne "") {
				my @samples = split(/ /,$d_words[3]);
				my $rand_index = int(rand($#samples));
				# if sample at rand_index is "" then pick the next sample
				while($samples[$rand_index] eq "") {
					$rand_index++;
					if($rand_index > $#samples) {
						$rand_index = 0;
					}
				}
				# update pilot with a randomly selected sample
				$pilot = int($samples[$rand_index]);
			}
		}

		if ($additional_pilots == 1) {
			my $priv_cycles = "";
			if(exists $store_pilot_map {$pc}) {
				$priv_cycles = $store_pilot_map {$pc};
			} 
			$store_pilot_map {$pc} = "$pilot $priv_cycles";

			$pilot_population_map {$pilot} = $population+1;
		}
	}
}
close IN_STORE;

print " loaded stack store_patterns_file\n";

#my @mode_file;
# read the mode info (ranges) in to arrays now and keep it in memory
# This will help not to open the file every time you want to access the mode info
my @range;
my $m_file_index = 0;
open IN_MODE_FILE, $mode_change_file_name or die "$!: $mode_change_file_name";
while (<IN_MODE_FILE>) {
	my $m_line = $_;
	chomp($m_line);
	if($m_line ne "") {
		#$mode_file[$m_file_index] = $m_line;
		chomp($m_line);
		my @temp_range = split (/-/, $m_line);
		$range[$m_file_index][0] = int($temp_range[0]); 
		$range[$m_file_index][1] = int($temp_range[1]); 

		$m_file_index++;
	}
}
close IN_MODE_FILE;

print " loaded mode info file\n";

sub adjust_cycle_up {
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

my %pilot_map = ();
my %def_pc_map = ();
my $total_population = 0;
my $simics_file_counter = 0;
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
			my $src_dest = -1;
			my $fault_type = -1;
			@elems = split(/,/, $fault);
            $elems[5] =~ s/\)//ig;
            #print "fault $fault, elems $elems[5] \n";
			if($agen_reg eq "reg" or $agen_reg eq "both") {
				if($elems[0] =~ /%([gloi][0-9]*)$/)  { # global registers
					$arch_regname = $1;
					$arch_reg = common_subs::arch_reg_name2num($arch_regname);
					$fault_type = 4;
					my $temp_key = "$pc.$arch_reg";
					$def_pc_map{$temp_key} = $elems[5];
                    #print "arch_reg = $arch_reg\n";
                    #print "$pc.$arch_reg: $def_pc_map{$temp_key}, $elems[5]\n";
				}elsif($elems[0] =~ /%([f][0-9]*)$/)  { # floating point registers
					$arch_regname = $1;
					$arch_reg = common_subs::arch_reg_name2num($arch_regname);
					$fault_type = 12;                    # arbitrarily set it to 12, for use in later stages
					my $temp_key = "$pc.$arch_reg";
					$def_pc_map{$temp_key} = $elems[5];
                    #print "arch_reg = $arch_reg\n";
                    #print "$pc.$arch_reg: $def_pc_map{$temp_key}, $elems[5]\n";
				}elsif($elems[0] =~ /%([d][0-9]*)$/)  { # double floating point registers
					$arch_regname = $1;
					$arch_reg = common_subs::arch_reg_name2num($arch_regname);
					$fault_type = 16;                    # arbitrarily set it to 16, for use in later stages
					my $temp_key = "$pc.$arch_reg";
					$def_pc_map{$temp_key} = $elems[5];
                    #print "arch_reg = $arch_reg\n";
                    #print "$pc.$arch_reg: $def_pc_map{$temp_key}, $elems[5]\n";
				}elsif($elems[0] =~ /%fp$/)  {
					$arch_regname = "i6";
					$arch_reg = common_subs::arch_reg_name2num($arch_regname);
					$fault_type = 4;
					my $temp_key = "$pc.$arch_reg";
					$def_pc_map{$temp_key} = $elems[5];
					#print "$pc.$arch_reg: $def_pc_map{$temp_key}, $elems[5]\n";
				}elsif($elems[0] =~ /%sp$/)  {
					$arch_regname = "o6";
					$arch_reg = common_subs::arch_reg_name2num($arch_regname);
					$fault_type = 4;
					my $temp_key = "$pc.$arch_reg";
					$def_pc_map{$temp_key} = $elems[5];
					#print "$pc.$arch_reg: $def_pc_map{$temp_key}, $elems[5]\n";
				}

			}
			if( $agen_reg eq "agen" or $agen_reg eq "both") {
				if($elems[0] =~ /^\[.*\]$/)  {
					$arch_reg = 1; # anything other than -1
					$fault_type = 8;
				}
			}

			my $low = 0;
			$high = $elems[1];
			$high = common_subs::trim($high);
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
				my $pilot_list = "";
				if ($dep_type == 0) { 
					$pilot_list = $store_pilot_map{$dep_pc} if exists $store_pilot_map{$dep_pc};
				} else {
					$pilot_list = $control_pilot_map{$dep_pc} if exists $control_pilot_map{$dep_pc}; # only one of the control_pilot or the store_pilot
				}
				@pilots = split(/ /,$pilot_list);
				foreach $pilot (@pilots) {
					if($pilot eq "") {
							next;
					}
					my $adjusted_pilot = adjust_cycle_up($pilot, $diff_cycle);
					my $job_name = "$pc.$adjusted_pilot.$arch_reg.$src_dest.$low.$high.$fault_type";
#					if ($adjusted_pilot == 355817682593 ) { #for blackscholes
                    #if ($adjusted_pilot == 326066075624 ) { #for streamcluste
                    #    my $temp_key2 = "$pc.$arch_reg";
                    #print "arch_reg = $arch_reg\n";
                    #print "$pc.$arch_reg: $def_pc_map{$temp_key2}, $elems[5]\n";
                    #}
                    if(exists $pilot_map{$job_name}) {
                     	print "$job_name\n\n\n"; 
print "$job_name, pilot=$pilot, diff_cycle=$diff_cycle, dep_type=$dep_type dep_pc=$dep_pc, pilot_list=$pilot_list\n";

                     }
					my $multiplier = 1.0 ;
					if (exists $def_pc_map{"$pc.$arch_reg"}) {
						if ($def_pc_map{"$pc.$arch_reg"} =~ /0x0/) {
							$multiplier = 1.0;
						} else {
							$multiplier = 2.0;
						}
					}
					$multiplier *= (int($high)-$low);
					#print "$multiplier\n";

					$pilot_map{$job_name} = $pilot_population_map{$pilot} * $multiplier;
					$total_population += $pilot_population_map{$pilot} * $multiplier;
					$simics_file_counter++;
				}
			}
		} 
		# print "$pc:$simics_file_counter\n";
	}
}
close IN_STATIC;

print "Total number of injection register sites: $simics_file_counter \n";
print "Size of pilot_pilot_map " . keys(%pilot_map) . "\n";

#$sum = 0;
        #       $total_keys = 0;
#		foreach $key ( keys %pilot_map) {
            #$total_keys = $total_keys+1;
            #$sum += $pilot_map{$key};
            #
            #}
        #print "sum = $sum\n";
        
$simics_file_counter = 0;
my $num_pilots = 0;

open (SUMMARY_FILE, ">>pilot_pop_map_common.txt");
print SUMMARY_FILE "$app_name\t";

print "Total population = $total_population\n";
#my @X = (50,60,70,80,90,95,96,97,98,99,99.9,99.99,100);
#my @X = (99.1, 99.2, 99.3, 99.4, 99.5, 99.6, 99.7, 99.8, 99.9, 100.1);
#my @X = (90,95,98,99,100);
#my @X = (50,60,70,80,90,95,99,100);
#my @X = (99,100);
#my @X = (90, 95, 98, 99, 100);
#my @X = {96, 97, 98};
#my @X = (100);
my @X = (95, 98, 99, 100, 101);
#my @X = (70, 90, 92, 95, 96, 97, 98, 99, 100);
my @metric_coverage = (1);
if ($metric_pc_based == 1) {
	@X = (100);
	#@metric_coverage = (0.5, 0.75, 0.9, 0.95, 0.98, 0.99, 1);
	#@metric_coverage = (0.5, 0.75, 0.9);
	@metric_coverage = (0.75);
}

foreach $cov (@metric_coverage) {
	my %pc_map = get_metric_pc_list($cov);
	my $size = scalar keys %pc_map;
	print "size = $size\n";

	foreach $x (@X) { 
		
		my %selected_pcs = ();
		my $stop = 0;
	
		if($write_files == 1) {
			my $simics_filename = "$ENV{APPROXILYZER}";
            $simics_filename .= "/fault_list_output/simics_file_list/simics_file_list_$app_name\_$prune_type\_50_$x\_$BIT_GAP.txt";

            # removes file if it already exists, because concatenation will append to old file
            if (-e $simics_filename) {
                unlink $simics_filename;
            }
			open (SIMICS_FILE_LIST, ">>$simics_filename");
			print "$simics_filename\n";
			SIMICS_FILE_LIST->autoflush(1);
		}
		$simics_file_counter = 0;
		$num_pilots = 0;
		my $total_pop_top_x = int(($total_population*$x/100)+0.5);
		my $total_pop = 0;
		foreach $key (sort { $pilot_map{$b} <=> $pilot_map{$a} } keys %pilot_map) {
			# print "$pilot_map{$key}:$key\n";
	#		write simics file for pilot 
	
			@words = split(/\./,$key);
			$pc = $words[0];
			$adjusted_pilot = $words[1];
			$c = 0; 
			$arch_reg = $words[2];
			$src_dest = $words[3];
			$low = $words[4];
			$high = $words[5];
			$fault_type = $words[6];
	
			if ($metric_pc_based == 1) {
				if (!exists $pc_map{$pc}) {
					#print "$pc_map{$pc}, $pc\n";
					next;
				}

				if ($pilot_map{$key}/($high-$low) < 1000) {
					next;
				}
			}
	
			if (!exists $selected_pcs{$pc}) {
				$selected_pcs{$pc} = 1;
				if ($stop == 1) {
					next;
				}
			} 
			if ($stop == 1) {
				if ($pilot_map{$key}/($high-$low) < 1000) {
					next;
				}
			}
			$total_pop += $pilot_map{$key};
	
			for($i = $low; $i < $high; $i=$i+$BIT_GAP) {
				my $job_name = "$app_name.$pc.$adjusted_pilot.$c.$arch_reg.$i.$src_dest.$fault_type";
				if($write_files == 1) {
					print SIMICS_FILE_LIST "$job_name\n";
				}
				$simics_file_counter += 1;
				$num_pilots += 1;
			}
	
			if($total_pop > $total_pop_top_x) {
                $stop = 1;
				#last;
			}
            else
            {
                #print "total pop = $total_pop, total_pop_top_x=$total_pop_top_x\n";
                    
            }
		}
		print SUMMARY_FILE "$num_pilots\t";
		my $f = 100*$total_pop/$total_population;
		print "$x: $f: $num_pilots\n";
        #print "$x: $f: $num_pilots: $total_pop: $total_population\n";
	
		if($write_files == 1) {
			close SIMICS_FILE_LIST;
		}
	}
}

print SUMMARY_FILE "\n";
close SUMMARY_FILE;
print "Number of pilots: $num_pilots\n";
print "Number of simics files: $simics_file_counter \n";
