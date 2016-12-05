local $| = 1;
use Math::BigInt;
use Time::HiRes qw(gettimeofday tv_interval);
#use lib '/home/sadve/shari2/research/relyzer/perl/';
#use lib '/shared/workspace/amahmou2/relyzer/perl/';
use lib "$ENV{APPROXILYZER}/scripts/static_relyzer/scripts/perl"; #TODO MAKE SCALABLE
use common_subs;

#$MAIN_LOGS_DIR = "/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/fully_optimized_main_logs";
#$FULL_INJ_LOG_DIR = "/home/sadve/shari2/GEMS/simics/home/live_analysis/logs";
#$RESULTS_OUTPUT_DIR = "/home/sadve/shari2/GEMS/simics/home/live_analysis/parsed_results";
#$MAIN_LOGS_DIR = "/shared/workspace/amahmou2/Relyzer_GEMS/src_GEMS/simics/home/dynamic_relyzer/siva_fully_optimized_main_logs_new";
#$MAIN_LOGS_DIR = "$ENV{GEMS_PATH_SHARED}/simics/home/dynamic_relyzer/output_log_input_independence/";

$FAULT_LIST_DIR = "$ENV{APPROXILYZER}/fault_list_output/";
$REFORMATTED_OUTPUT_DIR= "$FAULT_LIST_DIR/coverage_calculation/reformatted_outcomes/";
$RESULTS_OUTPUT_DIR = "$FAULT_LIST_DIR/coverage_calculation/parsed_results/";

$prune_type = "all";

my $numArgs = $#ARGV + 1;
if($numArgs != 1) {
	print "Usage: perl parsing_output.pl <at least one app name> \n"; 
	die;
}
#my $app_name=$ARGV[$0];
#my $INJECTION_TYPE=$ARGV[1];
my $i = 0;
my @apps = ();
for ($i=0; $i<$numArgs ; $i++) {
	push(@apps, $ARGV[$i]);
	print "$ARGV[$i]\n";
}
#print @apps;
#print "app name is $app_name\n";

$MAIN_LOGS_DIR = "$ENV{APPROXILYZER}/workloads/apps/$ARGV[0]/analysis_output/";

#$INJECTION_TYPE="floats";
#print "injection type is $INJECTION_TYPE\n";

my $check_inclusion = 0;

my $bad_pilots = 0;

# pc -> pilots
my %control_pilot_map = (); # pc->pilots
my %store_pilot_map = (); # pc->pilots

my %pilot_population_map = ();

my %gap_pc_map = (); # pc->gap
my %def_pc_map = (); # pc.arch_reg_num->def_pc
my %pc_pilot_map = (); # pc -> pilots
my %pcreg_high = (); # pc-reg -> high
sub populate_gap_pc_map ($) {
    my $app_str = shift;

	# clear the map first
	%gap_pc_map = ();

    my $output_file = "$FAULT_LIST_DIR/${app_str}_fault_list.txt";


#my $output_file = "/home/sadve/shari2/research/relyzer/output/$app_str\_$app_str\_fault_list.txt";
#    if ($INJECTION_TYPE eq "floats") {
#        print "floats output is here\n";
#        $output_file = "/shared/workspace/approx_comp/scripts/static_relyzer/output/float_fault_list/$app_str\_$app_str\_fault_list.txt";  # ENABLE FOR FLOATING POINT
#    }
#    else {
#        print "ints output is here\n";
#        $output_file = "/shared/workspace/approx_comp/scripts/static_relyzer/output/int_fault_list/$app_str\_$app_str\_fault_list.txt";  # ENABLE FOR INTEGER 
        #$output_file = "/shared/workspace/amahmou2/relyzer/output/int_only_fault_lists/$app_str\_$app_str\_fault_list.txt";  #ENABLE FOR INTEGER
#    }

    print "Fault list location is: $output_file\n"; 

	open IN_FAULT_LIST, $output_file or die "$!: $output_file";
	while (<IN_FAULT_LIST>) {
		my $d_line = $_;
		chomp($d_line);
		my @d_words = split (/:/, $d_line);

	    # frequecy[]:pc:dep_type:{dep_pc,diff_cycle}:faults
	    # 0- frequency, 1 - pc, 2 - dep_type, 3 - dep_pc, 4 - faults

		my $temp_pc = $d_words[1];
		$temp_pc = Math::BigInt->new($temp_pc);
	    my $dep_type = int($d_words[2]);
	    my $dep_pc = "";
	    my $diff_cycle = "";
	    if($d_words[3] =~ /\{([0-9a-z]*)\, (-*[0-9]+)\}/) {
	        $dep_pc = $1;
	        $diff_cycle = $2;
	    }
		$dep_pc = common_subs::trim($dep_pc);

		$gap_pc_map {$temp_pc} = $diff_cycle;

		my $hex_pc = sprintf ("0x%x", $temp_pc);


		@fault_list = split(/ \(/, $d_words[4]);
        # fault list : (0-reg, 1-max_bits, 2-is_address, 3-is_def, 4-extra_count, 5-def_pc) separated by spaces
        foreach (@fault_list) {
                 my $fault = $_;
                 chomp($fault);
                 if($fault ne "") {
                         if ($fault =~ /(.*)\)$/) {
                               $fault = $1;
                         }
                         #print "$fault\n";
                         my @elements = split(/\, /, $fault);
                        
                         my @conditional_value;
                         #
                         #if($INJECTION_TYPE eq "floats") {
                         #   $conditional_value = &common_subs::is_fp_register ( $elements[0]);
                         #}
                         #else {
                         #   $conditional_value = &common_subs::is_int_register ( $elements[0]);
                         #

                         # regardless of Float or Int
                         #}
                         $conditional_value = &common_subs::is_register ( $elements[0]);

#                         if(&common_subs::is_int_register( $elements[0] ) == 1) { # USE WITH INTEGER
#                         if(&common_subs::is_fp_register( $elements[0] ) == 1) { # USE WITH FLOATING POINT
                         if($conditional_value  == 1) { 
							 my $arch_reg = common_subs::arch_reg_name2num($elements[0]);
							 my $src_dest = $elements[3];
							 $def_pc_map {"$hex_pc.$arch_reg"} = $elements[5];
                             #print "$hex_pc $arch_reg:$elements[0] $elements[5]\n";


							$pcreg_high{"$hex_pc.$arch_reg.$src_dest"} = $elements[1];

							if ($dep_type == 0) {
								$p_str = $store_pilot_map{$dep_pc};
								my @my_ps = split(/ /, common_subs::trim($p_str));
								foreach $p (@my_ps) {
                                    #print "-$pilot_population_map{$p}- ";
									if ($pilot_population_map{$p} > 1000) { # don't add pilots with small population
										$acu = adjust_cycle_up($p, $diff_cycle);
								 		if(!exists $pc_pilot_map{"$hex_pc.$arch_reg.$src_dest"}{$acu}) {
											$pc_pilot_map{"$hex_pc.$arch_reg.$src_dest"}{$acu} = 1;
											#print "adding $temp_pc, $acu\n";
										}
									} else {
										#print "$pilot_population_map{$p} ";
									}
								}
							 } elsif ($dep_type == 1 || $dep_type == 2 || $dep_type == 10) {
								$p_str = $control_pilot_map{$dep_pc};
								my @my_ps = split(/ /, common_subs::trim($p_str));
								foreach $p (@my_ps) {
                                    #print "-$pilot_population_map{$p}- ";
									if ($pilot_population_map{$p} > 1000) { # don't add pilots with small population
										$acu = adjust_cycle_up($p, $diff_cycle);
								 		if(!exists $pc_pilot_map{"$hex_pc.$arch_reg.$src_dest"}{$acu}) {
											$pc_pilot_map{"$hex_pc.$arch_reg.$src_dest"}{$acu} = 1;
											#print "adding $temp_pc, $acu\n";
										}
									} else {
									#	print "$pilot_population_map{$p} ";
									}
								}
							 }

                         }
                 }
         }

	}
	close IN_FAULT_LIST;
}


# my %pilot_sample_map = ();
my %sample_pilot_map = ();
sub populate_pilot_population_map ($) {
	my $curr_app = shift;
	# clear map
	%pilot_population_map = ();
	# %pilot_sample_map = ();
	%sample_pilot_map = ();

	open(CLASSES_FILE, "<$MAIN_LOGS_DIR/$curr_app-heap-classes.output") or die "Can't open file: $!";
	while ( <CLASSES_FILE> ) {
	    my $my_line = $_;
		chomp ($my_line);
		if ( $my_line ne "") { 
			my @my_words = split(/:/, $my_line);
			my $my_pc = common_subs::trim($my_words[0]);
			my $my_pilot = common_subs::trim($my_words[1]);
			my $population = common_subs::trim($my_words[2]);
			$pilot_population_map {$my_pilot} = $population+1;
			# $pilot_sample_map {$my_pilot} = common_subs::trim($my_words[3]);
			my @my_samples = split(/ /, common_subs::trim($my_words[3]));
			foreach $sample (@my_samples) {
				$sample_pilot_map{$sample} = $my_pilot;
			}

			if(exists $store_pilot_map{$my_pc}) {
				$store_pilot_map{$my_pc} .= " $my_pilot";
			} else {
				$store_pilot_map{$my_pc} = "$my_pilot";
				#print "$my_pc added to store_pilot_map: $store_pilot_map{$my_pc}\n";
			}
		}
	}
	close(CLASSES_FILE);
	open(CLASSES_FILE, "<$MAIN_LOGS_DIR/$curr_app-stack-classes.output") or die "Can't open file: $!";
	while ( <CLASSES_FILE> ) {
		my $my_line = $_;
		chomp ($my_line);
		if ( $my_line ne "") { 
			my @my_words = split(/:/, $my_line);
			my $my_pc = common_subs::trim($my_words[0]);
			my $my_pilot = common_subs::trim($my_words[1]);
			my $population = common_subs::trim($my_words[2]);
			$pilot_population_map {$my_pilot} = $population+1;
			# $pilot_sample_map {$my_pilot} = common_subs::trim($my_words[3]);
			my @my_samples = split(/ /, common_subs::trim($my_words[3]));
			foreach $sample (@my_samples) {
				$sample_pilot_map{$sample} = $my_pilot;
			}

			if(exists $store_pilot_map{$my_pc}) {
				$store_pilot_map{$my_pc} .= " $my_pilot";
			} else {
				$store_pilot_map{$my_pc} = "$my_pilot";
				#print "$my_pc added to store_pilot_map: $store_pilot_map{$my_pc}\n";
			}

		}
	}
	close(CLASSES_FILE);

    #open(CLASSES_FILE, "</home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/pickle_dir/consolidated/$curr_app\_sample_control_pc_patterns50.txt") or die "Can't open file: $!";
    open(CLASSES_FILE, "<$MAIN_LOGS_DIR/$curr_app\_sample_control_pc_patterns_50_5000.txt") or die "Can't open file: $!";
    #open(CLASSES_FILE, "<$MAIN_LOGS_DIR/$curr_app\_sample_control_pc_patterns50.txt") or die "Can't open file: $!";
	#open(CLASSES_FILE, "<$MAIN_LOGS_DIR\_new/$curr_app\_sample_control_pc_patterns_10_500.txt") or die "Can't open file: $!";
	#open(CLASSES_FILE, "<$MAIN_LOGS_DIR/$curr_app\_sample_control_pc_patterns.txt") or die "Can't open file: $!";
	while ( <CLASSES_FILE> ) {
	    my $my_line = $_;
		chomp ($my_line);
		if ( $my_line ne "") { 
			my @my_words = split(/:/, $my_line);
			my $my_pc = common_subs::trim($my_words[0]);
			my $my_pilot = common_subs::trim($my_words[1]);
			my $population = common_subs::trim($my_words[2]);
			$pilot_population_map {$my_pilot} = $population+1;
			# $pilot_sample_map {$my_pilot} = common_subs::trim($my_words[3]);
			my @my_samples = split(/ /, common_subs::trim($my_words[3]));
			foreach $sample (@my_samples) {
				$sample_pilot_map{$sample} = $my_pilot;
			}

			if(exists $control_pilot_map{$my_pc}) {
				$control_pilot_map{$my_pc} .= " $my_pilot";
			} else {
				$control_pilot_map{$my_pc} .= "$my_pilot";
			}

		}
	}
	close(CLASSES_FILE);
}

# return pilot for a given sample
sub get_pilot ($) {
	my $sample = shift;
		
	my $ret_val = "";
	if ($sample ne "") {
		$ret_val = $sample_pilot_map{$sample};
	}

	return $ret_val;
}


foreach $curr_app (@apps) {
    #
    #	# Loading metrics file 
    #	# Format: PC \t sdc \t life \t fanout \t av.life \t av.fan \t di \t metrics_covered
    #	my $metrics_filename ="/home/sadve/venktgr2/from_collab/simulator/GEMS/simics/home/dynamic_relyzer/veena_dynamic_profile_output/correct_life_def/metric_results/live_across_save_result/sort/$curr_app\_tot_life_sort.txt";
    #	open IN_METRICS_FILE, $metrics_filename or die "$!: $metrics_filename";
    #	%fanout_map = ();
    #	%lifetime_map = ();
    #	%av_fanout_map = ();
    #	%av_lifetime_map = ();
    #	%dyn_inst_map = ();
    #	while (<IN_METRICS_FILE>) {
    #			my $m_line = $_;
    #			chomp($m_line);
    #			if($m_line ne "") {
    #					@words = split(/\t/, $m_line);
    #					if ($#words == 7 && $words[0] =~ /1000/) {
    #
    #							# my $key = hex($words[0]);
    #							# print "key: $key\n";
    #	
    #							$lifetime_map{$key} = $words[2];
    #	
    #							$fanout_map{$key} = $words[3];
    #	
    #							$av_lifetime_map{$key} = $words[4];
    #	
    #							$av_fanout_map{$key} = $words[5];
    #	
    #							$dyn_inst_map{$key} = $words[6];
    #	
    #	# print "\n";
    #	# foreach $word (@words) {
    #	# 	print "$word ";
    #	# }
    #					}
    #			}
    #	}
    #	close IN_METRICS_FILE;
    #	
    #	sub get_metric_pc_list {
    #			my $coverage = $_[0];
    #	
    #			my $total_cov = 0;
    #			my %pc_map = ();
    #	
    #	# %fanout_map = ();
    #			foreach $key (sort { $fanout_map{$b} <=> $fanout_map{$a} } keys %fanout_map) {
    #					$total_cov += $fanout_map{$key};
    #					if ($total_cov > $coverage) {
    #							last;
    #					}
    #					if (!exists $pc_map{$key}) {
    #						#print "$key:$fanout_map{$key}\n";
    #						$pc_map{$key} = 1; 
    #					}
    #			}
    #	# %lifetime_map = ();
    #			$total_cov = 0;
    #			foreach $key (sort { $lifetime_map{$b} <=> $lifetime_map{$a} } keys %lifetime_map) {
    #					$total_cov += $lifetime_map{$key};
    #					if ($total_cov > $coverage) {
    #							last;
    #					}
    #					if (!exists $pc_map{$key}) {
    #							$pc_map{$key} = 1; 
    #					}
    #			}
    #	# %av_fanout_map = ();
    #			$total_cov = 0;
    #			foreach $key (sort { $av_fanout_map{$b} <=> $av_fanout_map{$a} } keys %av_fanout_map) {
    #					$total_cov += $av_fanout_map{$key};
    #					if ($total_cov > $coverage) {
    #							last;
    #					}
    #					if (!exists $pc_map{$key}) {
    #							$pc_map{$key} = 1; 
    #					}
    #			}
    #	# %av_lifetime_map = ();
    #			$total_cov = 0;
    #			foreach $key (sort { $av_lifetime_map{$b} <=> $av_lifetime_map{$a} } keys %av_lifetime_map) {
    #					$total_cov += $av_lifetime_map{$key};
    #					if ($total_cov > $coverage) {
    #							last;
    #					}
    #					if (!exists $pc_map{$key}) {
    #							$pc_map{$key} = 1; 
    #					}
    #			}
    #	# %dyn_inst_map = ();
    #			$total_cov = 0;
    #			foreach $key (sort { $dyn_inst_map{$b} <=> $dyn_inst_map{$a} } keys %dyn_inst_map) {
    #					$total_cov += $dyn_inst_map{$key};
    #					if ($total_cov > $coverage) {
    #							last;
    #					}
    #					if (!exists $pc_map{$key}) {
    #							$pc_map{$key} = 1; 
    #					}
    #			}
    #			return %pc_map;
    #	}
    #	
    #	print " loaded metrics info file\n";
    #
    #
    #
    #
    #
    #
    #
    	print "curr app is $curr_app: ";
    	$app_str = common_subs::get_app_str($curr_app);
    	print "app str is $app_str\n";
    	my %outcome_info_map = ();
    
    
    
    	#my @mode_file;
    	# read the mode info (ranges) in to arrays now and keep it in memory
    	# This will help not to open the file every time you want to access the mode info
    	my @range;
    	my $mode_app_str = $curr_app;
    	my $mode_change_file_name = "$MAIN_LOGS_DIR/$mode_app_str\_modes.txt";
        print "mode file is $mode_change_file_name\n";
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
    
    print "loaded mode info file\n";
    
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


	sub adjust_cycle {
		my $cycle = $_[0];
		my $diff_cycle = $_[1];

		if($diff_cycle > 15) {
			my $temp_cycle = adjust_cycle($cycle, 15);
			$cycle = adjust_cycle($temp_cycle, $diff_cycle-15);
		} else {
			$cycle = $cycle + $diff_cycle;
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
					# if($temp_cycle == 385188573794) {
					# 	print "done:start=$range[$start][0], end=$range[$end][1]\n";
					# }
					for ($ii=$start; $ii<$end; $ii++) {
						# if($temp_cycle == 385188573794) {
						# 	print "done:start=$range[$ii][0], end=$range[$ii][1]\n";
						# }
						if($cycle >= $range[$ii][0] && $cycle < $range[$ii][1])  { # cycle falls in the range of priv mode
							$cycle = $cycle + ($range[$ii][1]-$range[$ii][0]) + 1;
							$adjusted = 1;  # cycle is adusted in this iteration 
							$num_adjustments++;
						} elsif($range[$ii][0] > $cycle) {
							last;
						}
					}
					if($adjusted == 0) { # check if the cycle is adusted in this iteration
						$need_adjustment = 0; # if not then no need to check again
					}
				}
			}
		}
		return $cycle;
	}

	my $LOGS;
	my $PARSED_RESULTS_DIR;
	$LOGS = "$REFORMATTED_OUTPUT_DIR/$curr_app";
	$PARSED_RESULTS_DIR = "$RESULTS_OUTPUT_DIR";
	print "curr app is: $curr_app \n";
	print "output location is: $PARSED_RESULTS_DIR \n";

	open OUT_ERRORS, ">$PARSED_RESULTS_DIR/$curr_app\_$prune_type\_errors.txt";
	open OUT_SDCS, ">$PARSED_RESULTS_DIR/$curr_app\_$prune_type\_sdcs.txt";
	open OUT_MASKED, ">$PARSED_RESULTS_DIR/$curr_app\_$prune_type\_masked.txt";
	open OUT_DETECTED, ">$PARSED_RESULTS_DIR/$curr_app\_$prune_type\_detected.txt";

	my @orig_detection;
	my @later_detection;
	my @masked;
	my @sdc;
	my @error;
	my @population_arr;
	my $index = 0;
	my $total_jobs = 0;

	populate_pilot_population_map($curr_app);
	print "populated pilot population map\n";

	populate_gap_pc_map($app_str);
	print "populated gap pc map\n";

	#open IN_FILE, "$LOGS.outcomes_full_injections_10" or die;
	open IN_FILE, "$LOGS.reformatted" or die;
	while (<IN_FILE>) {
		my $line = $_;
		chomp($line);
		$total_jobs++;
		if ($total_jobs % 10000 == 1) {
			print "jobs so far: $total_jobs\n";
		}

		@words = split (/:/, $line);
		# format - "$app.pc$pc.p$pilot.c$core.t$type.b$bit.s$stuck.r$reg.d$dest_src.$sample";
		my @attributes = split(/\./,$words[0]);
		my $result_str = "";
        my $detailed_output = "";
		if($#words > 0) {
			$result_str = $words[1];
            $detailed_output = $words[2];
		}

		my $app = $attributes[0];
		my $pc_str = $attributes[1];
		my $pc = 0;
		if($pc_str =~ /pc(.*)/) {
			$pc_str = $1;
			$pc = Math::BigInt->new($pc_str);
		}
		my $pilot = $attributes[2];
		if($pilot =~ /p(.*)/) {
			$pilot_str = $1;
			$pilot = Math::BigInt->new($pilot_str);
		}
		my $int_sample = -1;
		my $sample = $attributes[9];
		if($sample =~ /c([0-9]*)/) {
			$int_sample = $1;
		}
		my $reg = $attributes[7];
		if($reg =~ /r([0-9]*)/) {
			$reg = $1;
		}
		my $bit = $attributes[5];
		if($bit =~ /b([0-9]*)/) {
			$bit = $1;
		}
        my $type = $attributes[4];
		if($type =~ /t([0-9]*)/) {
			$type = $1;
		}

		my $dest_src = $attributes[8];
		my $int_dest_src = -1;
		if($dest_src =~ /d([0-9])/) {
			$int_dest_src = $1;
		}

		my $run_name = "$LOGS/$attributes[0]";
		for($i = 1; $i<$#attributes; $i++) {
			$run_name = "$run_name.$attributes[$i]";
		}
		#print "$run_name\n";


		my $hex_pc = sprintf ("0x%x", $pc);
		# print "app=$attributes[0], pc=$attributes[1], pilot=$attributes[2], sample=$attributes[9]\n";
		# print "reg=$attributes[7], bit=$attributes[5], src_dest=$attributes[8]\n";
		my $simics_job_name = "$app.$hex_pc.$pilot.$int_sample.$reg.$bit.$int_dest_src.$type";
		my $def_pc_key = "$hex_pc.$reg";
		my $def_pc_dest_src_key = "$hex_pc.$reg.$int_dest_src";
		my $def_pc = "0x0";
		my $eq_simics_job_name = "";
		if( exists $def_pc_map{$def_pc_key} ) {
			$def_pc = $def_pc_map{$def_pc_key};
			# FIXME: The cycle number in the equalized simics_job is incorrect. Please fix it by finding the correct adjustment to the cycle number (pilot)

			$eq_simics_job_name = "$app.$def_pc.$pilot.$int_sample.$reg.$bit.1.$type"; # int_dest_src should be 1 because in the equalized instruction this register should be in the destination.
		}


		my $gap = 0;
		if(exists $gap_pc_map{$pc}) {
			$gap = $gap_pc_map {$pc};
		}
		my $adjusted_cycle = adjust_cycle($pilot, $gap);

		if (!exists $pilot_population_map {$adjusted_cycle})  {
			# this cycle should be found in samples
			$actual_pilot = get_pilot($adjusted_cycle); 
			if( $actual_pilot eq "") {
				print " local_pilot=$pilot, sample=$adjusted_cycle, actual_pilotue=$actual_pilot gap=$gap\n";
			} else {
				$adjusted_cycle = $actual_pilot;#adjusted_cycle to be the cycle of the pilot to get the population more easily later on 
				$actual_pilot_adjusted_up = adjust_cycle_up($adjusted_cycle, $gap); 
				if(exists $pc_pilot_map{$def_pc_dest_src_key}{$actual_pilot_adjusted_up}) {
					$pc_pilot_map{$def_pc_dest_src_key}{$actual_pilot_adjusted_up} = 0;
				} else {
					if ($pilot_population_map {$adjusted_cycle} > 1000) {
						print "$actual_pilot_adjusted_up , $gap not found in $pc in pc_pilot_map\n";
					}
				}
				#keep a list of cycles for each fault site <pc, bit, reg>
				# if this list is different from list of cycles for this pc, then
					# consider injections in the remaining cycles
			}
		}

		$population_arr[$index] = -1;
		$population_arr[$index] = $pilot_population_map {$adjusted_cycle} if exists $pilot_population_map {$adjusted_cycle};
		if ($population_arr[$index] == -1) { 
			#print "population is -1 at $pilot-$adjusted_cycle, $pc_str : $simics_job_name\n"; #, $simics_file\n";
			$bad_pilots += 1;
		}

		$masked[$index][$bit] = 0;
		$orig_detection[$index][$bit] = 0;
		$later_detection[$index][$bit] = 0;
		$sdc[$index][$bit] = 0;
		$error[$index][$bit] = 0;


		my $result = 0;
        #if($result_str =~ /Detected/ || $detailed_output =~ /;1/) { #include detected and "weird" values
		if($result_str =~ /Detected/ ) { #include detected and "weird" values
			if ($result_str =~ /Later/) {
				$later_detection[$index][$bit]++;
				if($def_pc ne "0x0") { # count it again because this fault also represnts a def
					$later_detection[$index][$bit]++;
					print OUT_DETECTED "$population_arr[$index]:$eq_simics_job_name:0\n"; # 0 means that it's a equalized detection for a def
				}
				print OUT_DETECTED "$population_arr[$index]:$simics_job_name:1\n"; # 1 means that it's the actual injected job
				# if ($population_arr[$index] == -1) { 
				# 	print "population is -1 at $pilot-$adjusted_cycle, $pc_str : $simics_job_name\n"; #, $simics_file\n";
				# }
				$result = 1;
			} else {
				$orig_detection[$index][$bit]++;
				if($def_pc ne "0x0") { # count it again because this fault also represnts a def
					$orig_detection[$index][$bit]++;
					# print OUT_DETECTED "$simics_job_name\n"; # old code
					print OUT_DETECTED "$population_arr[$index]:$eq_simics_job_name:0\n"; # 0 means that it's a equalized detection for a def
				}
				print OUT_DETECTED "$population_arr[$index]:$simics_job_name:1\n"; # 1 means that it's the actual injected job
				# if ($population_arr[$index] == -1) { 
				# 	print "population is -1 at $pilot-$adjusted_cycle, $pc_str : $simics_job_name\n"; #, $simics_file\n";
				# }
				$result = 2;
			}
		} elsif ($result_str =~ /Masked/) {
			$masked[$index][$bit]++;
			if($def_pc ne "0x0") { # count it again because this fault also represnts a def
				$masked[$index][$bit]++;
				print OUT_MASKED "$population_arr[$index]:$eq_simics_job_name:0\n"; # 0 means that it's a equalized sdc - for def
			}
			print OUT_MASKED "$population_arr[$index]:$simics_job_name:1\n";
			# if ($population_arr[$index] == -1) { 
			# 	print "population is -1 at $pilot-$adjusted_cycle, $pc_str : $simics_job_name\n"; #, $simics_file\n";
			# }
			$result = 3;
		} elsif ($result_str =~ /SDC/) {
			$sdc[$index][$bit]++;
			if($def_pc ne "0x0") { # count it again because this fault also represnts a def
				$sdc[$index][$bit]++;
				print OUT_SDCS "$population_arr[$index]:$eq_simics_job_name:0\n"; # 0 means that it's a equalized sdc - for def
			}
			print OUT_SDCS "$population_arr[$index]:$simics_job_name:1\n"; # 1 means that it's the actual injected job
			# if ($population_arr[$index] == -1) { 
			# 	print "population is -1 at $pilot-$adjusted_cycle, $pc_str : $simics_job_name\n"; #, $simics_file\n";
			# }
			$result = 4;
		} elsif ($result_str =~ /Error/) {
			$error[$index][$bit]++;
			if($def_pc ne "0x0") { # count it again because this fault also represnts a def
				$error[$index][$bit]++;
			}
			print OUT_ERRORS "$population_arr[$index]:$simics_job_name\n";
			$result = 5;
		} else {
			$error[$index][$bit]++;
			if($def_pc ne "0x0") { # count it again because this fault also represnts a def
				$error[$index][$bit]++;
			}
			print OUT_ERRORS "$population_arr[$index]:$simics_job_name\n";
			$result = 5;
		}
		my $outcome_key = "$hex_pc.$reg.$bit";
		$outcome_info_map{$outcome_key}{"$pilot.$adjusted_cycle"} = $result;
		if($def_pc ne "0x0") { # count it again because this fault also represnts a def
			$outcome_key = "$def_pc.$reg.$bit";
			$outcome_info_map{$outcome_key}{"$pilot.$adjusted_cycle"} = $result;
		}

		$index++;
	}

	print "Total jobs = $total_jobs\n";
	my $bit_gap = 1;
	print " Bit gap = $bit_gap\n";

	open TOTAL_SUMMARY, ">$PARSED_RESULTS_DIR/$curr_app\_$prune_type\_summary.txt";
	open TOTAL_SUMMARY_WITH_POP, ">$PARSED_RESULTS_DIR/$curr_app\_$prune_type\_summary_with_pop.txt";
	for($bit = 0; $bit< 64; $bit+=$bit_gap) {
		my $total_with_pop_masked = 0;
		my $total_with_pop_detections = 0;
		my $total_with_pop_later_detects = 0;
		my $total_with_pop_sdcs = 0;
		my $total_with_pop_errors = 0;

		my $total_masked = 0;
		my $total_detections = 0;
		my $total_later_detects = 0;
		my $total_sdcs = 0;
		my $total_errors = 0;
		for($i = 0; $i<$index; $i++) {
			$something_defined = 0;
			if (defined $masked[$i][$bit]) {
				$total_masked += $masked[$i][$bit];
				$total_with_pop_masked += $masked[$i][$bit]*$population_arr[$i];
				$something_defined = 1;
			}
			if (defined $orig_detection[$i][$bit]) {
				$total_detections += $orig_detection[$i][$bit];
				$total_with_pop_detections += $orig_detection[$i][$bit]*$population_arr[$i];
				$something_defined = 1;
			}
			if (defined $later_detection[$i][$bit]) {
				$total_later_detects += $later_detection[$i][$bit];
				$total_with_pop_later_detects += $later_detection[$i][$bit]*$population_arr[$i];
				$something_defined = 1;
			}
			if (defined $sdc[$i][$bit]) {
				$total_sdcs += $sdc[$i][$bit];
				$total_with_pop_sdcs += $sdc[$i][$bit]*$population_arr[$i];
				$something_defined = 1;
				if($sdc[$i][$bit] == 1 && $population_arr[$i]>100000) {
# print "bit = $bit, sdc=$sdc[$i][$bit], population=$population_arr[$i]\n";
				}
			}
			if (defined $error[$i][$bit]) {
				$total_errors += $error[$i][$bit];
				$total_with_pop_errors += $error[$i][$bit]*$population_arr[$i];
				$something_defined = 1;
			}
			# if the bit is not found, then it was pruned out by the address bounding. Threat them as detected
		}
		my $total_all = $total_masked  + $total_detections  +
			$total_later_detects  + $total_sdcs + $total_errors;

		if($total_all != 0) {
			# print "\nbit = $bit\n";

			$total_with_pop_masked = $total_with_pop_masked/1000000;
			$total_with_pop_detections = $total_with_pop_detections/1000000;
			$total_with_pop_later_detects = $total_with_pop_later_detects/1000000;
			$total_with_pop_sdcs = $total_with_pop_sdcs/1000000;
			$total_with_pop_errors = $total_with_pop_errors/1000000;

			# print "total_with_pop_masked $total_with_pop_masked \n";
			# print "total_with_pop_detections $total_with_pop_detections \n";
			# print "total_with_pop_later_detects $total_with_pop_later_detects \n";
			# print "total_with_pop_sdc $total_with_pop_sdcs \n";
			# print "total_with_pop_errors $total_with_pop_errors \n";

			print TOTAL_SUMMARY_WITH_POP "$bit\t $total_with_pop_masked\t $total_with_pop_detections\t $total_with_pop_later_detects\t $total_with_pop_sdcs\t $total_with_pop_errors\n";
			print TOTAL_SUMMARY "$bit\t $total_masked\t $total_detections\t $total_later_detects\t $total_sdcs\t $total_errors\n";


			# print "total_masked $total_masked \n";
			# print "total_detections $total_detections \n";
			# print "total_later_detects $total_later_detects \n";
			# print "total_sdcs $total_sdcs \n";
			# print "total_errors $total_errors \n";
		}
	}
	close TOTAL_SUMMARY;
	close TOTAL_SUMMARY_WITH_POP;

	close OUT_ERRORS;
	close OUT_SDCS;
	close OUT_MASKED;
	close OUT_DETECTED;

	my $total_population = 0.0;
	my $predicted = 0.0;
	my $mis_predicted = 0.0;
	my @num_outcomes = (0,0,0,0,0,0);
	foreach $key (keys %outcome_info_map) {
		my @outcome_types = (0,0,0,0,0);
		foreach $pilot (keys %{ $outcome_info_map{$key} }) {
			@words = split(/\./, $pilot);
			$pop = $pilot_population_map{$words[1]};

			$outcome_types[$outcome_info_map{$key}{$pilot}-1] += $pop ;
		}
		#print "$key\n";
		my $num_diff_outcomes = 0;
		foreach $elem (@outcome_types) {
			if ($elem != 0) {
				$num_diff_outcomes++;
			}
		}
		$num_outcomes[$num_diff_outcomes-1]++;

		my $local_total_pop = 0;
		foreach $val (@outcome_types) {
			$local_total_pop += $val;
		}
		$total_population += $local_total_pop;
		$random_num = int(rand($local_total_pop));
		my $local_predicted = 0;
		my $local_mis_predicted = 0;

		for($i=0; $i<=$#outcome_types; $i++) {
			$val = $outcome_types[$i];
			if ($random_num <= $val) {
				$local_predicted = $val;
				if($i != 3) { # selected is not SDC
					$local_mis_predicted = $outcome_types[3];
				}
				last;
			} 
			$random_num = $random_num - $val;
		}
		$predicted += $local_predicted;
		$mis_predicted += $local_mis_predicted;
	}
	$i= 0;
	open NEED_CATS, ">>need_cats.txt";
	print NEED_CATS "$curr_app";
	foreach $val (@num_outcomes) {
		# print "num cats $i = $val \n";
		print NEED_CATS "\t$val";
		$i++;
	}
	my $predicted_f = $predicted/$total_population;
	my $mis_predicted_f = $mis_predicted/$total_population;
	print NEED_CATS "\t$predicted_f\t$mis_predicted_f\n";
	print "predicted_f=$predicted_f, $mis_predicted_f\n";

	#my @coverage = (0.5, 0.75, 0.9, 0.95, 1.0);
    #my @coverage = (0.5, 0.75);
	my @coverage = (0.95, 0.98, 0.99);
	foreach $cov (@coverage) {
        #my %pc_map = get_metric_pc_list($cov);
		#my $size = scalar keys %pc_map;
		#foreach $key (keys %pc_map) {
		#	print "$key\n";
		#}
		#print "size = $size\n";

		#my $need_more = 1;
		#my %pc_count = {};
        #foreach my $key (sort keys %pc_pilot_map) {
		#	my $injected_something = 0;
		#	$injected_something = 1; # temp fix for added pilots for metrics

		#	foreach my $acu (keys %{ $pc_pilot_map{$key} }) {
		#		if ($pc_pilot_map{$key}{$acu} == 0) {
		#			$injected_something = 1;
		#		}
		#	}

		#	if ($injected_something == 1) {
		#		#print "injected_something: $key \n";
		#		foreach my $acu (keys %{ $pc_pilot_map{$key} }) {
		#			if ($pc_pilot_map{$key}{$acu} == 1) {
		#				@words = split(/\./, $key);
		#				$pc = $words[0];
		#				$reg = $words[1];
		#				$src_dest = $words[2];

		#				if (!exists $pc_map{hex($pc)}) {
		#					next;
		#				}
		#				my $val = hex($pc);

		#				print "PC-reg: $key, $acu, $pcreg_high{$key}, $pc_pilot_map{$key}{$acu}\n";
		#				$high = int($pcreg_high{$key});
		#				for ($bit=0; $bit<$high; $bit+=1) {
		#					#print "$curr_app.$pc.$acu.0.$reg.$bit.$src_dest.4\n";
		#					$need_more += 1;
		#					if (exists $pc_count{$pc}) {
		#						$pc_count{$pc} += 1;
		#					} else {
		#						$pc_count{$pc} = 1;
		#					}
		#				}
		#			}
		#		}
		#	}
		#	# my $more_1000 = 0; 
		#	# my $less_1000 = 0; 
		#	# foreach my $pc (keys %pc_count) {
		#	# 	if(pc_count{$pc} > 1000) {
		#	# 		$more_1000 += 1;
		#	# 	} else {
		#	# 		$less_1000 += 1;
		#	# 	}

		#	# 	print "$pc: $pc_count{$pc}\n";
		#	# }
		#	# print ">1000 = $more_1000, <1000 = $less_1000\n";
		#}
		print "Need more: $need_more\n";
	}
}

print "bad pilots = $bad_pilots\n";
