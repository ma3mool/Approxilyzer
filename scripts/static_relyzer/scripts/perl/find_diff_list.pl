#!/usr/bin/perl

use Cwd;
use FileHandle;
use lib '/home/sadve/shari2/research/relyzer/perl';
use common_subs;

my $numArgs = $#ARGV + 1;
if($numArgs != 4) {
	print "Usage: perl gen_all_simics_files.pl <app_name> <static_old_file_name> <static_new_file_name> <agen/reg/both>\n";  
	print "$ARGV[0] \n";
	print "$ARGV[1] \n";
	print "$ARGV[2] \n";
	print "$ARGV[3] \n";
	die;
}
my $app_name = $ARGV[0];
my $static_old_file_name = $ARGV[1];
my $static_new_file_name = $ARGV[2];
my $agen_reg = $ARGV[3];

my $prune_type = "all";

my $bounding_end = 0;

my $pwd = cwd();
# $app_name = common_subs::get_actual_app_name($app_name);

my %pc_fault_list_old = ();
my %pc_fault_list_new = ();
open IN_STATIC, $static_old_file_name or die "$!: $static_old_file_name";
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

	$pc_fault_list_old{$pc} = $line;
	#print "Adding.. $pc $line";
}
close IN_STATIC;

@keys = keys %pc_fault_list_old;
$size = $#keys;
print "size of old: $size \n";


open IN_STATIC, $static_new_file_name or die "$!: $static_new_file_name";
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

	$pc_fault_list_new{$pc} = $line;
}
close IN_STATIC;

print "size of new: ". keys(%pc_fault_list_new) ."\n";

open OUT_TEST, ">output/diff_lists/$app_name\_$app_name\_fault_list.txt";

foreach $key (keys %pc_fault_list_new) {

	if( exists $pc_fault_list_old{$key}) {

			if( $pc_fault_list_new{$key} ne $pc_fault_list_old{$key}) {

				my @words_new = split (/:/, $pc_fault_list_new{$key});
				my @words_old = split (/:/, $pc_fault_list_old{$key});
				# frequecy[]:pc:dep_type:{dep_pc,diff_cycle}:faults
				# 0- frequency
				# 1 - pc
				# 2 - dep_type
				# 3 - dep_pc
				# 4 - faults
				if( $words_new[0] eq $words_old[0] && $words_new[1] eq $words_old[1] && $words_new[2] eq $words_old[2] && $words_new[3] eq $words_old[3] ) {
					print OUT_TEST "$words_new[0]:$words_old[1]:$words_new[2]:$words_old[3]:";

					my $fault_list_new = $words_new[4];
					my $fault_list_old = $words_old[4];
					#chomp($fault_list_new);
					#chomp($fault_list_old);

					my @faults_new =  split(/\) \(/, $fault_list_new);
					my @faults_old =  split(/\) \(/, $fault_list_old);

					my $min = 0;
					my $max = 0;
					if ($#faults_new > $#faults_old) {
						$min = $#faults_old;
						$max = $#faults_new;
					} else {
						$min = $#faults_new;
						$max = $#faults_old;
					}

					for( $i=0; $i<=$min; $i++) { 
						$fault_new = $faults_new[$i]; 
						$fault_old = $faults_old[$i]; 

						@elems_new = split(/,/, $fault_new);
						@elems_old = split(/,/, $fault_old);

						if( $elems_new[0] eq $elems_old[0] && $elems_new[1] eq $elems_old[1] && $elems_new[2] eq $elems_old[2] ) {
								# Skip this fault
								# print "Skipping: $fault_new, $fault_old\n";
						} else {
								if($fault_new =~ /^ \(/) {
									chomp($fault_new);
									if($fault_new =~ /\)$/) {
										print OUT_TEST " $fault_new";
									} else {
										$fault_new .= ")";
										print OUT_TEST "+$fault_new";
									}
								} elsif($fault_new =~ /\)$/) {
									chomp($fault_new);
									$fault_new = "($fault_new";
									print OUT_TEST " $fault_new";
								} else {
									$fault_new = "($fault_new)";
									print OUT_TEST " $fault_new";
								}
						}
					}
					$min++;
					for($i=$min; $i<=$max; $i++) {
						if(exists $faults_new[$i]) {
							$fault_new = $faults_new[$i];
							if($fault_new =~ /^ \(/) {
								$fault_new .= ")";
								print OUT_TEST "$fault_new";
							} elsif($fault_new =~ /\)$/) {
								chomp($fault_new);
								$fault_new = "($fault_new";
								print OUT_TEST " $fault_new";
							} else {
								$fault_new = "($fault_new)";
								print OUT_TEST " $fault_new";
							}
						}
					}
					print OUT_TEST "\n";
				} else {
				 	print OUT_TEST "$pc_fault_list_new{$key}" ;

				}
			}
	} else {
		print OUT_TEST "$pc_fault_list_new{$key}" ;
	}
}

close OUT_TEST; 
