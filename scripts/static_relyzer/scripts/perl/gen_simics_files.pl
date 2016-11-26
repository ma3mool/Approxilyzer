#!/usr/bin/perl

#input merged_fault_list/output/, samples

use Cwd;

my $BIT_GAP = 8;
my $simics_dir  = "./simics_files/";

my $numArgs = $#ARGV + 1;
if($numArgs != 3) {
	print "Usage: perl gen-simics-files.pl <app_name> <merged_fault_faultname> <sample_filename> \n"; 
	die;
}
my $app_name = $ARGV[0];
my $merged_filename = $ARGV[1];
my $sample_filename = $ARGV[2];

open (SIMICS_FILE_LIST, ">simics_file_list.txt");

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

my $simics_file_counter = 0;

open IN_MERGED_FILE, $merged_filename or die "$!: $merged_filename";
#@pcs = ("0x1000015d0","0x100001ae8","0x100001d08","0x1000021ec","0x1000022a8","0x100002444","0x100002580","0x100002590","0x1000025d8","0x100002610","0x100002678","0x1000026ec","0x100002820","0x100002848","0x100002878","0x1000028a0","0x10000170c","0x10000192c");
#@pcs = (
#"0x100004a40",
#"0x100007fe0",
#"0x100007eb4",
#"0x1000058a4",
#"0x10000222c",
#"0x100008140",
#"0x1000052a4",
#"0x100005a0c",
#"0x100005dd8",
#"0x100007248"
#);


while (<IN_MERGED_FILE>) {
	$line = $_;

	my @words = split (/:/, $line);
	my $dep_type =  $words[0];
	my $count = $words[1];
	my $cycle_list = $words[2];
	my $pc = $words[3];
	my $fault_list = $words[4];
#	my $skip = 1;
#	foreach $check_pc (@pcs) {
#		if($pc eq $check_pc) {
#			$skip = 0;
#		}
#	}
#	if($skip == 1) { 
#		next;
#	}
#	print "$pc $check_pc\n";
	
	if($dep_type == 0) {
		$cycle_list = trim($cycle_list);
		if ($cycle_list ne "") {
			my $num_faults = 0;
			@faults =  split(/\) \(/, $fault_list);
			my $arch_reg =  -1;
			my $high = -1;
			my $low = -1;
			my $src_dest = -1;
			foreach $fault (@faults) {
				@elems = split(/,/, $fault);
				if($elems[0] =~ /%([gloi][0-9]*)$/)  {
					$arch_regname = $1;
					$arch_reg = arch_reg_name2num($arch_regname);
					$high = $elems[1];
					$low = $elems[2];
					$low = trim($low);
					if($elems[3] =~ /([01]+)/) {
						$src_dest = $1;
					}
					$num_faults = $high-$low;
				}
			}
			if($num_faults == 0) {
				next;
			}

# 			write simics files for selected cycle 
			my @cycles = split (/ /, $cycle_list);
			my $c = 0;

			if( $#cycles > 2) { 
				print "@cycles\n";

				#next;
				#$rand_num  = rand();
				#if($rand_num > 0.10) {
				#	next;
				#}
			} else {
			#	next;
			#	$rand_num  = rand();
			#	if($rand_num > 0.01) {
			#		next;
			#	}
			}


		#	foreach  $cycle (@cycles) {
		#		for ($i=$low; $i<$high;$i+=$BIT_GAP) {
		#			$simics_file_counter += 1;
# 		#			cycle, arch_reg, bit
		#			my $simics_filename = "$simics_dir/$app_name.pc$pc.c$c.r$arch_reg.b$i.simics";
		#			print SIMICS_FILE_LIST "$simics_filename\n";
# 		#			print "$simics_filename\n";
		#			open OUT_SIMICS_FILE, ">$simics_filename";
		#			print OUT_SIMICS_FILE "\@sys.path.append(\".\")\n";
		#			print OUT_SIMICS_FILE "\@import simpoint\n";
		#			print OUT_SIMICS_FILE "\@simpoint.run_new_all(\'$app_name\',\'p0\',\'c$c\',\'$pc\',0,4,$i,4,$arch_reg,$src_dest,$cycle)\n";
		#			print OUT_SIMICS_FILE "quit\n";
		#			close OUT_SIMICS_FILE;
		#		}
		#		$c += 1;
		#	}

# 			write simics files for all the samples for this pc

			 open IN_SAMPLE_FILE, $sample_filename or die "$!: $sample_filename";
			 while (<IN_SAMPLE_FILE>) {
			 	$new_line = $_;

			 	my @new_words = split (/:/, $new_line);
			 	my $sample_pc =  $new_words[0];
			 	if($sample_pc eq $pc) { 
			 		my $sample_list =  $new_words[2];
			 		$sample_list = trim($sample_list);
			 		my @cycles = split (/ /, $sample_list);
			 		$s = 0;
			 		foreach  $cycle (@cycles) {
# 			 			this number is specific for blackscholes
#			 			$cycle = 152815135068 + $cycle;

			 			for ($i=$low; $i<$high;$i+=$BIT_GAP) {
			 				$simics_file_counter += 1;
#			 				cycle, arch_reg, bit
			 				my $simics_filename = "$simics_dir/$app_name.pc$pc.s$s.r$arch_reg.b$i.simics";
			 				print SIMICS_FILE_LIST "$simics_filename\n";
#			 				print "$simics_filename\n";
			 				open OUT_SIMICS_FILE, ">$simics_filename";
			 				print OUT_SIMICS_FILE "\@sys.path.append(\".\")\n";
			 				print OUT_SIMICS_FILE "\@import simpoint\n";
			 				print OUT_SIMICS_FILE "\@simpoint.run_new_all(\'$app_name\',\'p0\',\'s$s\',\'$pc\',0,4,$i,4,$arch_reg,$src_dest,$cycle)\n";
			 				print OUT_SIMICS_FILE "quit\n";
			 				close OUT_SIMICS_FILE;
			 			}
			 			$s += 1;
			 		}
			 	}
			 }
			 close IN_SAMPLE_FILE;
		}
	}
}

print "Number of simics files: $simics_file_counter \n";
close SIMICS_FILE_LIST;


