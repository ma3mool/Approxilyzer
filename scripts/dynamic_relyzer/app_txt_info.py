# for unoptimized code
#text_start = {}
#text_start['swaptions_simsmall_merged'] = 0x100001ac0;
#text_start['blackscholes_simmedium_merged'] = 0x1000012e0;
#text_start['lu_small_merged'] = 0x100001680;
#text_start['lu_reduced'] = 0x100001680;
#text_start['fft_small_merged'] = 0x100001980;
#text_end = {}
#text_end['swaptions_simsmall_merged'] = 0x100008b80;
#text_end['blackscholes_simmedium_merged'] = 0x100002e80;
#text_end['lu_small_merged'] = 0x10000912c;
#text_end['lu_reduced'] = 0x10000912c;
#text_end['fft_small_merged'] = 0x1000086d4;
# actual_app_start = {}
# actual_app_start['blackscholes_simsmall_me'] = 0x100001cb4

# for optimized code
# The following are for optimized apps - with SUN cc -xO2
# text_start = {}
# text_start['swaptions_simsmall'] = 0x100001ac0
# text_start['blackscholes_simmedium'] = 0x1000012e0
# text_start['blackscholes_simlarge'] = 0x1000012e0
# text_start['lu_reduced'] = 0x100001680
# text_start['fft_small'] = 0x100001980
# text_start['gcc_test'] = 0x1000629c0
# text_start['mcf_test'] = 0x100001540
# text_start['hmmer_test'] = 0x100009cc0
# text_start['libquantum_test'] = 0x100003120
# text_start['sjeng_test'] = 0x1000062c0
# text_start['bzip2_test'] = 0x100002840
# text_start['omnetpp_test'] = 0x1000438e0
# text_end = {}
# text_end['swaptions_simsmall'] = 0x100004fc0
# text_end['blackscholes_simmedium'] = 0x100002050
# text_end['blackscholes_simlarge'] = 0x100002050
# text_end['lu_reduced'] = 0x100005bfc
# text_end['fft_small'] = 0x100004b5c
# text_end['gcc_test'] = 0x100404b7c
# text_end['mcf_test'] = 0x1000042dc
# text_end['hmmer_test'] = 0x10005d6ec
# text_end['libquantum_test'] = 0x10000e88c
# text_end['sjeng_test'] = 0x10002df1c
# text_end['bzip2_test'] = 0x100016b7c
# text_end['omnetpp_test'] = 0x1000df760
#
# The following are for optimized apps - with SUN cc -xO2
# actual_app_start = {}
# actual_app_start['swaptions_simsmall'] = 0x100004cb0
# actual_app_start['blackscholes_simlarge'] = 0x100001cb4
# actual_app_start['lu_reduced'] = 0x100003bd4
# actual_app_start['fft_small'] = 0x100002f14
# actual_app_start['gcc_test'] = 0x1003b0b40
# actual_app_start['mcf_test'] = 0x1000018e8
# actual_app_start['omnetpp_test'] = 0x1000900f0
# actual_app_end = {}
# actual_app_end['swaptions_simsmall'] = 0x100004d00
# actual_app_end['blackscholes_simlarge'] = 0x100001cec
# actual_app_end['lu_reduced'] = 0x100002a68
# actual_app_end['fft_small'] = 0x1000024a8
# actual_app_end['gcc_test'] = 0x1003b0c68
# actual_app_end['mcf_test'] = 0x1000019d4
# actual_app_end['omnetpp_test'] = 0x1000900f8

OPT_LEVEL = ""
text_start = {}
text_end = {}
actual_app_start = {}
actual_app_end = {}
actual_app_length = {}

def init_txt_info():

  if OPT_LEVEL == "unopt" :
    text_start['blackscholes_simlarge'] =	0x1000012e0
    text_start['swaptions_simsmall'] = 	0x100001ae0
    text_start['fft_small'] = 		0x100001980
    text_start['lu_reduced'] = 		0x100001680
    text_start['libquantum_test'] = 	0x100002ee0

    text_end['blackscholes_simlarge'] =	0x100002e68
    text_end['swaptions_simsmall'] = 	0x100008ba0
    text_end['fft_small'] = 		0x1000086d4
    text_end['lu_reduced'] = 		0x100009124
    text_end['libquantum_test'] = 	0x100014864

    actual_app_start['blackscholes_simlarge'] =   0x100001b90
    actual_app_start['swaptions_simsmall'] = 	0x100007118
    actual_app_start['fft_small'] = 		0x1000045d0
    actual_app_start['lu_reduced'] = 		0x1000041e0
    actual_app_start['libquantum_test'] = 	0x100013bf0

    actual_app_end['blackscholes_simlarge'] =	0x1000029d0
    actual_app_end['swaptions_simsmall'] = 	0x1000086ec
    actual_app_end['fft_small'] = 		0x1000033ac
    actual_app_end['lu_reduced'] = 		0x10000327c
    actual_app_end['libquantum_test'] = 		0x1000144b0

  # The following are for fully optimized apps - with SUN cc -fast
  if OPT_LEVEL == "fully_optimized" :

    # abdul fully optimized  (copied over from below, but checked)

    text_start['blackscholes_opt_abdul']=	0x1000012e0
    text_start['fft_opt_abdul']         =	0x100001980
    #text_start['fluidanimate_opt_abdul']=	0x100001d00 # ???
    text_start['fluidanimate_opt_abdul']=	0x11918 # ???
    text_start['gcc_opt_abdul']         =	0x1000629c0
    text_start['libquantum_opt_abdul']  =	0x100003120
    text_start['lu_opt_abdul']          =	0x100001680
    text_start['mcf_opt_abdul']         =	0x100001540
    text_start['ocean_opt_abdul']       =	0x100002320
    text_start['streamcluster_opt_abdul']=	0x100001da0
    text_start['swaptions_opt_abdul']   =	0x100001c80
    text_start['water_opt_abdul']       =	0x100002b00

    text_end['blackscholes_opt_abdul']  =	0x100003a78
    text_end['fft_opt_abdul']           =	0x100005e04
    #text_end['fluidanimate_opt_abdul']  =	0x1000075c0 # ???
    text_end['fluidanimate_opt_abdul']  =	0x153a4 # ???
    text_end['gcc_opt_abdul']           =	0x100404b7c
    text_end['libquantum_opt_abdul']    =	0x10000e88c
    text_end['lu_opt_abdul']            =   0x1000061dc
    text_end['mcf_opt_abdul']           =	0x1000042dc
    text_end['ocean_opt_abdul']         =	0x10002bc4c
    text_end['streamcluster_opt_abdul'] =	0x10000be80
    text_end['swaptions_opt_abdul']     =	0x100008060
    text_end['water_opt_abdul']         =	0x10000cd6c

    actual_app_start['blackscholes_opt_abdul']  =	0x100002880
    actual_app_start['fft_opt_abdul']           =	0x1000023d8
    #actual_app_start['fluidanimate_opt_abdul']  =	0x100005edc  # old fluidanimate_simsmall 0x14ff4
    actual_app_start['fluidanimate_opt_abdul']  =	0x14ff4 # old fluidanimate_simsmall 0x14ff4
    actual_app_start['gcc_opt_abdul']           =	0x1003b7c4c
    actual_app_start['libquantum_opt_abdul']    =	0x10000c10c
    actual_app_start['lu_opt_abdul']            =	0x1000029b4
    actual_app_start['mcf_opt_abdul']           =	0x1000018e8
    actual_app_start['ocean_opt_abdul']         =	0x10000b0f4
    actual_app_start['streamcluster_opt_abdul'] =	0x10000ae4c
    actual_app_start['swaptions_opt_abdul']     =	0x100007354
    actual_app_start['water_opt_abdul']         =	0x100009cac

    actual_app_end['blackscholes_opt_abdul']    =	0x10000324c
    actual_app_end['fft_opt_abdul']             =	0x1000023e0
    #actual_app_end['fluidanimate_opt_abdul']    =	0x100006b40  # old fluidanimate_simsmall 0x15014
    actual_app_end['fluidanimate_opt_abdul']    =	0x15014 # old fluidanimate_simsmall 0x15014
    actual_app_end['gcc_opt_abdul']             =	0x1003b7c54
    actual_app_end['libquantum_opt_abdul']      =	0x10000c3d4
    actual_app_end['lu_opt_abdul']              =   0x1000029bc
    actual_app_end['mcf_opt_abdul']             =	0x1000019dc
    actual_app_end['ocean_opt_abdul']           =	0x10000b0fc
    actual_app_end['streamcluster_opt_abdul']   =	0x10000b410
    actual_app_end['swaptions_opt_abdul']       =	0x100007400
    actual_app_end['water_opt_abdul']           =	0x100009cb4

    actual_app_length['blackscholes_opt_abdul'] =	22308072
    actual_app_length['fft_opt_abdul']          =	548003357
    #actual_app_length['fluidanimate_opt_abdul'] =	611359873
    actual_app_length['fluidanimate_opt_abdul'] =   3056791689
    actual_app_length['gcc_opt_abdul']          =	4570242655
    actual_app_length['libquantum_opt_abdul']   =	235408872
    actual_app_length['lu_opt_abdul']           =	402775160
    actual_app_length['mcf_opt_abdul']          =	3800525493
    actual_app_length['ocean_opt_abdul']        =   358076803
    actual_app_length['streamcluster_opt_abdul'] =	1435369269
    actual_app_length['swaptions_opt_abdul']    =	922226111
    actual_app_length['water_opt_abdul']        =	504349963


    # abdul
    text_start['blackscholes_abdul'] =	0x1000012e0
    text_start['swaptions_abdul'] =	0x100001c80

    text_end['blackscholes_abdul'] =	0x100003a78
    text_end['swaptions_abdul'] =	0x100008060

    actual_app_start['blackscholes_abdul'] =	0x100002880
    actual_app_start['swaptions_abdul'] =	0x100007354

    actual_app_end['blackscholes_abdul'] =	0x10000324c
    actual_app_end['swaptions_abdul'] =		0x100007400

    actual_app_length['blackscholes_abdul'] =	22308072
    actual_app_length['swaptions_abdul'] =	922226111


    #abdulrahman new apps and runs

    #blackscholes_input_00000
    text_start['blackscholes_input_run_00000'] = 0x1000012e0
    text_end['blackscholes_input_run_00000'] =	0x100003a78
    actual_app_start['blackscholes_input_run_00000'] =	0x100002880
    actual_app_end['blackscholes_input_run_00000'] =	0x10000324c
    actual_app_length['blackscholes_input_run_00000'] =	413
    #blackscholes_input_00001
    text_start['blackscholes_input_run_00001'] = 0x1000012e0
    text_end['blackscholes_input_run_00001'] =	0x100003a78
    actual_app_start['blackscholes_input_run_00001'] =	0x100002880
    actual_app_end['blackscholes_input_run_00001'] =	0x10000324c
    actual_app_length['blackscholes_input_run_00001'] =	416
    #blackscholes_input_00002
    text_start['blackscholes_input_run_00002'] = 0x1000012e0
    text_end['blackscholes_input_run_00002'] =	0x100003a78
    actual_app_start['blackscholes_input_run_00002'] =	0x100002880
    actual_app_end['blackscholes_input_run_00002'] =	0x10000324c
    actual_app_length['blackscholes_input_run_00002'] =	377
    #blackscholes_input_00003
    text_start['blackscholes_input_run_00003'] = 0x1000012e0
    text_end['blackscholes_input_run_00003'] =	0x100003a78
    actual_app_start['blackscholes_input_run_00003'] =	0x100002880
    actual_app_end['blackscholes_input_run_00003'] =	0x10000324c
    actual_app_length['blackscholes_input_run_00003'] =	417
    #blackscholes_input_00004
    text_start['blackscholes_input_run_00004'] = 0x1000012e0
    text_end['blackscholes_input_run_00004'] =	0x100003a78
    actual_app_start['blackscholes_input_run_00004'] =	0x100002880
    actual_app_end['blackscholes_input_run_00004'] =	0x10000324c
    actual_app_length['blackscholes_input_run_00004'] = 377






    #siva
    text_start['blackscholes_simlarge'] =	0x1000012e0
    text_start['blackscholes_simlarge_abdul'] =	0x1000012e0
    text_start['fluidanimate_simsmall'] =	0x100001d00
    text_start['streamcluster_simsmall'] =	0x100001da0
    text_start['swaptions_simsmall'] =	0x100001c80

    text_start['fft_small'] =		0x100001980
    text_start['lu_reduced'] =		0x100001680
    text_start['lu_64_reduced'] =		0x100001680
    text_start['ocean_small'] =		0x100002320
    text_start['water_small'] =		0x100002b00

    text_start['gcc_test'] =		0x1000629c0
    text_start['mcf_test'] =		0x100001540
    text_start['libquantum_test'] =		0x100003120
    text_start['omnetpp_test'] =		0x1000438e0

    text_end['blackscholes_simlarge'] =	0x100003a78
    text_end['blackscholes_simlarge_abdul'] =	0x100003a78
    text_end['fluidanimate_simsmall'] =	0x1000075c0
    text_end['streamcluster_simsmall'] =	0x10000be80
    text_end['swaptions_simsmall'] =	0x100008060

    text_end['fft_small'] =			0x100005e04
    text_end['lu_reduced'] =		0x1000061dc
    text_end['lu_64_reduced'] =		0x1000061dc
    text_end['ocean_small'] =		0x10002bc4c
    text_end['water_small'] =		0x10000cd6c

    text_end['gcc_test'] =			0x100404b7c
    text_end['mcf_test'] =			0x1000042dc
    text_end['libquantum_test'] =		0x10000e88c
    text_end['omnetpp_test'] =		0x1000df760

    # parsec
    actual_app_start['blackscholes_simlarge'] =	0x100002880
    actual_app_start['blackscholes_simlarge_abdul'] =	0x100002880
    actual_app_start['fluidanimate_simsmall'] =	0x100005edc
    # old fluidanimate_simsmall 0x14ff4
    actual_app_start['streamcluster_simsmall'] =	0x10000ae4c
    actual_app_start['swaptions_simsmall'] =	0x100007354

    actual_app_end['blackscholes_simlarge'] =	0x10000324c
    actual_app_end['blackscholes_simlarge_abdul'] =	0x10000324c
    actual_app_end['fluidanimate_simsmall'] =	0x100006b40
    # old fluidanimate_simsmall 0x15014
    actual_app_end['streamcluster_simsmall'] =	0x10000b410
    actual_app_end['swaptions_simsmall'] =		0x100007400

    # splash2
    actual_app_start['fft_small'] =		0x1000023d8
    actual_app_start['lu_reduced'] =	0x1000029b4
    actual_app_start['lu_64_reduced'] =	0x1000029b4
    actual_app_start['ocean_small'] =	0x10000b0f4
    actual_app_start['water_small'] =	0x100009cac

    actual_app_end['fft_small'] =		0x1000023e0
    actual_app_end['lu_reduced'] =	        0x1000029bc
    actual_app_end['lu_64_reduced'] =	        0x1000029bc
    actual_app_end['ocean_small'] =	        0x10000b0fc
    actual_app_end['water_small'] =	        0x100009cb4

    #spec 2006
    actual_app_start['gcc_test'] =		0x1003b7c4c
    actual_app_start['mcf_test'] =		0x1000018e8
    actual_app_start['libquantum_test'] =	0x10000c10c
    actual_app_start['omnetpp_test'] =	0x1000900e0

    actual_app_end['gcc_test'] =		0x1003b7c54
    actual_app_end['mcf_test'] =		0x1000019dc
    actual_app_end['libquantum_test'] =	0x10000c3d4
    actual_app_end['omnetpp_test'] =	0x100090114

    # parsec
    actual_app_length['blackscholes_simlarge'] =	22308074
    actual_app_length['blackscholes_simlarge_abdul'] =	22308072
    actual_app_length['fluidanimate_simsmall'] =	611359873
    actual_app_length['streamcluster_simsmall'] =	1435369269
    actual_app_length['swaptions_simsmall'] =	922226111
    actual_app_length['swaptions_simsmall_large'] =	11000000000

    # splash2
    actual_app_length['fft_small'] =	        548003357
    actual_app_length['lu_reduced'] =	        402775160
    actual_app_length['lu_64_reduced'] =	        402775160
    actual_app_length['ocean_small'] =              358076803
    actual_app_length['water_small'] =	        504349963

    #spec 2006
    actual_app_length['gcc_test'] =		        4570242655
    actual_app_length['mcf_test'] =		        3800525493
    actual_app_length['libquantum_test'] =	        235408872
    actual_app_length['omnetpp_test'] =	        1345151052

# pc_list_size = {}
# pc_list_size['swaptions_simsmall_merged'] = swaptions_brpc_list.swaptions_br_pc_list_size;
# pc_list_size['blackscholes_simmedium_merged'] = blackscholes_brpc_list.blackscholes_br_pc_list_size;
# pc_list_size['lu_small_merged'] = lu_brpc_list.lu_br_pc_list_size;
# pc_list_size['lu_reduced'] = lu_brpc_list.lu_br_pc_list_size;
# pc_list_size['fft_small_merged'] = fft_brpc_list.fft_br_pc_list_size;
#
# pc_list= {}
# pc_list['swaptions_simsmall_merged'] = swaptions_brpc_list.swaptions_br_pc;
# pc_list['blackscholes_simmedium_merged'] = blackscholes_brpc_list.blackscholes_br_pc;
# pc_list['lu_small_merged'] = lu_brpc_list.lu_br_pc;
# pc_list['lu_reduced'] = lu_brpc_list.lu_br_pc;
# pc_list['fft_small_merged'] = fft_brpc_list.fft_br_pc;
#
# cycle_pc_list= {}
# cycle_pc_list['swaptions_simsmall_merged'] = brk_list.list_pc_swaptions;
# cycle_pc_list['blackscholes_simmedium_merged'] = brk_list.list_pc_blackscholes;
# cycle_pc_list['lu_small_merged'] = brk_list.list_pc_lu;
# cycle_pc_list['lu_reduced'] = brk_list.list_pc_lu;
# cycle_pc_list['fft_small_merged'] = brk_list.list_pc_fft;
#
# cycle_sample_size= {}
# cycle_sample_size['swaptions_simsmall_merged'] = brk_list.list_sample_size_swaptions;
# cycle_sample_size['blackscholes_simmedium_merged'] = brk_list.list_sample_size_blackscholes;
# cycle_sample_size['lu_small_merged'] = brk_list.list_sample_size_lu;
# cycle_sample_size['lu_reduced'] = brk_list.list_sample_size_lu;
# cycle_sample_size['fft_small_merged'] = brk_list.list_sample_size_fft;
#
# cycle_population= {}
# cycle_population['swaptions_simsmall_merged'] = brk_list.list_population_swaptions;
# cycle_population['blackscholes_simmedium_merged'] = brk_list.list_population_blackscholes;
# cycle_population['lu_small_merged'] = brk_list.list_population_lu;
# cycle_population['lu_reduced'] = brk_list.list_population_lu;
# cycle_population['fft_small_merged'] = brk_list.list_population_fft;
#
