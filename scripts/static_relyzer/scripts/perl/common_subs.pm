#!/usr/bin/perl

package common_subs;
use Exporter;
@ISA = ('Exporter');
@EXPORT = qw(get_actual_app_name trim arch_reg_name2num is_fp_register is_int_register is_register);

sub get_actual_app_name($) {
	$app_name = shift ;
#	if($app_name eq "blackscholes_simlarge") {
#		$app_name = "blackscholes_simlarge";
##       $app_name = "blackscholes_abdul";
##		$app_name = "blackscholes_opt_abdul";
#	}
#	if($app_name eq "fluidanimate") {
##	    $app_name = "fluidanimate_simsmall";
#		$app_name = "fluidanimate_opt_abdul";
#    }
#	if($app_name eq "blackscholes_input_run_00000") {
#        $app_name = "blackscholes_input_run_00000";
#	}
#    if($app_name eq "blackscholes_input_run_00001") {
#        $app_name = "blackscholes_input_run_00001";
#	}
#	if($app_name eq "blackscholes_input_run_00002") {
#        $app_name = "blackscholes_input_run_00002";
#	}
#	if($app_name eq "blackscholes_input_run_00003") {
#        $app_name = "blackscholes_input_run_00003";
#	}
#	if($app_name eq "blackscholes_input_run_00004") {
#        $app_name = "blackscholes_input_run_00004";
#	}
#	if($app_name eq "streamcluster") {
#		$app_name = "streamcluster_simsmall";
##		$app_name = "streamcluster_opt_abdul";
#	}
#	if($app_name eq "swaptions_simsmall") {
#		$app_name = "swaptions_simsmall";
##		$app_name = "swaptions_opt_abdul";
#	}
#	if($app_name eq "lu_reduced") {
#		$app_name = "lu_reduced";
##		$app_name = "lu_opt_abdul";
#	}
#	if($app_name eq "fft_small") {
#		$app_name = "fft_small";
##		$app_name = "fft_opt_abdul";
#	}
#	if($app_name eq "ocean") {
##		$app_name = "ocean_small";
#		$app_name = "ocean_opt_abdul";
#	}
#	if($app_name eq "water_small") {
#		$app_name = "water_small";
##		$app_name = "water_opt_abdul";
#	}
#	if($app_name eq "libquantum") {
##		$app_name = "libquantum_test";
#		$app_name = "libquantum_opt_abdul";
#	}
#	if($app_name eq "omnetpp") {
##		$app_name = "omnetpp_test";
#		$app_name = "omnetpp_opt_abdul";
#	}
#	if($app_name eq "mcf") {
##		$app_name = "mcf_test";
#		$app_name = "mcf_opt_abdul";
#	}
#	if($app_name eq "gcc") {
##		$app_name = "gcc_test";
#		$app_name = "gcc_opt_abdul";
#	}
	return $app_name;
}

sub get_app_str ($) {
    my $app_name = shift;
    return $app_name;
#    if($app_name eq "blackscholes_simlarge") {
#         #if($app_name eq "blackscholes_opt_abdul") {
#        return "blackscholes_simlarge";
#    } 
#    elsif($app_name eq "blackscholes_input_run_00000") {
#        return "blackscholes_input_run_00000";
#    }
#    elsif($app_name eq "blackscholes_input_run_00001") {
#        return "blackscholes_input_run_00001";
#    }
#    elsif($app_name eq "blackscholes_input_run_00002") {
#        return "blackscholes_input_run_00002";
#    }
#    elsif($app_name eq "blackscholes_input_run_00003") {
#        return "blackscholes_input_run_00003";
#    }
#    elsif($app_name eq "blackscholes_input_run_00004") {
#        return "blackscholes_input_run_00004";
#    }
#    elsif($app_name eq "fluidanimate_opt_abdul") {
#            return "fluidanimate";
#    } elsif($app_name eq "streamcluster_simsmall") {
#            return "streamcluster";
#    } elsif($app_name eq "swaptions_simsmall") {
#            return "swaptions_simsmall";
#    } elsif($app_name eq "lu_reduced") {
#            return "lu_reduced";
#    } elsif($app_name eq "fft_small") {
#            return "fft_small";
#    } elsif($app_name eq "ocean_opt_abdul") {
#            return "ocean";
#    } elsif($app_name eq "water_small") {
#            return "water_small";
#    } elsif($app_name eq "gcc_opt_abdul") {
#            return "gcc";
#    } elsif($app_name eq "mcf_opt_abdul") {
#            return "mcf";
#    } elsif($app_name eq "omnetpp_opt_abdul") {
#            return "omnetpp";
#    } elsif($app_name eq "libquantum_opt_abdul") {
#            return "libquantum";
#    }
#	return "";
}


# Perl trim function to remove whitespace from the start and end of the string
sub trim($)
{
	my $string = shift;
	$string =~ s/^\s+//;
	$string =~ s/\s+$//;
	return $string;
}

#input: register name in str format (e.g., "g1", "l4")
sub arch_reg_name2num($)
{
	my $arch_regname = shift;
	my $class = -1;
	my $reg_num = -1;
	if($arch_regname =~ /([golifd])([0-9]*)/) {
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
	} elsif($class eq 'f') { #floating point don't have a categorization. all start at 0.
        $logical = 0;
    } elsif($class eq 'd') { #same with double floating point
        $logical = 0;
    }

	$logical += $reg_num;
    #print "arch2logical: $arch_regname -> $logical\n";

	return $logical;
}

sub is_fp_register()
{
	my $reg = $_;
	if(is_register($reg) == 1) {
		if ($reg =~ /\%fp/) {
			return 0;
		} 
		if ($reg =~ /\%d/) {
			return 1;
		} elsif ($reg =~ /\%f/) {
			return 1;
		}
		return 0;
	}
	return 0;
}

sub is_int_register()
{
	my $reg = $_;
	if(is_register($reg) == 1) {
		if ($reg =~ /\%fp/) {
			return 1;
		} 
		if ($reg =~ /\%d/) {
			return 0;
		} elsif ($reg =~ /\%f/) {
			return 0;
		}
		return 1;
	}
	return 0;
}

sub is_register()
{
	my $reg = $_;
	if ($reg =~ /\%icc/) {
		return 0;
	}
	if ($reg =~ /\%xcc/) {
		return 0;
	}
    if ($reg =~ /\%fsr/) {
        return 0;
    }
	if ($reg =~ /\%/) {
		return 1;
	}
	return 0;
}




1;

