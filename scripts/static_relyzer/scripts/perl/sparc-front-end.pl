#!/usr/bin/perl

# This script cleans up the assembly file to contain only the instructions. This will make parsing easy in C/C++.
#
# output will contain the following information
# For instructions
# 1 c_frequency c_line asm_line pc op op1 op1-stack-heap? op1-address-data? op1-has-offset? op2 (similar options as op1)
# For labels
# 0 c_frequency c_line asm_line label 

use Cwd;

my $numArgs = $#ARGV + 1;
if($numArgs != 2) {
	print "Usage: perl sparc-front-end.pl <file_name> <program_name> \n"; 
}
my $file_name = $ARGV[0];
my $program_name = $ARGV[1];
#my $file_type = $ARGV[2];

my $APPS_PATH = "$ENV{RELYZER_SHARED}/workloads/apps/$program_name";
#$APPS_PATH .= "/$program_name";
print "APP PATH IS $APPS_PATH\n";

#my $input_file_name = "$APPS_PATH/$file_name.s";
my $dis_file_name = "$APPS_PATH/$program_name.dis";
my $output_file_name = "$APPS_PATH/$file_name.clean.dis";
#my $c_file_name = "$file_name.$file_type";
my $prof_file_name = "$APPS_PATH/$program_name.prof";

#open IN_FILE, $input_file_name or die "$!: $input_file_name";
open IN_FILE, $dis_file_name or die "$!: $dis_file_name";
open OUT_FILE, ">$output_file_name" or die "$!: $output_file_name";

sub remove_spaces($)
{
	my $string = shift;
	$string =~ s/\s//g;
	return $string;
}

# Formula : (Z^2 * p*(1-p) ) / (E^2)
# Z :
#   99% - 2.58
#   98% - 2.33
#   95% - 1.96 
#   90% - 1.645
my $E = 0.05;
my $Z = 2.58;
my $p = 0.5;

my $E_2 = $E*$E;
my $Z_2_p_1_p = $Z*$Z*$p*(1-$p);

my $total_faults = 0.0;
my $stat_remaining_faults = 0.0;

my %pc_line_freq_map = ();
open PROF_IN_FILE, $prof_file_name or die "$!: $prof_file_name";
while (<PROF_IN_FILE>) {
	my $prof_line = $_;
	chomp($prof_line);
	@words = split (/:/, $prof_line);
	# $words[0] is pc
	# $words[1] is asm_frequency
	#print " -$pc:$words[1]- \n";
	$pc_line_freq_map {$words[0]} = int($words[1]);
}
close PROF_IN_FILE;

my $start = 0;
my $line = "";
my $asm_line_number = 0;
my $c_line_number = 0;
my $c_line_frequency = 0;
my $pc = "";
my $not_pc = 1;
my $current_function_name = "";
while (<IN_FILE>) {
	$line = $_;
	$asm_line_number++;
	my $print = 0;
	my $is_instruction = 0;
	my $is_func_name = 0;
	my $opcode = "";
	my $comments = "";
	my $old_comments = "";
	my @ops;

	# start here 
	if($line =~ /disassembly for/) {
		$start = 1;
	}

	if($start) {

		if($line =~ /^([a-zA-Z_0-9\-\.]*)\(\)/) { # function name 
			$current_function_name = $1;
			$is_func_name = 1;
			$pc = "";
			$not_pc = 1;
			$print = 1;
		} elsif ($line =~ /^\s+([0-9a-fx]+):\s+[0-9a-f]+\s[0-9a-f]+\s[0-9a-f]+\s[0-9a-f]+\s+(.*)/) { # instruction
			#    0x100001a74:            d8 27 a7 6b  st        %o4, [%fp + 0x76b]
			$pc = $1;
			$instruction = $2;
			$is_instruction = 1;
			$print = 1;
			#print "pc=$1, instr=$2\n";
			# print "$asm_line_number ";
		}  elsif ($line =~ /^\s+([\.]*[0-9a-zA-Z_]+):$/) { # label
			$label = $1;
			$print = 1;
		}
	}

	if($is_instruction) { # parse instruction

		if ($instruction =~ /^([a-z,]+)\s*(.*)/) {
			$opcode = $1;

			$operands = $2;
			$operands =~ s/%sp/%o6/g;
			$operands =~ s/%fp/%i6/g;
			@ops = split(/,/, $operands);
			if($#ops != -1) { 
				while ( $ops[$#ops-1] =~ /^(.*)\!(.*)/) {
					$ops[$#ops] = $1; 
					$old_comments = $comments;
					$comments = $2;
				}
			}

			# special case for call
			if($opcode =~ /call/) {
				if($ops[0] =~ /<([0-9a-zx]+)>/) {
					$ops[0] = $1;
				}
				#print "opcode=$opcode operand=$ops[0]\n";
			}
			# special case for branch
			if($opcode =~ /^b/) {
				if($ops[$#ops] =~ /<([0-9a-zx]+)>/) {
					$ops[$#ops] = $1;
				}
				#print "opcode=$opcode last operand=$ops[$#ops]\n";
			}
			# special case for branch
			if($opcode =~ /^fb/) { # floating point branch instruction 
				if($ops[$#ops] =~ /<([0-9a-zx]+)>/) {
					$ops[$#ops] = $1;
				}
				#print "opcode=$opcode last operand=$ops[$#ops]\n";
			}

			# remove spaces from operands
			for ($i=0; $i<=$#ops; $i++) {
				#print "$ops[$i]:";
				$ops[$i] = remove_spaces($ops[$i]);
				#print "$ops[$i] ";
			}
			#print "\n";

			# replace retl with jmpl %o7+8, %g0
			if($opcode eq "retl") {
				$opcode = "jmpl";
				$ops[0] = "%o7+8";
				$ops[1] = "%g0";
			}
			# replace neg reg with sub %g0, reg, reg
			if($opcode eq "neg") {
				$opcode = "sub";
				if($#ops == 1) {
					$ops[2] = $ops[1];
					$ops[1] = $ops[0];
					$ops[0] = "%g0";
				} else {
					$ops[2] = $ops[0];
					$ops[1] = $ops[0];
					$ops[0] = "%g0";
				}
			}
			# replace sethi %hi(0x102993), %o5 with sethi %hi(0x102993), %o5, %o5  (one %o5 is use and the other is a def)
			if($opcode eq "sethi") {
				$ops[2] = $ops[1];
			}

		} else {
			$print = 0;
		}

		$c_line_frequency = 0;
		$c_line_frequency = $pc_line_freq_map{$pc} if exists $pc_line_freq_map{$pc};
		#print " =$pc:$c_line_frequency= \n";
		$total_faults += $c_line_frequency*$#ops*32;
		my $sample_size = $c_line_frequency;
		if($c_line_frequency > 100) { 
			$sample_size = ($c_line_frequency * $Z_2_p_1_p) / (($E_2*($c_line_frequency -1)) + $Z_2_p_1_p);
			$sample_size = int($sample_size+0.99);
		}
		$stat_remaining_faults += $sample_size*$#ops*32;
	}

	if($print) {
		if($is_instruction) { 
			print OUT_FILE "1 $c_line_frequency $c_line_number $asm_line_number $pc $opcode";
			my $i;
			for($i=0; $i<3; $i++) {
				if($i<=$#ops) {
					if($ops[$i] ne "") {
						if($opcode eq "prefetch") {
							print OUT_FILE " BLANK";
						} else {
							print OUT_FILE " $ops[$i]";
						}
					} else {
						print OUT_FILE " BLANK";
					}
				} else {
					print OUT_FILE " BLANK";
				}
			}
			if($opcode eq "call") {
				# for call instructions print the number of arguments and the arguments
				if($comments =~ /^\s+params\s+=\s+(%.*)\s+$/) {
					$comments = $1;
					my @arguments = split(/ /, $comments);
					my $args_size  = $#arguments+1;
					print OUT_FILE " $args_size";
					foreach (@arguments) {
						print OUT_FILE " $_";
					}
				} else {
					print OUT_FILE " 0";
				}
				# for return instructions print the number of return registers 
				if($old_comments =~ /^\s+Result\s+=\s+(%.*)\s*$/) {
						$old_comments = $1;
						my @arguments = split(/ /, $old_comments);
						my $args_size  = $#arguments+1;
						print OUT_FILE " $args_size";
						foreach (@arguments) {
							print OUT_FILE " $_";
						}
				} else {
					print OUT_FILE " 0";
				}

			}
			if($opcode eq "ret") {
				# for return instructions print the number of return registers 
				if($comments =~ /^\s+Result\s+=\s+(%.*)\s*$/) {
						$comments = $1;
						my @arguments = split(/ /, $comments);
						my $args_size  = $#arguments+1;
						print OUT_FILE " $args_size";
						foreach (@arguments) {
							print OUT_FILE " $_";
						}
				} else {
					print OUT_FILE " 0";
				}
			}

			print OUT_FILE "\n";
		} elsif ($is_func_name == 1) {
			print OUT_FILE "2 $c_line_frequency $c_line_number $asm_line_number $current_function_name\n";
		} else {
			print OUT_FILE "0 $c_line_frequency $c_line_number $asm_line_number $label\n";
		}
	}

}
close IN_FILE;
close OUT_FILE;

print "total faults = $total_faults\n";
print "remaining faults = $stat_remaining_faults\n";
if($total_faults != 0) {
	$pruning = $stat_remaining_faults/$total_faults;
} else {
	$pruning = -1;
}
print "pruning = $pruning\n";
