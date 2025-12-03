#include <immintrin.h>
#include "queue.h"
#include "parse.h"
#include "cpu.h"
#include "sequencer.h"

// This thread runs the parsing function async, by popping ParsingBuffer pointers from the parsing queue and
// passing it to parseMessage, then passing the pointers back to the freeQueue
void parserThread(std::shared_ptr<SPSCQ<ParsingBuffer*>> parseQueuePtr, std::shared_ptr<SPSCQ<ParsingBuffer*>> freeQueuePtr, int cpu_num) {
    // set CPU affinity and raise priority to ensure maximum CPU share
    pinToCpu(cpu_num);
    setPriority(98);

    ParsingBuffer *next;
    while(GlobalState::runParser.load(std::memory_order_relaxed)) {
        // If the queue has an element, pop it, parse it and return the buffer back to the
        // free pool so the network thread can copy another UDP payload into it
        if(parseQueuePtr->pop(next)) { 
            parseMessage(next->data, next->size);
            freeQueuePtr->push(next);
            continue;
        }
        _mm_pause();
    }
}

//
Architecture:             x86_64
  CPU op-mode(s):         32-bit, 64-bit
  Address sizes:          39 bits physical, 48 bits virtual
  Byte Order:             Little Endian
CPU(s):                   4
  On-line CPU(s) list:    0-3
Vendor ID:                GenuineIntel
  Model name:             Intel(R) Core(TM) i5-7300U CPU @ 2.60GHz
    CPU family:           6
    Model:                142
    Thread(s) per core:   2
    Core(s) per socket:   2
    Socket(s):            1
    Stepping:             9
    CPU(s) scaling MHz:   23%
    CPU max MHz:          3500.0000
    CPU min MHz:          400.0000
    BogoMIPS:             5399.81
    Flags:                fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge m
                          ca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 s
                          s ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc 
                          art arch_perfmon pebs bts rep_good nopl xtopology nons
                          top_tsc cpuid aperfmperf pni pclmulqdq dtes64 monitor 
                          ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm p
                          cid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_tim
                          er aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch
                           cpuid_fault epb pti ssbd ibrs ibpb stibp tpr_shadow f
                          lexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 a
                          vx2 smep bmi2 erms invpcid mpx rdseed adx smap clflush
                          opt intel_pt xsaveopt xsavec xgetbv1 xsaves dtherm ida
                           arat pln pts hwp hwp_notify hwp_act_window hwp_epp vn
                          mi md_clear flush_l1d arch_capabilities ibpb_exit_to_u
                          ser
Virtualization features:  
  Virtualization:         VT-x
Caches (sum of all):      
  L1d:                    64 KiB (2 instances)
  L1i:                    64 KiB (2 instances)
  L2:                     512 KiB (2 instances)
  L3:                     3 MiB (1 instance)
NUMA:                     
  NUMA node(s):           1
  NUMA node0 CPU(s):      0-3
Vulnerabilities:          
  Gather data sampling:   Mitigation; Microcode
  Itlb multihit:          KVM: Mitigation: VMX disabled
  L1tf:                   Mitigation; PTE Inversion; VMX conditional cache flush
                          es, SMT vulnerable
  Mds:                    Mitigation; Clear CPU buffers; SMT vulnerable
  Meltdown:               Mitigation; PTI
  Mmio stale data:        Mitigation; Clear CPU buffers; SMT vulnerable
  Reg file data sampling: Not affected
  Retbleed:               Mitigation; IBRS
  Spec rstack overflow:   Not affected
  Spec store bypass:      Mitigation; Speculative Store Bypass disabled via prct
                          l
  Spectre v1:             Mitigation; usercopy/swapgs barriers and __user pointe
                          r sanitization
  Spectre v2:             Mitigation; IBRS; IBPB conditional; STIBP conditional;
                           RSB filling; PBRSB-eIBRS Not affected; BHI Not affect
                          ed
  Srbds:                  Mitigation; Microcode
  Tsx async abort:        Mitigation; TSX disabled
  Vmscape:                Mitigation; IBPB before exit to userspace
roarkmenezes@dredge:~$ 

