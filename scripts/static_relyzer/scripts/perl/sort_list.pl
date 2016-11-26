#!/usr/bin/perl

use Cwd;
use FileHandle;
#use lib '/home/sadve/shari2/research/relyzer/perl';
use lib '/shared/workspace/amahmou2/relyzer/perl';
use common_subs;

my $numArgs = $#ARGV + 1;
if($numArgs != 1) {
	print "Usage: perl sort_list.pl <simics_files_list>\n";  
	print "$ARGV[0] \n";
	die;
}

my $simics_file_name = $ARGV[0];

print "simics files list: $simics_file_name \n";

my %pilot_map = ();
my $count = 0;
open IN_STATIC, $simics_file_name or die "$!: $simics_file_name";
while (<IN_STATIC>) { # for every line
	my $line = $_;
	chomp($line);
	my @words = split(/\./,$line);
	$pilot = $words[2];
	if(exists $pilot_map{$pilot}) {
		push(@{$pilot_map{$pilot}}, $line);
	} else {
		$pilot_map{$pilot} = [$line];
	}
	$count++;
}
close IN_STATIC;

open OUT_LIST, ">$simics_file_name.sorted";
my $num_keys = 0;
foreach $key (sort keys %pilot_map) {
	$num_keys++;
	foreach $line (@{$pilot_map{$key}}) {
		print OUT_LIST "$line\n"; 
	}
}
close OUT_LIST;

$val = $count/$num_keys;
print "jobs per group: $val\n"
