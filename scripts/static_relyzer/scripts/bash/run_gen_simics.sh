# Blackscholes
time perl perl/gen_new_simics_files.pl blackscholes_simmedium merged_fault_list/output/blackscholes_blackscholes_fault_list.txt ~/GEMS/simics//home/dynamic_relyzer/optimized_main_logs/blackscholes_simmedium-classes.output

# FFT
#time perl perl/gen_simics_files.pl fft_small_merged merged_fault_list/output/fft_fft_fault_list.txt ~/GEMS/simics/home/dynamic_relyzer/logs/fft_small_merged_samples.txt
time perl perl/gen_new_simics_files.pl fft_small merged_fault_list/output/fft_fft_fault_list.txt ~/GEMS/simics/home/dynamic_relyzer/optimized_main_logs/fft_small-classes.output

# Swaptions
time perl perl/gen_new_simics_files.pl swaptions_simsmall merged_fault_list/output/swaptions_swaptions_fault_list.txt ~/GEMS/simics/home/dynamic_relyzer/optimized_main_logs/swaptions_simsmall-classes.output

# LU
time perl perl/gen_new_simics_files.pl lu_reduced merged_fault_list/output/lu_lu_fault_list.txt ~/GEMS/simics/home/dynamic_relyzer/optimized_main_logs/lu_reduced-classes.output

# libquantum 
time perl perl/gen_new_simics_files.pl libquantum_test merged_fault_list/output/libquantum_libquantum_fault_list.txt ~/GEMS/simics/home/dynamic_relyzer/optimized_main_logs/libquantum_test-classes.output

# omnetpp
time perl perl/gen_new_simics_files.pl omnetpp_test merged_fault_list/output/omnetpp_omnetpp_fault_list.txt ~/GEMS/simics/home/dynamic_relyzer/optimized_main_logs/omnetpp_test-classes.output
