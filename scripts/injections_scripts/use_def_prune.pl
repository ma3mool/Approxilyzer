#!/usr/bin/perl
# This script generates the SDC level value for the definition pilot that is pruned to match that of 
# first use
my $numArgs = $#ARGV + 1;
if($numArgs != 1) {
	print "Usage: perl use_def_prune.pl <app_name> \n";  
	print "$ARGV[0] \n";
	die;
}

my $write_output =1 ;

my $app_name = $ARGV[0];
#my $inj_type = $ARGV[1];
#my $sdc_file ="/home/shari2/GEMS/simics/home/live_analysis/parsed_results/".$app_name."_all_sdcs.txt";
#my $outcome_file = "/home/venktgr2/GEMS/simics/home/dynamic_relyzer/approximate_computing/detailed_pilots_sdc_outputs/all_sdcs/".$app_name.".outcomes";
#my $output_file = "/home/venktgr2/GEMS/simics/home/dynamic_relyzer/approximate_computing/detailed_pilots_sdc_outputs/use_def_prune/".$app_name."_def.outcomes";


my $injection_results="$ENV{APPROXILYZER}/fault_list_output/injection_results/"; 
my $sdc_file ="$injection_results/parsed_results/${app_name}_all_sdcs.txt";
my $outcome_file ="$injection_results/full_sdc_list/temp_files/${app_name}_just_sdcs.tmp";
my $output_file = "$injection_results/full_sdc_list/temp_files/${app_name}_def.outcomes";
#my $outcome_file ="/shared/workspace/amahmou2/Relyzer_GEMS/src_GEMS/simics/home/dynamic_relyzer/output/".$app_name.".outcomes_final";

my %is_this_use = (); #key->pid, #value - 0 (if it is a pruned def) or 1 (probably a use or source injection)
my %src_or_dest = (); #key->pid, #value - is this a src or dest reg. Dest reg is 1, source is 0.
my %udid_map = (); #key ->id for use-def pair(common part of pilot), value->pid
my %sdc_level_map = (); # Only for pilots that resulted in SDC. key->pid, #value - outcome like SDC1, SDC2 ect.
my %pilot_not_sdc = (); #key -> pid, #value - outcome that is not SDC
my %pc_map = (); #key->pid, #value -> pc
 
 
open SDC_FILE, "<$sdc_file" or die $!;
while(<SDC_FILE>)
{
	my $line = $_;
	chomp($line);
	my @words = split(/:/, $line); # sdc number and job name
        my $sdc = $words[0];
	my $bug_bit = $words[2];
	my $pid = $words[1];
	my @t_words = split(/\./,$words[1]); # split job name to get pc
	my $pc = $t_words[1]; # get PC
    #print "$pc\n";
	my $udid = $t_words[2].".".$t_words[3].".".$t_words[4].".".$t_words[5].".".$t_words[7]; #the id that the use and def share (cycle, register, bit. PC willNOT be same
    #print "$udid\n";
	my $dest_src_register = $t_words[6]; #is it a source or dest reg? Dest reg will ahve a value of 1
#add sdc map for sdc caused by fault injection in destination reg

	if (exists $is_this_use{$pid})
	{
		die;
	}
	else
	{
		$is_this_use{$pid} = $bug_bit;
	}

	if (exists $src_or_dest{$pid})
	{
		die;
	}
	else
	{
		$src_or_dest{$pid} = $dest_src_register;
	}

	if (($bug_bit == 1) and ($dest_src_register == 0))
	{
		if (exists $udid_map{$udid})
		{
			die;
		}
		else
		{
			$udid_map{$udid} = $pid;
		}
	
	}
	if(exists $pc_map{$pid})
	{
		die;
	}
	else
	{
		$pc_map{$pid} = $pc;
	}
	
	#if (($bug_bit == 0)||(($bug_bit == 1)&&($dest_src_register == 1)))
}
close SDC_FILE;

open RES_FILE, "<$outcome_file" or die $!;
while(<RES_FILE>)
{
	my $line = $_;
	chomp($line);
	my @words = split(/:/, $line); # sdc number and job name
	my $pid = $words[0];
	my $outcome = $words[2];
	#my $sdc_level = $words[3];

	if ($words[1] ne "")
	{
		die;
	}

	if ($outcome eq "") # a result was not produced for whatever reason
	{
		$outcome = "NR";
	}
	#we want to track the different SDC_levels
	
	if($outcome eq "SDC")
	{
		my $sdc_level = $words[3];
		if ($sdc_level eq "")
		{
			print "ERROR in sdc tolerability level\n";
			die;
		}
		$outcome = "SDC".":".$sdc_level;
	

		if (exists $sdc_level_map{$pid})
		{
			die;
		}
		else
		{
			$sdc_level_map{$pid} = $outcome;	
		}

	}
	else
	{
		if (exists $pilot_not_sdc{$pid})
		{
			die;
		}
		else
		{
			$pilot_not_sdc{$pid} = $outcome;	
		}

	}

	
}
close RES_FILE;

if ($write_output = 1)
{
	open (LOG, ">$output_file") or die $!;
	foreach my $pid (sort keys %pilot_not_sdc)
	{
		$outcome = $pilot_not_sdc{$pid};
		if ($outcome eq "Masked")
		{
			#find out if it is a pruned def.
			#if so, then find the corresponding def and copy the value
			if ((exists $is_this_use{$pid}) and (exists $src_or_dest{$pid} ))
			{
				if (($is_this_use{$pid} == 0) and ($src_or_dest{$pid} == 1))  #pruned def
				{
					my @parts = split(/\./, $pid);
					my $def_pc = $parts[1];
					my $udid = $parts[2].".".$parts[3].".".$parts[4].".".$parts[5].".".$parts[7];
					if (exists $udid_map{$udid})
					{
						$use_pid = $udid_map{$udid};
						if (exists $sdc_level_map{$use_pid})
						{
					            	#make sure pcs are different
					        	if ($def_pc eq $pc_map{$use_pid})
							{
								die; #use and def can't have same PCs
							}
						    	#copy to the def what is in the use
					            	#write to a file in the same format
					            	printf LOG ("%s::%s\n",$pid,$sdc_level_map{$use_pid});
						}
						else
						{
							print "There is no result for Use ($use_pid) for this def ($pid) yet\n";
						}
					}
					else
					{
						die;
					}
				}
				else
				{
					print "Masked but not def $pid\n";
					#die;
				}
			}
			else
			{
                print $pid;
				#print "Hello $pid\n";
                die;
			}
		}
		else
		{
			print "$pid : $outcome\n";
			die;			
		}
	}

	close LOG;
}
