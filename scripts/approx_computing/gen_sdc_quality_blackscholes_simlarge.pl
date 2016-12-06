#!/usr/bin/perl -w
use Scalar::Util 'looks_like_number';
use POSIX qw/ceil/;
#use strict;
#use Exporter;

########### README ###########
#This script takes the quality information for Output Corruption outcomes, generated using the
#user specified quality metric (provided in detailed_app_analysis.pl) and divides them into DDC,
# SDC-Good, SDC-Bad and SDC-Maybe depending on (optional) quality thresholds specified by the user.
# More information in 
######### END README #########

#Read inputs
my $numArgs = $#ARGV + 1;
if($numArgs != 1) {
	print "Usage: perl generate_sdc_quality.pl <app_name> \n";  
	#print "$ARGV[0] \n";
	die;
}
my $app_name = $ARGV[0];

############## START USER CONFIG/ CUSTOM FUNCTIONS ########################
#user set configuration parameters for Approxilyzer
#Thresholds. A value of 1 indicates that the user wishes to set these thresholds
############ USER TOLERABLE QUALITY THRESHOLD (OPTIONAL) #######################
my $set_output_quality_threshold = 0; #1 indicates that the user wishes to set a threshold for output quality
my $output_quality_threshold = 10; # all outputs with quality better than threshold is good/tolerable
				   # and outputs with quality worse than threshold is bad/not-tolerable

########### USER THRESHOLDS FOR SDC-GOOD AND SDC_BAD (OPTIONAL). NOT NEEDED IF QUALITY THRESHOLD (ABOVE) IS SET #######################
my $set_sdc_good_threshold = 1; #1 indicates that the user wishes to set the SDC-Good threshold to the value assigned in sdc_good_threshold
my $sdc_good_threshold = 0.0001; #Any deviation less than this threshold is considered SDC-Good.
my $set_sdc_bad_threshold = 1; #1 indicates that the user wishes to set the SDC-Bad threshold to the value assigned in sdc_bad_threshold
my $sdc_bad_threshold = 100; #Any deviation greater than this threshold is considered SDC-Bood.

########## USER SPECIFIED DISCRETIZATION FOR SDC_MAYBE (OPTIONAL) #################
my $set_discretize_sdc_maybe = 1; #If this value is 1, then SDC-Maybe can be discretized according to the algorithm in subroutine discretize_sdc_maybe

############# LOGS TO WRITE ###########################
my $write_oc_log_with_quality = 1; #rewrite the sdc log appended with quality information for each pilot
my $write_per_pc_summary_for_approx = 1; #write the summary (worse case quality info for each PC studied) with application to identifying approximate instructions.
my $write_per_pc_summary_for_resiliency = 1; #write the summary (worse case quality info for each PC studied) with application to resiliency protection

#is this error an SDC-Good?
sub is_an_sdc_good
{
	local ($diff) = ($_[0]);
	#if the quality threshold is set, everything below the threshold is SDC-Good
	#if SDC_Good threshold is set, everything below it is SDC-Good.
	if((($set_output_quality_threshold == 1) and ($diff < $output_quality_threshold)) or (($set_sdc_good_threshold == 1) and ($diff < $sdc_good_threshold)))
	{
		return 1; #SDC-Good	
	}

	return 0; #Not SDC-Good
}

#is this error an SDC-Bad?
sub is_an_sdc_bad
{
	local ($err_percent) = ($_[0]);
	#if the quality threshold is set, everything above the threshold is SDC-Bad
	#if SDC_Bad threshold is set, everything above it is SDC-Bad.
	if((($set_output_quality_threshold == 1) and ($err_percent > $output_quality_threshold)) or (($set_sdc_bad_threshold == 1) and ($err_percent > $sdc_bad_threshold)))
	{
		return 1; #SDC-Bad	
	}

	return 0; #Not SDC-Bad

}

#function to discretize SDC-Maybe
sub discretize_sdc_maybe
{
	local ($err_percent) = ($_[0]);
	my $discretized_percent = ceil($err_percent);
	return $discretized_percent; 
}

############## END USER CONFIG/ CUSTOM FUNCTIONS ########################

my $FAULT_LIST_OUTPUT="$ENV{APPROXILYZER}/fault_list_output/";
my $input_file ="$FAULT_LIST_OUTPUT/injection_results/full_sdc_list/$app_name.full_sdc_list";
my $out_path = "$FAULT_LIST_OUTPUT/approxilyzer_results/final_output_impact_list/";
my $sdc_file ="$FAULT_LIST_OUTPUT/injection_results/parsed_results/${app_name}_all_sdcs.txt";
my $detected_file ="$FAULT_LIST_OUTPUT/injection_results/parsed_results/${app_name}_all_detected.txt";
my $masked_file ="$FAULT_LIST_OUTPUT/injection_results/parsed_results/${app_name}_all_masked.txt";
my $disasm_file = "$ENV{APPROXILYZER}/workloads/apps/${app_name}/${app_name}.dis";
#output logs bath
my $oc_log_with_quality = "$FAULT_LIST_OUTPUT/injection_results/parsed_results/${app_name}_all_oc_quality.txt";
my $per_pc_summary_for_approx = "$FAULT_LIST_OUTPUT/injection_results/parsed_results/${app_name}_pc_summary_approx.txt";
my $per_pc_summary_for_resiliency = "$FAULT_LIST_OUTPUT/injection_results/parsed_results/${app_name}_pc_summary_resiliency.txt";

my %pilot_outcome_quality = (); #Hash (Key=)$pid -> (Value =)quality degradation. This number depends on metric that is chosen to acess quality degradation
my %pilot_outcome_category = (); #Hash (Key = )$pid -> (Value =) outcome category number. Different outcome categories are :
				#Masked, Detected, DDC (Detectable Data Corruption), SDC-good, SDC-Maybe, SDC-Bad
my %pilot_outcome_num_category = (); #Hash (Key = )$pid -> (Value =) outcome category number. The greater the number the more egrerious the outcome for
				     # that application
my %pc_final_category =(); # PC -> final output category (SDC-Good, DDC etc.). This will be the worst category of all the pilots associated withe the PC
my %pc_final_quality =(); # PC ->final quality . This is the worst quality bound of all the pilots associated with that PC.
my %pc_final_num_category =(); # PC -> final output category in numerics. This will be the worst category of all the pilots associated withe the PC
my %sdc_pop_map = (); #pilot_uniq_id->population

sub parse_quality_info
{
	local($sdc_info, $pid) = ($_[0], $_[1]) ;
	my ($weird_val,$max_diff, $err_percent,$max_faulty_val) = 0;
	my @words = split(/;/,$sdc_info);
	my $acceptability = $words[0];
	my $quality_info = $words[1];
	if($acceptability =~ /Eggregious/)
	{
		$pilot_outcome_category{$pid} = "Detectable-Data-Corruption";
		$pilot_outcome_quality{$pid} = "Mismatch in number of outputs" 

	}
	#for the tolerable cases find where on the SDC level each pilot falls
	elsif($acceptability =~ /Tolerable/)
	{	
		my @info = split(/,/,$quality_info); 
		
		$weird_val = $info[0];
		$max_diff = $info[5];
		$err_percent = $info[8];
		$max_faulty_val = $info[16];
		#if there are any weird values then it is eggregious
		if ($weird_val == 1)#these can be detected
		{
			$pilot_outcome_category{$pid} = "Detectable-Data-Corruption";
			$pilot_outcome_quality{$pid} = "Infinity/NaN/AlphaNum";
		}
		elsif ($max_faulty_val >= 500) #detected
		{
			#we can put detectors that will detect if the output
			#value is greater than a particular $ value
			#and if it is then detect the error
			$pilot_outcome_category{$pid} = "Detectable-Data-Corruption";
			$pilot_outcome_quality{$pid} = "Out of range bound";

		}
		else
		{
			if ($weird_val != 0)
			{
				die;
			}
			if(&is_an_sdc_good($max_diff)) #sdc-good is determined from abs-max-diff
			{
				$pilot_outcome_category{$pid} = "SDC-Good";
				$pilot_outcome_quality{$pid} = $max_diff;
			}
			elsif(&is_an_sdc_bad($err_percent)) #sdc-bad is determined from max-rel-error (percent)
			{
				$pilot_outcome_category{$pid} = "SDC-Bad";
				$pilot_outcome_quality{$pid} = $err_percent;
			}
			else #these are sdc-maybe
			{
				$pilot_outcome_category{$pid} = "SDC-Maybe";
				if($set_discretize_sdc_maybe == 1)
				{
					$pilot_outcome_quality{$pid} = &discretize_sdc_maybe($err_percent);
				}
				else
				{
					$pilot_outcome_quality{$pid} = $err_percent;	 	
				}		
			}

		}
	}
}

sub assign_each_masked_and_detected_pilot_category_and_quality
{
	open (DET_FILE, "<$detected_file") or die $!;
	while (<DET_FILE>)
	{
		my $s_line = $_;
		chomp($s_line);
		my @words = split (/:/, $s_line);
		my $pid = $words[1]; 
		if ((exists $pilot_outcome_category{$pid}) or (exists $pilot_outcome_quality{$pid}))
		{
			die;
		}
		$pilot_outcome_category{$pid} = "Detected";
		$pilot_outcome_quality{$pid} = -1; # quality degradation is bad	
			
	}
	close (DET_FILE);

	#do same for masked
	open (MASK_FILE, "<$masked_file") or die $!;
	while (<MASK_FILE>)
	{
		my $s_line = $_;
		chomp($s_line);
		my @words = split (/:/, $s_line);
		my $pid = $words[1]; 
		if ((exists $pilot_outcome_category{$pid}) or (exists $pilot_outcome_quality{$pid}))
		{
			die;
		}
		$pilot_outcome_category{$pid} = "Masked"; # masked category is 0
		$pilot_outcome_quality{$pid} = 0; # quality degradation is 0
	}
	close (MASK_FILE);

}

sub assign_numerical_category
{
	local($app_type) = $_[0];
	if ($app_type eq "resiliency")
	{
		#order of eggregiousness for error outcomes(for resiliency) is Masked, Detected, DDC, SDC-Good, SDC-Maybe, SDC-Bad
		foreach my $pid (sort keys %pilot_outcome_category)
		{
			if ($pilot_outcome_category{$pid} eq "Masked")
			{
				$pilot_outcome_num_category{$pid} = 0;	
			}
			elsif ($pilot_outcome_category{$pid} eq "Detected")
			{
				$pilot_outcome_num_category{$pid} = 1;	
			}
			elsif ($pilot_outcome_category{$pid} eq "Detectable-Data-Corruption")
			{
				$pilot_outcome_num_category{$pid} = 2;	
			}
			elsif ($pilot_outcome_category{$pid} eq "SDC-Good")
			{
				$pilot_outcome_num_category{$pid} = 3;	
			}
			elsif ($pilot_outcome_category{$pid} eq "SDC-Maybe")
			{
				$pilot_outcome_num_category{$pid} = 4;	
			}
			elsif ($pilot_outcome_category{$pid} eq "SDC-Bad")
			{
				$pilot_outcome_num_category{$pid} = 5;	
			}
			else
			{
				die;
			}
	
		}

	}
	elsif ($app_type eq "approx")
	{
		#order of eggregiousness for error outcomes is Masked, SDC-Good, SDC-Maybe, SDC-Bad, DDC, Detected
		foreach my $pid (sort keys %pilot_outcome_category)
		{
			if ($pilot_outcome_category{$pid} eq "Masked")
			{
				$pilot_outcome_num_category{$pid} = 0;	
			}
			elsif ($pilot_outcome_category{$pid} eq "Detected")
			{
				$pilot_outcome_num_category{$pid} = 5;	
			}
			elsif ($pilot_outcome_category{$pid} eq "Detectable-Data-Corruption")
			{
				$pilot_outcome_num_category{$pid} = 4;	
			}
			elsif ($pilot_outcome_category{$pid} eq "SDC-Good")
			{
				$pilot_outcome_num_category{$pid} = 1;	
			}
			elsif ($pilot_outcome_category{$pid} eq "SDC-Maybe")
			{
				$pilot_outcome_num_category{$pid} = 2;	
			}
			elsif ($pilot_outcome_category{$pid} eq "SDC-Bad")
			{
				$pilot_outcome_num_category{$pid} = 3;	
			}
			else
			{
				die;
			}
		}
	}
	else
	{
		die;
	}

}

sub assign_per_pc_category_and_quality
{
	#go through each pilot, find which PC it belongs to and then assign it correctly
	#empty per pc hash
	%pc_final_num_category = ();
	%pc_final_category = ();
	%pc_final_quality = ();

	foreach my $pid (sort keys %pilot_outcome_category)
	{
		
		#get the PC from the PID
		my @pid_words = split(/\./, $pid);
		my $pc  = $pid_words[1];
		#sanity check
		if (! exists $pilot_outcome_quality{$pid})
		{
			printf ("Error %s, %d\n", $pid, $pilot_outcome_category{$pid});
			die;
		}
		if (exists $pc_final_num_category{$pc})
		{
			#replace if this pilot is more eggregious
			if($pilot_outcome_num_category{$pid} > $pc_final_num_category{$pc})
			{
				$pc_final_category{$pc} = $pilot_outcome_category{$pid};
				$pc_final_num_category{$pc} = $pilot_outcome_num_category{$pid};
				$pc_final_quality{$pc} = $pilot_outcome_quality{$pid};
			}
			elsif($pilot_outcome_num_category{$pid} == $pc_final_num_category{$pc})
			{
				#if the outcomes are any of the SDC-good, SDC-bad or SDC-maybe, assign the larger quality degradation to the PC
				if(($pilot_outcome_category{$pid}eq "SDC-Good") or ($pilot_outcome_category{$pid} eq "SDC-Maybe")or 
				($pilot_outcome_category{$pid} eq "SDC-Bad"))
				{
					if($pc_final_quality{$pc} < $pilot_outcome_quality{$pid})
					{
						$pc_final_quality{$pc} = $pilot_outcome_quality{$pid};
					}
				}
			}
			else
			{
				#do nothing
			}

		}
		else
		{
			$pc_final_num_category{$pc} = $pilot_outcome_num_category{$pid};
			$pc_final_category{$pc} = $pilot_outcome_category{$pid};
			$pc_final_quality{$pc} = $pilot_outcome_quality{$pid};

		}
	}
	
}

sub gen_disassembly_with_impact
{
	local($output_log) = $_[0];

	open (OUT_LOG, ">$output_log") or die $!;
	open (DIS_FILE, "<$disasm_file") or die $!;
	while (<DIS_FILE>)
	{
		my $s_line = $_;
		chomp($s_line);
		if ($s_line =~ /:/) #there is an instruction here
		{
			my @words = split (/:/, $s_line);
			my $pc = $words[0];
			$pc =~ s/^\s+//;
			if (exists $pc_final_category{$pc})
			{
				printf OUT_LOG ("%s: %s : %s : %s\n", $pc, $s_line, $pc_final_category{$pc}, $pc_final_quality{$pc});	
			}

		}
			
	}
	close DIS_FILE;
	close OUT_LOG;
}


#parse the file that have the sdc info for the various pilots and parse quality information
open (OUTCOME_FILE, "<$input_file") or die $!;
while (<OUTCOME_FILE>) {
	my $s_line = $_;
	chomp($s_line);
	my @words = split (/:/, $s_line);
	my $pid = $words[0]; # unique id per pilot
	my $outcome = $words[2]; 
	if ($outcome eq "") # a result was not produced for whatever reason
	{
		printf("WARNING: The pilot id %s does not have quality information\n",$pid);
	}
	
	if ($outcome eq "SDC")
	{
        	my $sdc_info = $words[3]; #This has the string that has all the info about the deviation of sdc output from the golden output

		if ((exists $pilot_outcome_category{$pid}) or (exists $pilot_outcome_quality{$pid}))
		{
			printf("ERROR: This pilot id (pid) %s exists already\n", $pid);
			die;
		} 
		# Parse the quality information for the given pilot
		&parse_quality_info($sdc_info, $pid);
		
	}
}
close(OUTCOME_FILE);

#Parse the sdc file and add to the population to the sdc_pop_map.
#Also write the oc log with quality info (error category and quality)
if ($write_oc_log_with_quality == 1)
{
	open (QUALITY_LOG, ">$oc_log_with_quality") or die $!;
	
	open (SDC_FILE, "<$sdc_file") or die $!;
	while (<SDC_FILE>) 
	{
		my $s_line = $_;
		chomp($s_line);
		my @words = split (/:/, $s_line);
		my $pop = $words[0];
		my $pid = $words[1]; # unique id per pilot
		if (exists $sdc_pop_map{$pid})
		{
			die;
		} 
		else
		{
			$sdc_pop_map{$pid} = $pop;
		}

		if (exists $pilot_outcome_category{$pid})
		{
			printf QUALITY_LOG ("%s:%s:%s\n", $s_line, $pilot_outcome_category{$pid}, $pilot_outcome_quality{$pid});
		}
		else
		{
			printf ("ERROR pid %s has no quality information\n",$pid);
		}

	}
	close(SDC_FILE);
	close(QUALITY_LOG);
}

&assign_each_masked_and_detected_pilot_category_and_quality(); #populate the outcome category (quality means nothing) for masked and detected cases
if ($write_per_pc_summary_for_approx == 1)
{
&assign_numerical_category("approx");
&assign_per_pc_category_and_quality();
&gen_disassembly_with_impact($per_pc_summary_for_approx);
}

if ($write_per_pc_summary_for_resiliency == 1)
{
&assign_numerical_category("resiliency");
&assign_per_pc_category_and_quality();
&gen_disassembly_with_impact($per_pc_summary_for_resiliency);

}

