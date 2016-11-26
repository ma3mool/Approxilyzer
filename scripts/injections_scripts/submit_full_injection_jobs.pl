#!/usr/bin/perl -w

my $SPARE = 0;
my $USERNAME = "amahmou2";
#my $USERNAME = "venktgr2";

open my $file, '<', "max_jobs.txt";
my $MAX_JOBS = <$file>;
close $file;
#my $MAX_JOBS = $firstline;
#my $MAX_JOBS=120;
print "Max Jobs = $MAX_JOBS\n";


$num_args = $#ARGV+1;
if($num_args != 1) {
	print "Usage: ./submit_full_injection_jobs.pl <simics_file_list>\n";
	die;
}
open(JOB_LIST, "<$ARGV[0]");
$go_to_sleep = 1;

$job_list_length = `wc -l $ARGV[0]`;
if ($job_list_length =~/^(\d+)/ ) {
    $num_jobs = $1;
}
print "Total number of jobs: $num_jobs\n";

my $job_sub_file = "$ENV{RELYZER_SHARED}/fault_list_output/condor_scripts/all_jobs_$num_jobs.condor";

if($num_jobs == 0) {
	exit(1);
}

my $num_submitted_jobs = 0;

while ($go_to_sleep == 1) {

	my $empty_slots = 0;
	
	# busy_nodes = # jobs that are running
	# total_nodes = # nodes that are available
	# empty_slots = # nodes that are unclaimed and idle
	# my_nodes = # of my jobs that are running.
	
	# num jobs on condor, not running
    my $num_running_jobs = `condor_q amahmou2 | tail -1 | cut -d' ' -f1`;
    #my $num_running_jobs = `condor_q venktgr2 | tail -1 | cut -d' ' -f1`;
	$num_running_jobs = int($num_running_jobs);

    # dynamically update number of condor jobs from file
    open my $file, '<', "max_jobs.txt";
    $MAX_JOBS = <$file>;
    close $file;


	if ($num_running_jobs < $MAX_JOBS) {
    	$empty_slots = $MAX_JOBS-$num_running_jobs;
	} else {
		$empty_slots = 0;
	}

	if (($empty_slots>0) && (!eof(JOB_LIST))) {

		open(CONDOR_FILE, ">$job_sub_file");

		print CONDOR_FILE "universe            = vanilla\n";
		print CONDOR_FILE "requirements        = ((target.memory * 2048) >= ImageSize) && ((Arch == \"X86_64\") || (Arch == \"INTEL\"))\n";
		print CONDOR_FILE "Executable          =  $ENV{RELYZER_SHARED}/scripts/injections_scripts/run_injection_job.pl\n";
        #	print CONDOR_FILE "Executable          =  /home/venktgr2/GEMS/simics/home/dynamic_relyzer/run_injection_job.pl\n";
		print CONDOR_FILE "getenv              = true\n";
		print CONDOR_FILE "notification = error\n\n";

		my $i = 0;
		for ($i=0; $i<$empty_slots; $i++) {

			my $num_per_job = 1; 
			my $job_args = "";
			while (<JOB_LIST>) { 
				my($line) = $_;
				chomp($line);
				#$line=~ s/\s+//g;
				if($line eq "") {
					#print "Found an empty line\n";
					next;
				}


				$num_submitted_jobs++;
				$num_per_job--;

				$job_args = "$line";

				if($num_per_job <= 0) {
					last; 
				}
			}
			if($job_args ne "") { 
				print CONDOR_FILE "arguments       = $job_args\n" ;
				print CONDOR_FILE "periodic_remove = (RemoteWallClockTime > 100*10*60)\n" ;
				print CONDOR_FILE "output          = /dev/null\n" ;
				print CONDOR_FILE "error           = /dev/null\n" ;
				print CONDOR_FILE "queue\n\n" ;

				#@words = split(/\//,$line);
				#print CONDOR_FILE "output          = condor_output/$words[2].output\n" ;
				#print CONDOR_FILE "error           = condor_output/$words[2].err\n" ;
				#print CONDOR_FILE "periodic_remove = (RemoteWallClockTime > 10*10*60*60)\n" ;
				# remove if mem exceeds 2 gig
				#print CONDOR_FILE "periodic_remove = (ImageSize > 2*1024*1024)\n" ;
			}

			if($num_per_job > 0) { # no more jobs to submit
				$go_to_sleep = 0;
				last;
			}
		}

		close(CONDOR_FILE);

		# print "submitting this file\n";
		# system "cat $job_sub_file";
		system "condor_submit $job_sub_file";

		printf "Progress : %.2f percent, Number of Submitted jobs : %d\n", ($num_submitted_jobs*100.0/$num_jobs, $num_submitted_jobs);
	}
	if ($go_to_sleep) {
		#$st = 10 + rand() * 100; # sleep time
		$st = 8;

		sleep ($st);
	}
}

close(JOB_LIST);

system "/shared/workspace/amahmou2/scripts/send_email.sh all_injections_in"
