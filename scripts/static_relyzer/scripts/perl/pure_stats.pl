#!/usr/bin/perl

use Cwd;

my $numArgs = $#ARGV + 1;
if($numArgs != 1) {
	print "Usage: perl pure_stats.pl <program_name>\n"; 
}
my $program_name = $ARGV[0];

my $APPS_PATH = getcwd;
#$APPS_PATH .= "/apps/optimized/$program_name";
$APPS_PATH .= "/apps/$program_name";

my $prof_file_name = "$APPS_PATH/$program_name.prof";

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
my $Z = 1.96;
my $p = 0.5;

my $E_2 = $E*$E;
my $Z_2_p_1_p = $Z*$Z*$p*(1-$p);

my %pc_line_freq_map = ();

my $total_faults = 0;
my $remaining_faults = 0;

open PROF_IN_FILE, $prof_file_name or die "$!: $prof_file_name";
while (<PROF_IN_FILE>) {
	my $prof_line = $_;
	chomp($prof_line);
	@words = split (/:/, $prof_line);
	# $words[0] is pc
	$pc = $words[0];
	# $words[1] is asm_frequency
	$line_freq = $words[1];
	#print " -$pc:$words[1]- \n";

	$total_faults += $line_freq;

	if($line_freq > 100) { 
		my $sample_size = ($line_freq * $Z_2_p_1_p) / (($E_2*($line_freq -1)) + $Z_2_p_1_p);
		$sample_size = int($sample_size+0.99);
		$remaining_faults += $sample_size;
	}
}
close PROF_IN_FILE;

print "total faults = $total_faults\n";
print "remaining faults = $remaining_faults\n";
