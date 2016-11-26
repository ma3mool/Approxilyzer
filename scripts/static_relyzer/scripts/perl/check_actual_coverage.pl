#!/usr/bin/perl

use Cwd;
use FileHandle;
use lib '/home/sadve/shari2/research/relyzer/perl';
use common_subs;

my $BIT_GAP = 1;
my $write_files = 1;
my $use_random_selection = 1;
my $additional_pilots = 1; # 1=no additional pilots, 10=9 more pilots per eq. group. Should NEVER be 0

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

print "\nApp name: $app_name \n";
# 	Static file name: $static_file_name 
# 	Mode change file: $mode_change_file_name
# 	Heap patterns: $heap_patterns_file_name
# 	Stack patterns: $stack_patterns_file_name
# 	Control patterns: $control_patterns_file_name
# 	Fault model: $agen_reg \n";

my $prune_type = "all";

my $pwd = cwd();
$app_name = common_subs::get_actual_app_name($app_name);

my %gap_pc_map = (); # pc->gap
my %def_pc_map = (); # pc.arch_reg_num->def_pc
sub populate_gap_pc_map ($) {
	my $app_str = shift;

	# clear the map first
	%gap_pc_map = ();

	my $output_file = "/home/sadve/shari2/research/relyzer/output/$app_str\_$app_str\_fault_list.txt";
	open IN_FAULT_LIST, $output_file or die "$!: $output_file";
	while (<IN_FAULT_LIST>) {
		my $d_line = $_;
		chomp($d_line);
		my @d_words = split (/:/, $d_line);

	    # frequecy[]:pc:dep_type:{dep_pc,diff_cycle}:faults
	    # 0- frequency, 1 - pc, 2 - dep_type, 3 - dep_pc, 4 - faults

		my $temp_pc = $d_words[1];
		$temp_pc = Math::BigInt->new($temp_pc);
	    my $dep_pc = "";
	    my $diff_cycle = "";
	    if($d_words[3] =~ /\{([0-9a-z]*)\, (-*[0-9]+)\}/) {
	        $dep_pc = $1;
	        $diff_cycle = $2;
	    }
		$gap_pc_map {$temp_pc} = $diff_cycle;

		my $hex_pc = sprintf ("0x%x", $temp_pc);
		@fault_list = split(/ \(/, $d_words[4]);
        # fault list : (0-reg, 1-max_bits, 2-is_address, 3-is_def, 4-extra_count, 5-def_pc) separated by spaces
        foreach (@fault_list) {
                 my $fault = $_;
                 chomp($fault);
                 if($fault ne "") {
                         if ($fault =~ /(.*)\)$/) {
                               $fault = $1;
                         }
                         # print "$fault\n";
                         my @elements = split(/\, /, $fault);
                         if(&common_subs::is_int_register( $elements[0] ) == 1) {
							 my $arch_reg = common_subs::arch_reg_name2num($elements[0]);
							 $def_pc_map {"$hex_pc.$arch_reg"} = $elements[5];
							 # print "$hex_pc $arch_reg:$elements[0] $elements[5]\n";
                         }
                 }
         }

	}
	close IN_FAULT_LIST;
}


