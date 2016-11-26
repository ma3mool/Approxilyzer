#!/usr/bin/perl

use Cwd;

my $BIT_GAP = 16;
my $write_files = 1;

my $numArgs = $#ARGV + 1;
if($numArgs != 3) {
	print "Usage: perl gen-simics-files.pl <app_name> <merged_file> <sample_filename> \n"; 
	die;
}
my $app_name = $ARGV[0];
my $merged_filename = $ARGV[1];
my $sample_filename = $ARGV[2];

my $pwd = cwd();
my $simics_dir  = "$pwd/$app_name\_simics_files/";
print "$simics_dir: $sample_filename\n";
open (SIMICS_FILE_LIST, ">simics_file_list_$app_name.txt");

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
my $num_pilots = 0;

open (IN_SAMPLE_FILE, "$sample_filename");

while (<IN_SAMPLE_FILE>) {
	$line = $_;

	my @words = split (/:/, $line);
	my $pc =  $words[0];
	$pc = trim($pc);
	my $pilot = $words[1];
	$pilot = trim($pilot);
	my $population = $words[2];
	$population = trim($population);
	my $sample_list = $words[3];
	$sample_list = trim($sample_list);
	my @samples = split (/ /, $sample_list);

	open IN_MERGED_FILE, $merged_filename or die "$!: $merged_filename";
	while (<IN_MERGED_FILE>) {
		my $found = 0;
		$my_line = $_;
		my @words = split (/:/, $my_line);
		my $temp_pc = $words[3];
		my $fault_list = $words[4];
		@faults =  split(/\) \(/, $fault_list);
		if($pc ne $temp_pc) {
			next ;
		} else {
			foreach $fault (@faults) {
				my $arch_reg =  -1;
				my $high = -1;
				my $low = -1;
				my $src_dest = -1;
				@elems = split(/,/, $fault);
				if($elems[0] =~ /%([gloi][0-9]*)$/)  {
					$arch_regname = $1;
					$arch_reg = arch_reg_name2num($arch_regname);
				}elsif($elems[0] =~ /%(fp)$/)  {
					$arch_regname = "i6";
					$arch_reg = arch_reg_name2num($arch_regname);
				}elsif($elems[0] =~ /%(sp)$/)  {
					$arch_regname = "o6";
					$arch_reg = arch_reg_name2num($arch_regname);
				}

				$high = $elems[1];
				$low = $elems[2];
				$low = trim($low);
				$high = trim($high);
				if($elems[3] =~ /([01]+)/) {
					$src_dest = $1;
				}
				if($arch_reg == -1  || $src_dest == -1) {
					next;
				} else {
					$found = 1;
				}
				$rand_num  = rand();
				if($rand_num > 0.02) {
					next;
				}

#				write simics file for pilot 
				my $c = 0;
				for($i = $low; $i < $high; $i=$i+$BIT_GAP) {
					$simics_file_counter += 1;
					$num_pilots += 1;
# 					cycle, arch_reg, bit
					my $simics_filename = "$simics_dir/$app_name.pc$pc.p$pilot.c$c.r$arch_reg.b$i.simics";
					if($write_files == 1) {
						print SIMICS_FILE_LIST "$simics_filename\n";
						open OUT_SIMICS_FILE, ">$simics_filename";
						print OUT_SIMICS_FILE "\@sys.path.append(\".\")\n";
						print OUT_SIMICS_FILE "\@import simpoint\n";
						print OUT_SIMICS_FILE "\@simpoint.run_new_all(\'$app_name\',\'p0\',\'c$c\',\'$pc\',0,4,$i,4,$arch_reg,$src_dest,$pilot,$pilot)\n";
						print OUT_SIMICS_FILE "quit\n";
						close OUT_SIMICS_FILE;
					} else {
						print "$simics_filename\n";
					}
				}

#				write simics files for all the samples for this pilot

				my $s = 0;
				foreach $sample (@samples) {
					 for ($i=$low; $i<$high;$i+=$BIT_GAP) {
						$simics_file_counter += 1;
#						cycle, arch_reg, bit
						my $simics_filename = "$simics_dir/$app_name.pc$pc.p$pilot.s$s.r$arch_reg.b$i.simics";
						if($write_files == 1) {
							print SIMICS_FILE_LIST "$simics_filename\n";
							open OUT_SIMICS_FILE, ">$simics_filename";
							print OUT_SIMICS_FILE "\@sys.path.append(\".\")\n";
							print OUT_SIMICS_FILE "\@import simpoint\n";
							print OUT_SIMICS_FILE "\@simpoint.run_new_all(\'$app_name\',\'p0\',\'s$s\',\'$pc\',0,4,$i,4,$arch_reg,$src_dest,$pilot,$sample)\n";
							print OUT_SIMICS_FILE "quit\n";
							close OUT_SIMICS_FILE;
						} else {
					 		print "$simics_filename\n";
						}
					}
					$s += 1;
				}

			}
			last;
			if($found == 0) {
				print "Reg not found $pc\n";
			}
		}
	}
	close IN_MERGED_FILE;
}

print "Number of pilots: $num_pilots\n";
print "Number of simics files: $simics_file_counter \n";

close SIMICS_FILE_LIST;

