APPROXILYZER
=========================================================

Approxilyzer is an open-source framework for instruction level approximation 
and resiliency software. Approxilyzer provides a systematic way to identify 
instructions that exhibit first-order approximation potential. It can also identify 
silent data corruption (SDC) causing instructions in the presence of single-bit errors. 
Approxilyzer employs static and dynamic analysis, in addition to heuristics, to reduce 
the run-time of finding Approximate instructions and SDC-causing instructions by 3-6x 
orders of magnitude.




APPROXILYZER REQUIREMENTS
=========================================================
1) WindRiver SIMICS 3.0.31 (requires WindRiver license)
2) Wisconsin Multifacet GEMS
3) A SPARC V9 machine, running OpenSolaris (TODO future versions will allow cross-compilation)



SETTING UP ENVIRONMENT
==========================================================
1) Download and untar GEMS into a local directory.  
    You can find this at http://research.cs.wisc.edu/gems/, or use our tarball at:
    https://rsim.cs.illinois.edu/approxilyzer/GEM_Approxilyzer.tar 
2) Append the following two lines to your .bashrc:
    GEMS=/full/path/to/src_GEMS
    export GEMS
3) Download SIMICS 3.0.31. Untar locally. 
    Instructions on how to obtain SIMICS can be found here: http://research.cs.wisc.edu/gems/GEMS-Ubuntu.pdf
4) Make a directory for SIMICS installation alongside $GEMS
    $ cd $GEMS
    $ cd ../
    $ mkdir simics
5) When prompted during the SIMICS installation, provide the absolute path
to the directory just created in Step 4
6) Edit your .bashrc with the following lines:
    SIMICS_INSTALL=/full/path/to/simics/simics-3.0.31
    export SIMICS_INSTALL
7) Follow the instructions from GEMS site for setting up SIMICS.
    http://research.cs.wisc.edu/gems/doc/gems-wiki/moin.cgi/Setup_for_Simics_3.0.X 
8) In common/Makefile.common, under the amd64-linux, use the following settings 
for various flags:
    CC = /usr/bin/g++34
    OPT_FLAGS=-m64 -march-opetron -fPIC
    LDFLAGS += -WI, -R/usr/lib64
    MODULE_LDFLAGS += -WI, -R/usr/lib64
9) Update the variable SIMICS_INCLUDE_ROOT in Makefile.common to point to the 
/src/include directory. Change the $(GEMS_ROOT) and fully elaborate the path name.
10) In $GEMS/scripts/prepare_simics_home.sh, change
    x86-linux -> amd64-linux
    ../sarek/simics -> ../../simics




11) Copy our provided Opal version, and then install Opal using:
    $ cd $GEMS/opal
    $ make clean
    $ make module DESTINATION=dynamic_relyzer
    (might have to manually make the directory "dynamic_relyzer" in $GEMS/simics/home/)
12) Install Ruby
    $ cd $GEMS/ruby
    $ make clean
    $ make module DESTINATION=dynamic_relyzer PROTOCOL=MOSI_SMP_bcast
    


APPROXILYZER SETUP 
==========================================================

Everything you need to run Approxilyzer is provied in the approxilyzer.sh script.

git clone this repo locally:
    $git clone git@github.com:ma3mool/Approxilyzer.git

To setup Approxilyzer:
    $./approxilyzer.sh -s prep

This will prepare your Approxilyzer directory structure, copy and install Emerald 
(a GEMS module), and allow you to use the rest of Approxilyzer. This only has to
be done once. If you run into issues, check that environmental issues are set up
properly.

Finally, add an additional environment path variable to your .bashrc
    RELYZER_SHARED = /absolute/path/to/cloned/environment


APPROXILYZER USE 
==========================================================

PREPARING ISO:
Approxilyzer requires application binaries to be mounted on an ISO for analysis.
The technique is streamlined once you have your application(s) on an ISO.

The required files in the ISO are:
    For each application:
        - a binary (app_name)
        - a disassembly file (app_name.dis)
        - a run script (run_script.sh)
Each application must be in its own seperate directory.

Each applications ISO also requires a corresponding ISO with the "golden output" of
an application's fault-free run. This ISO should have all outputs together at the 
top level, without subdirectories:
    For each application run script:
        - Golden output. (app_name.output)

When generating the ISO, you can use the following command in a terminal:
    $ mkisofs -RJ -o <filename>.iso <directory/with/required/files>

You can test the mount locally with:
    $ mount - loop <filename>.iso /mnt/point/here


We provide a template application ISO and golden ISO with the distribution loading 
some PARSEC and SPLASH-2 benchmarks. 



PREPARING SIMICS CHECKPOINTS:
1) Place both ISOs in the $RELYZER_SHARED/workloads/iso/ directory.
2) Copy the content of the golden ISO and place it in $RELYZER/workloads/golden_output/ 
directory.
3) Make a checkpoint for the whole iso:
    $ ./approxilyzer -I [iso_name]
4) Make a checkpoint for each application you want to run Approxilyzer on:
    $ ./approxilyzer -a [app_name] -c [associated run_script_name]



GENERATING A FAULT LIST:
1) Make a new directory for the application inside $RELYZER_SHARED/workloads/apps/
The naming convention should be [app_name]_[run_script_name]
2) Inside this new directory, place the binary and disassembly file.
3) Run Approxilyzer static analysis phase 1:
    $ ./approxilyzer -r prof -a [app_name]
4) You should be prompted to edit a newly generated file inside the apps directory.
Follow the instructions in the prompt. 
5) Rerun the application profiler:
    $ ./approxilyzer -r prof -a [app_name]
6) Once done, run the dynamic analysis phase:
    $ ./approxilyzer -r anlys -a [app_name]
7) In the config file, you can now make some optimizations that are highly recommended
8) Run error site generation:
    $ ./approxilyzer -r fault_gen -a [app_name]
9) Generate intermediate SIMICS checkpoints for improved performance during injection:
    $ ./approxilyzer -r int_ckpt -a [app_name]
10) You are now done! The fault list should be located inside $RELYZER_SHARED/results/


RUNNING RELYZER FAULT INJECTIONS:
1) Prepare fault injection directories:
    $ ./approxilyzer -i prep -a [app_name]
2) A list of fault injections will be available in $RELYZER_SHARED/results/simics_file_list. Run one injection experiment per line. These can be done in 
parallel.
    $ cd scripts/injections_scripts/
    $ ./run_injection_jobs.pl [injection]
Alternatively, you can use a parallel script which calls run_injection_jobs using condor
    $ ./submit_full_injection_jobs.pl
3) Once injections are done (this may take some time), collect the results and analyze:
    $ ./approxilyzer -i results -a [app_name]
4) Results should now be at $RELYZER_SHARED/results/injection_results/parsed_results/


RUNNING APPROXILYZER ANALYSIS:


