#define CONFIG_IN_OPAL

#define EMERALD_TIMING

#ifdef EMERALD_TIMING
#define OBSERVE_MODEL TIMING_MODEL_INTERFACE
#define MODEL_STRING "timing_model"
#else  // EMERALD_TIMING
#define OBSERVE_MODEL SNOOP_MEMORY_INTERFACE
#define MODEL_STRING "snoop_device"
#endif // EMERALD_TIMING

#define CHEETAH_START 0x0000040000400000ULL
#define CHEETAH_END (0x0000040000400000ULL + 0x48)

#define NUM_MEM_SPACE 1

#define SERVER_WORKLOADS 0

#if SERVER_WORKLOADS

#define CPU_NAME "server_cpu0"
#define CLIENT_CPU_NAME "client_cpu0"
#define PHYS_IO_NAME "server_phys_io0"
#define PHYS_MEM_NAME "server_phys_mem0"

#else // SERVER_WORKLOADS

#define CPU_NAME "cpu0"
#define PHYS_IO_NAME "phys_io0"
#define PHYS_MEM_NAME "phys_mem0"

#endif //SERVER_WORKLOADS
