#!/usr/bin/perl

use Fcntl qw(:flock :seek);
use Time::HiRes qw( usleep nanosleep );
my $numArgs = $#ARGV + 1;
if($numArgs < 1) {
        print "Usage: perl run_injection_job.pl <simics_file_info> (<file_to_log_result_in>) \n";
        die;
}
my $simics_file_info = $ARGV[0];
my $result_file = $ARGV[1];

# 1. run the fault injeciton
# 2. check if its an unknown
# 3. run completion job
# 4. check if it's detected, masked, or SDC 

system ("date");

$CHKPT_FILE_DIR = "/scratch/amahmou2/chkpt";
$CRAFF_DIR = "/scratch/amahmou2/craff";
$LOCAL_LOG_DIR = "/scratch/amahmou2/logs";
$COMPARE_CHKPT_DIR = "/mounts/sadve/tmp";
$COMPARE_GOLDEN_CHKPT_DIR = "/scratch/amahmou2/compare_golden_checkpoints";
$OUTPUT_DIR= "/scratch/amahmou2/outputs";
$GENERATED_SIMICS_FILE_DIR= "/scratch/amahmou2/generated_simics_files";
$LOG_DIR = $ENV{'APPROXILYZER'} .+ "/fault_list_output/injection_results/parallel_outcomes";
print "$LOG_DIR \n";
$SIMICS_PATH = $ENV{'GEMS_PATH'} .+ "/simics/home/dynamic_relyzer";
$OUTPUT_MAX_ATTEMPTS = 2;
$MAX_PROCEEDS = 2;
$MAX_USED_SPACE = 90;
$MAX_TRIES_FOR_SPACE = 100;

sub log_error {
	# open file
	my $output_filename = "$LOG_DIR/errors.log";
	print "$output_filename\n";
	open(MYFILE, "+< $output_filename");
	# lock file 
	my $MAX_ATTEMPTS = 500;
	my $num_attempts = 0;
    my $lower_limit = 1;
    my $upper_limit = 625;
	while ($num_attempts < $MAX_ATTEMPTS) {
		if (flock(MYFILE, 2|4)) { 
			# got the lock
			last;
		}
        $num_attempts++;

        #my $rand_num = int(rand($upper_limit-$lower_limit)) + $lower_limit; 
        #usleep($rand_num);
        usleep(200);
	}
	
	if ($num_attempts < $MAX_ATTEMPTS) {
		# append 
		seek(MYFILE, 0, SEEK_END); #truncate(MYFILE, 0);
        #print MYFILE "Not enough space to run job ::$simics_file_info:$result_file\n";
		print MYFILE "$simics_file_info\n";
		# release lock and close file
		close(MYFILE);
	} else {
		close(MYFILE);
	}
}


#check if there is enough space left in /scratch before you run (we need space to write files)
my $disk_space=`df /scratch | awk 'FNR>1'`;
my @used_space = split(' ',$disk_space);
my $percent_used = $used_space[4];
$percent_used =~ s/%//g;
#print "Hmm $percent_used\n";


$tries = 0;
while ($percent_used > $MAX_USED_SPACE) 
{
    if( $tries < $MAX_TRIES_FOR_SPACE)
    {
        usleep(500000);
        $disk_space =`df /scratch | awk 'FNR>1'`;
        @used_space = split(' ',$disk_space);
        $percent_used = $used_space[4];
        $percent_used =~ s/%//g;
        $tries++;
    }
    
    else
    {
	    print "Not enough space left!. \n";
	    log_error();
	    exit;
    }
}

#if ($percent_used > $MAX_USED_SPACE)
#{
#	print "Not enough space left!. \n";
#
#	log_error();
#	exit;
#}





# check if scratch directories exist
if( -d "/scratch/amahmou2/") {
	if( -d $CHKPT_FILE_DIR) {
		# great!
	} else {
		system("mkdir $CHKPT_FILE_DIR");
	}
	if( -d $CRAFF_DIR) {
		# great!
	} else {
		system("mkdir $CRAFF_DIR");
	}
	if( -d $COMPARE_CHKPT_DIR) {
		# great!
	} else {
		system("ls $COMPARE_CHKPT_DIR");
	}
	if( -d $COMPARE_GOLDEN_CHKPT_DIR) {
		# great!
	} else {
		system("mkdir $COMPARE_GOLDEN_CHKPT_DIR");
	}
	if( -d $OUTPUT_DIR) {
		# great!
	} else {
		system("mkdir $OUTPUT_DIR");
	}
	if( -d $GENERATED_SIMICS_FILE_DIR) {
		# great!
	} else {
		system("mkdir $GENERATED_SIMICS_FILE_DIR");
	}
	if( -d $LOCAL_LOG_DIR) {
		# great!
	} else {
		system("mkdir $LOCAL_LOG_DIR");
	}

} else {
	# print "Please ensure that you have /scratch/venktgr2 in place\n";
	# create /scratch/venktgr2
	system("mkdir /scratch/amahmou2");
	system("mkdir $CHKPT_FILE_DIR");
	system("mkdir $COMPARE_CHKPT_DIR");
	system("mkdir $CRAFF_DIR");
	system("mkdir $OUTPUT_DIR");
	system("mkdir $GENERATED_SIMICS_FILE_DIR");
	system("mkdir $LOCAL_LOG_DIR");
	system("mkdir $COMPARE_GOLDEN_CHKPT_DIR");
}


my $num_proceeded = 1;
while ($num_proceeded >= 1) {

	# create simics_file
	my @info = split(/\./,$simics_file_info);
	my $app_name = $info[0];
	my $inj_pc = $info[1];
	my $adjusted_pilot = $info[2];
	my $c = $info[3];
	my $arch_reg = $info[4];
	my $inj_index = $info[5];
	my $src_dest = $info[6];
	my $fault_type = $info[7];
	if ($c > 0) { 
		$prefix_str = "s";
	} else {
		$prefix_str = "c";
	}
	my $simics_file = "$GENERATED_SIMICS_FILE_DIR/$app_name.pc$inj_pc.p$adjusted_pilot.$prefix_str$c.r$arch_reg.b$inj_index.sd$src_dest.t$fault_type.simics";
	open OUT_SIMICS_FILE, ">$simics_file";
	print OUT_SIMICS_FILE "\@sys.path.append(\"$SIMICS_PATH\")\n"; #FIX HERE: Point to Dynamic Dir
	print OUT_SIMICS_FILE "\@import simpoint\n";
	print OUT_SIMICS_FILE "\@simpoint.run_new_all(\'$app_name\',\'p0\',\'$prefix_str$c\',\'$inj_pc\',0,$fault_type,$inj_index,4,$arch_reg,$src_dest,$adjusted_pilot,$adjusted_pilot)\n";
	print OUT_SIMICS_FILE "quit\n";
	close OUT_SIMICS_FILE;
	
    #print "GENERATED SIMICS FILE!"
	my $string = "";
	open IN_SIMICS_FILE, "<$simics_file" or die "couldn't open $simics_file: $!";
	while (<IN_SIMICS_FILE>) {
		$line = $_;
		if($line =~ /run_new_all\((.*)\)/) {
			$string = $1;
			last;
		}
	}
	close IN_SIMICS_FILE;
	sub destring($)
	{
	        my $string = shift;
		if($string =~ /\'(.*)\'/) {
			$string = $1;
		}
	        return $string;
	}
	@params = split(/,/, $string);
	my $app = destring($params[0]);
	my $phase = destring($params[1]);
	my $seqnum = destring($params[2]);
	my $pc = destring($params[3]);
	my $core = $params[4];
	my $type = $params[5];
	my $bit = $params[6];
	my $stuck = $params[7];
	my $faultreg = $params[8];
	my $srcdest = $params[9];
	my $pilot = $params[10];
	my $injcycle = $params[11];
	print " $app, $phase, $core, $type, $bit, $stuck, $faultreg, $srcdest, $injcycle, $seqnum, $pilot, $pc\n";
	
	my $final_result = "";
	
	my $run_name = "$app.pc$pc.p$pilot.c$core.t$type.b$bit.s$stuck.r$faultreg.d$srcdest.$seqnum";
	my $checkpoint_filename = "$CHKPT_FILE_DIR/$run_name.chkpt";
	my $pickle_filename = "$CHKPT_FILE_DIR/$run_name.p";
	my $craff_filename = "$CRAFF_DIR/$run_name.diff.craff";
	
	system("rm -f $checkpoint_filename $checkpoint_filename.raw $checkpoint_filename.memory0_image $checkpoint_filename.*sd25B_2_0_image $pickle_filename");
	#system("rm -f $checkpoint_filename $checkpoint_filename.raw $checkpoint_filename.memory0_image");
	my $completion_output_filename = "$LOCAL_LOG_DIR/$run_name.completion_output";
	
	sub log_final_result {
		# open file
        my $lower_limit = 1;
        my $upper_limit = 625;
        my $rand_num = int(rand($upper_limit-$lower_limit)) + $lower_limit; 
		
        my $output_filename = "$LOG_DIR/$app-$rand_num.outcomes";
        #my $output_filename = "$LOG_DIR/$app.outcomes";
        #my $output_filename = "$LOG_DIR/$app.debug.outcomes";

        #if ($result_file ne "")
        #{
        #		$output_filename = "$LOG_DIR/$result_file.outcomes";
        #}
		open(MYFILE, "+< $output_filename");
		# lock file 
		my $MAX_ATTEMPTS = 500;
        my $upper_limit = 625;
		while ($num_attempts < $MAX_ATTEMPTS) {  # keep running till acquire lock.
			if (flock(MYFILE, 2|4)) { 
				# got the lock
				last;
			}
            $num_attempts++;
            #usleep($rand_num);
			usleep(100);
		}

		
		if ($num_attempts < $MAX_ATTEMPTS) {
			# append 
			seek(MYFILE, 0, SEEK_END); #truncate(MYFILE, 0);
            #print MYFILE "$run_name:$final_result\n";
			print MYFILE "$simics_file_info:$result_file:$final_result\n";
			# release lock and close file
			close(MYFILE);
		} else {
			system ("echo $simics_file_info:$result_file:$final_result >> $LOG_DIR/$run_name.completion_output");
            #system ("echo $final_result >> $LOG_DIR/$run_name.completion_output");
			close(MYFILE);
		}
	}

	#for cases where the final result couldn't be gotten (could not read the craff file), log them somewhere do they can be retried.
	sub log_for_retry {
		# open file
	    my $output_filename = "$LOG_DIR/$app.retries";


		open(MYFILE, "+< $output_filename");
		# lock file 
		my $MAX_ATTEMPTS = 500;
		my $num_attempts = 0;
        my $lower_limit = 1;
        my $upper_limit = 625;
		while ($num_attempts < $MAX_ATTEMPTS) {
			if (flock(MYFILE, 2|4)) { 
				# got the lock
				last;
			}

            $num_attempts++;
            #my $rand_num = int(rand($upper_limit-$lower_limit)) + $lower_limit; 
            #usleep($rand_num);
            usleep(200);
		}
		
		if ($num_attempts < $MAX_ATTEMPTS) {
			# append 
			seek(MYFILE, 0, SEEK_END); #truncate(MYFILE, 0);
            #print MYFILE "$simics_file_info:$result_file:$final_result\n";
			print MYFILE "$simics_file_info\n";
			# release lock and close file
			close(MYFILE);
		} else {
            #system ("echo $final_result >> $LOG_DIR/$run_name.completion_output");
			close(MYFILE);
		}
	}


	
	# remove logs 
	sub clean_logs {
		#system (" rm -f $LOG_DIR/$run_name.fault_log.stat");
		#system (" rm -f $LOG_DIR/$run_name.trace_log");
        system ("rm -f $LOCAL_LOG_DIR/$run_name.capture_log");
	
		# clean up - remove all temp files
        system ("rm -f $simics_file $simics_file.completion $completion_output_filename");
        system ("rm -f $checkpoint_filename $checkpoint_filename.raw $checkpoint_filename.memory0_image $checkpoint_filename.*sd25B_2_0_image $craff_filename $pickle_filename");
	}
	
	# 1. Run the faut injection job
    print "Hmm..Running injection run...\n";
	$result = `$SIMICS_PATH/simics -stall -no-win $simics_file`;
        #system ("$SIMICS_PATH/simics -stall -no-win $simics_file");
    print "Should be here\n";
	print "result is: $result\n";
    print "END OF RESULT!!!!!\n";
    #print "Should now be here\n";
	if($result =~ /Fault not detected/) {
		print "Fault Not Detected\n";
		# check if the fault is injected
	 	if($result !~ /Old value =/) {
	 		# double check if the fault is injected
	 		if($result !~ /Injecting Transient fault/) {
	 			#system("echo \"Error : Fault Not Injected\" >> $completion_output_filename");
			#	$final_result = "Error";
	 		#	system ("date");
			#	log_final_result();
	 		#	clean_logs();
			#	print "Fault Not Injected\n";
	 		#	exit;
	 		}
	 	}
	}
	
	# clean_logs();
	# exit;
	
	if($result =~ /Fault detected/) {
		print "Fault Detected\n";
		$final_result = "Detected";
		# simplify parsing
		if ($result =~ /symptom detected (.*)$/) {
			$final_result = "$final_result:$1";
		}
        if($result =~ /Detected App:(.*)/) {
			$why = $1;
            $symp = 1 ;
			$final_result = "Detected:App:$why";
		}
		# system("echo \"Detected\" >> $completion_output_filename");
		system ("date");
		log_final_result();
		clean_logs();
		exit;
	}
	
	# 2. Only unkown run comes to this point
	# Run completion job
	# checkpoint name = $CHKPT_FILE_DIR/$app.pc$pc.c$core.t$type.b$bit.s$stuck.r$faultreg.d$srcdest.seqnum.chkpt))
	
	#exit(1);
	
	if (-e "$checkpoint_filename")
	{
		# Do the bit to remove opal and ruby from the final checkpoint, and add sd1
		print "Cleaning final checkpoint: Removing opal and ruby from checkpoint and adding sd1\n";
		open( IN_CONFIG, "<$checkpoint_filename" );
		open( OUT_CONFIG, ">$checkpoint_filename.out");
		$no_print = 0;
        
        # MAKE SCALABLE
        my $sd1_config = "/home/venktgr2/checkpts/spec_opensolaris/sd1_config" ; # EDIT HERE
		my $add_sd1 = 1 ;
		my $has_sd1 = 1 ;
		while (<IN_CONFIG>) {
		    $line = $_;
		    if( $line =~ /OBJECT opal0 TYPE opal \{/ ) {
		        $no_print=3;
		    }
		    if( $line =~ /OBJECT ruby0 TYPE ruby \{/ ) {
		        $no_print=3;
		    }
	        if( $line =~ /timing_model: ruby0/ ) {
	        } elsif( $line =~ /OBJECT amber0 TYPE amber \{/ ) {
		        $no_print=3;
		    } elsif( $line =~ /amber0/ ) {
		        $no_print=1;
	        } elsif( $line =~ /OBJECT amber_mem TYPE amber \{/ ) {
		        $no_print=3;
		    } elsif( $line =~ /amber_mem/ ) {
		        $no_print=1;
		    }
	
		    if ($no_print != 0) {
		        $no_print--;
		    } else {
		        if($line =~ /targets: \(\(0, sd0, 0\), \(6, cd25B_2_6, 0\), \(7, glm0, 0\)\)/) {
		                if($add_sd1) {
		                        $line = "\ttargets: ((0,sd0,0), (1,sd1,0), (6,cd25B_2_6,0), (7,glm0,0))\n" ;
		                }
		                $has_sd1 = 0 ;
		        } elsif($line =~ /no_window: 1/) {
		                $line = "\tno_window: 0\n" ;
		        }
		        print OUT_CONFIG $line;
		    }
		}
		close(IN_CONFIG);
		close(OUT_CONFIG);
		if($add_sd1 && !$has_sd1) {
		#       print "Adding sd1 config to checkpoint\n" ;
		        system("cat $sd1_config >> $checkpoint_filename.out") ;
		}
		system("mv $checkpoint_filename.out $checkpoint_filename") ;
	
		system ("date");
		
		# create completion simics file
		system("cp $simics_file $simics_file.completion") ;
		system("sed -i \"s/run_new_all/run_complete_new/g\" $simics_file.completion") ;

	
		# run completion job
		print "Completion Run...\n";
		system("$SIMICS_PATH/simics -no-win -echo -x $simics_file.completion");
		system ("date");
	
		# Get output
	
		# Boot sol9 only if this didn;t give symp>10m
		open(OUTPUT,"<$completion_output_filename") ;
		my $symp = 0 ;
		my $masked = 0 ;
		while(<OUTPUT>) {
			$line = $_;
            print "OUTPUT IS $line\n";
	        if($line =~ /Detected (.*)/) {
                print "GOT INSIDE THE IF\n";
				$why = $1;
                $symp = 1 ;
				$final_result = "Detected:$why";
                last ;
	        }
	        if($line =~ /Masked/) {
                $masked = 1 ;
				$final_result = "Masked";
                last ;
	        }
		}
		close OUTPUT;
	
		if(!$symp && !$masked) {
			# Then boot this in sol9 and get the output from it
			system("sed -i \"s/run_complete_new/sol9_get_output/g\" $simics_file.completion") ;
	
			my $attempts = 0;
			while($attempts < $OUTPUT_MAX_ATTEMPTS) {
				$attempts++;
				# make OUTPUT_MAX_ATTEMPTS attempts to obtain the output file from the craff file
				system("$SIMICS_PATH/simics -no-win -echo -x $simics_file.completion");
				$output_file = "/scratch/amahmou2/outputs/$run_name.output";
				$output_file_err = "/scratch/amahmou2/outputs/$run_name.output.err";
				if(-e "$output_file") {
					$attempts = $OUTPUT_MAX_ATTEMPTS;
	
					# test for SDC and zip output and move to /home/sadve/shari2/outputs/
					
					my $golden_file = "";

                    #MAKE THIS SCALABLE
                    #$golden_file = "/home/shari2/outputs/fully_optimized/$app_name.output";
                    #$golden_file = "/shared/workspace/amahmou2/iso/fully_optimized_output/$app_name.output";
                    $golden_file = $ENV{APPROXILYZER} .+ "/workloads/golden_output/$app_name.output";
                    print "golden file is: $golden_file\n";
					
					if(-e "$output_file") {
						#print "$output_file exists\n";
						$result = `diff -q $output_file $golden_file`;
						if ($result eq "") {
							# system("echo \"Masked\" >> $completion_output_filename");
							$final_result = "Masked";
							print "Masked\n";
						} else {

							my @golden_f = ();
							my @faulty_f = ();
							open IN_GOLDEN, "<$golden_file";
							while(<IN_GOLDEN>) {
								my $line = $_;
								push(@golden_f, $line);
							}
							close IN_GOLDEN;

							open IN_FAULTY, "<$output_file";
							while(<IN_FAULTY>) {
								my $line = $_;
								push(@faulty_f, $line);
							}
							close IN_FAULTY;

							my $flag = 0;
                            #print "$#golden_f \t $#faulty_f\n";
							if($#golden_f == $#faulty_f) {
								for($i=0; $i<=$#golden_f; $i++) {

									my $g_line = $golden_f[$i];
									if($g_line =~ /PROCESS STATISTIC/) {
											# dont compare files from this point on.
											last;
									}
									my $f_line = $faulty_f[$i];
									# print "$golden_f[$i] -- $faulty_f[$i]\n";
									# print "$f_line -- $g_line \n";
									if( $g_line ne $f_line) {
                                        #print "$g_line \t $f_line \n";
											$flag = 1;
											last;
									}
								}
							}  else {
									$flag = 1;
							}
							
							if($flag == 1) {
								$temp_result = `diff $output_file $golden_file`;
                                #print $temp_result;
								# system("echo \"SDC\" >> $completion_output_filename");
								$final_result = "SDC";
								my $curr_l = -1;
                                $temp_result = `perl $ENV{APPROXILYZER}/scripts/injections_scripts/detailed_app_level_analysis.pl $app_name $output_file`;
                                #$temp_result = `perl $SIMICS_PATH/scripts/app_level_analysis.pl $app_name $output_file`;
								print $temp_result;
								print "SDC acceptability = $temp_result\n";
                                if($temp_result =~ /SDC/) {
									chomp($temp_result);
									my @words = split(/:/,$temp_result);
									$curr_l = $words[1];
									#print "Radha = $curr_l\n";
								}
								#print "Debug 1 SDC:$curr_l\n";
								$final_result = "SDC:$curr_l";
							} else {
								$final_result = "Masked";
								print "Masked\n";
							}
						}
                                
                                
                                
                                
                        #   OLD STUFF: WORKS WITH SIVA's APP_LEVEL_ANALYSIS 
                        #        if($temp_result =~ /SDC ([0-9]*)/) {
                        #				$curr_l = $1;
                        #		}
                        #		print "SDC:$curr_l\n";
                        #		$final_result = "SDC:$curr_l";
                        #	} else {
                        #		$final_result = "Masked";
                        #		print "Masked\n";
                        #	}
                        #}
                        

                        # DISABLE THIS LINE FOR DEBUGGING AND PRINTING OUTPUT!
                        system("rm $output_file\n"); # don't remove file for now, instead store it somewhere for analsis

                        # ENABLE THIS LINE FOR DEBUGGING
                        #system("mv $output_file $ENV{GEMS_PATH_SHARED}/simics/home/dynamic_relyzer/output/sdc_outputs/\n"); # its a better idea to zip the file to store space on disk see next two lines for example
                        #system("mv $output_file /shared/workspace/amahmou2/outputs/\n"); # its a better idea to zip the file to store space on disk see next two lines for example
						# Radha system("mv $output_file /home/sadve/venktgr2/GEMS/simics/home/dynamic_relyzer/approximate_computing/sdc_outputs\n"); # its a better idea to zip the file to store space on disk see next two lines for example
                        #system("gzip $output_file\n");
                        # system("mv $output_file.gz /shared/workspace/amahmou2/outputs/\n");
                         # MAKE SCALABLE
                         # system("mv $output_file.gz /home/sadve/shari2/outputs/\n");
						
					} else {
						print "Expecting an output file!\n";
					}
	
				} 
				if(-e "$output_file_err") {
					$result = `cat $output_file_err`;
					if ($result ne "") {
						# system("echo \"Detected\" >> $completion_output_filename");
						$final_result = "Detected:Error";
						print "Detected Error\n";
					} 
					system("rm $output_file_err\n");
				}
			}
		}
		system ("date");
		# compare output
	
		# write result
	} 

	if($final_result eq "") {
		$num_proceeded++; 
		if($num_proceeded >= 3) {
			$num_proceeded = 0;
			log_for_retry();
		}
	} else {
		$num_proceeded = 0;
		log_final_result();
	}
	clean_logs();
}

