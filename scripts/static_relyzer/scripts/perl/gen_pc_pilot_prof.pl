#!/usr/bin/perl

use Cwd;
use FileHandle;
#use lib '/home/sadve/shari2/research/relyzer/perl';
use lib '/shared/workspace/amahmou2/relyzer/perl';
use common_subs;

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

my $prune_type = "all";

my $bounding_end = 0;
open IN_BOUNDING, "<bounding_pruning.txt";
while (<IN_BOUNDING>) { # for every line
	my $line = $_;
	my @words = split (/:/, $line);
	if($words[0] =~ /$app_name/) {
			$bounding_end = $words[1];
			last;
	}
}
close IN_BOUNDING;

my $pwd = cwd();
$app_name = common_subs::get_actual_app_name($app_name);

my @population_list = ();

my %pc_factor = ();
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
			if($agen_reg eq "reg" or $agen_reg eq "both") {
				if($elems[0] =~ /%([gloi][0-9]*)$/)  {
					$arch_regname = $1;
					$arch_reg = common_subs::arch_reg_name2num($arch_regname);
					$fault_type = 4;
				}elsif($elems[0] =~ /%fp$/)  {
					$arch_regname = "i6";
					$arch_reg = common_subs::arch_reg_name2num($arch_regname);
					$fault_type = 4;
				}elsif($elems[0] =~ /%sp$/)  {
					$arch_regname = "o6";
					$arch_reg = common_subs::arch_reg_name2num($arch_regname);
					$fault_type = 4;
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

			$elems[4] = common_subs::trim($elems[4]);
			my $extra_counting = -1;
			if($elems[4] =~ /^([0-1])\)$/)  {
				$extra_counting = $1;
			}

			my $factor  = ($high - $low) * ($extra_counting + 1);

			if($arch_reg == -1  || $src_dest == -1) {
					next;
			} else {
				$found = 1;
			}

			if($found == 1) { # if arch register is found in source or dest
				# get pilot list for leading branch

				if(exists $pc_factor{$dep_pc} ) {
						$pc_factor{$dep_pc} += $factor;
				} else {
						$pc_factor{$dep_pc} = $factor;
				}
			}
		} 
	}
}
close IN_STATIC;


my $num_not_added = 0;

open IN_CONTROL, $control_patterns_file_name or die "$!: $control_patterns_file_name";
while (<IN_CONTROL>) {
	my $d_line = $_;
	chomp($d_line);
	my @d_words = split (/:/, $d_line);
	my $priv_cycles = "";
   	my $my_pc = common_subs::trim($d_words[0]);
	my $population = common_subs::trim($d_words[2]);

	my $size = 0;
	if(exists $pc_factor{$my_pc}) { 
		$size = $pc_factor{$my_pc};
	}
	if($population > 1) {
		for($i=0; $i<$size; $i++) {
			push(@population_list, $population); 
		}
	} else {
		$num_not_added += $size ;
	}
}
close IN_CONTROL;

print " loaded control_patterns_file\n";

my $num_lines = 0;
open IN_STORE, $heap_patterns_file_name or die "$!: $heap_patterns_file_name";
while (<IN_STORE>) {
	my $d_line = $_;
	$d_line = substr $d_line, 0, 40;
	chomp($d_line);
	if($d_line ne "") {
		my @d_words = split (/:/, $d_line);
   		my $my_pc = common_subs::trim($d_words[0]);
		my $population = common_subs::trim($d_words[2]);

		my $size = 0;
		if(exists $pc_factor{$my_pc}) { 
			$size = $pc_factor{$my_pc};
		}

		$population = $population+1;
		if($population > 1) {
			for($i=0; $i<$size; $i++) {
				push(@population_list, $population); 
			}
		} else {
			$num_not_added += $size;
		}

		if($num_lines % 1000000 == 1) {
			print "num_not_added=$num_not_added, population_list_size=$#population_list\n";
		}
		$num_lines++;
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
   		my $my_pc = common_subs::trim($d_words[0]);
		my $population = common_subs::trim($d_words[2]);
		$population = $population+1;

		my $size = 0;
		if(exists $pc_factor{$my_pc}) { 
			$size = $pc_factor{$my_pc};
		}

		if($population > 1) {
			for($i=0; $i<$size; $i++) {
				push(@population_list, $population); 
			}
		} else {
			$num_not_added += $size ;
		}
	}
}
close IN_STORE;

print " loaded stack store_patterns_file\n";

print "num_not_added=$num_not_added, population_list_size=$#population_list\n";

@population_list = sort {$b <=> $a} @population_list;

my $total_population = $num_not_added;
foreach $pop_elem (@population_list) { 
	$total_population += $pop_elem;
}

print "Total population = $total_population\n";

open (SUMMARY_FILE, ">>pilot_pop_map.txt");
print SUMMARY_FILE "$app_name\t";
my @D = (0,20,40,60,80,90,95,96,97,98,99,99.9,99.99,100);
print "$bounding_end\n";
 
foreach $d (@D) { 
	$x = 100*(($d-$bounding_end)/(100-$bounding_end));
	if($x < 0) {
			next;
	}
	$num_pilots = 0;
	my $total_pop_top_x = int($total_population*$x/100);
	my $total_pop = 0;
	my $flag = 0;
	foreach $key (@population_list) {

		$total_pop += $key;
		$num_pilots++;
		if($total_pop > $total_pop_top_x) {
			$flag = 1;
			last;
		}
	}
	if($flag == 0) {
			#population list is exhausted, now look at not_added numbers
			print " Check!!!\n";
			$num_pilots += ($total_pop_top_x-$total_pop);
	}
	print SUMMARY_FILE "$num_pilots\t";
	print "$x: $num_pilots\n";
}

print SUMMARY_FILE "\n";
close SUMMARY_FILE;
