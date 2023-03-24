# Lab1

### Linux内核裁剪过程 

* first `12.4MB`
***
* second `10.1MB`
  * delete `Kernel hacking` 
***
* third `7.2MB`
  * delete `Networking support`
***
* fourth `6.4MB`
  * change `General setup_Compiler optimization level (Optimize for size (-Os))`
***
* fifth `6.1MB`
  * delete `Security options`
  * delete `Library routines`
  * delete `Cryptographic API`
  * delete `Device Drivers_USB support`
***
* sixth `5.0MB`
  * change `General setup_Kernel compression mode (LZMA)`
***
* seventh `4.8MB`
  * delete `File systems_Quota support`
  * delete `File systems_The Extended 4 (ext4) filesystem`
***
* eighth `4.5MB`
  * delete `General setup_Control Group support`
  * delete `Processor type and features_Enable 5-level page tables support`
  * delete `General setup_Compile the kernel with warnings as errors`
  * delete `General setup_Automatically append version information to the version string`
  * delete `General setup_Auditing support`
  * delete `General setup_Profiling support`
  * delete `Processor type and features_Check for low memory corruption`
  * delete `Processor type and features_Indirect Branch Tracking`
  * delete `Processor type and features_Memory Protection Keys`
  * delete `General architecture-dependent options_Kprobes`
  * delete `General architecture-dependent options_Stack Protector buffer overflow detection`
  * delete `General architecture-dependent options_Provide system calls for 32-bit time_t`
  * delete `Processor type and features_EFI runtime service support`
***
* ninth `3.8MB`
  * delete `Processor type and features_Symmetric multi-processing support`
  * delete `Processor type and features_Single-depth WCHAN output`
  * delete `Processor type and features_Linux guest support`
  * delete `Processor type and features_/dev/cpu/*/msr - Model-specific register support`
  * delete `Processor type and features_/dev/cpu/*/cpuid - CPU information support`
  * delete `Processor type and features_Machine Check / overheating reporting`
  * delete `Processor type and features_Build a relocatable kernel`
  * delete `Processor type and features_kexec system call` 
  * delete `Processor type and features_kernel crash dumps`
  * change `Processor type and features_Processor family (Intel P4 / older Netburst based Xeon)`
  * delete `Processor type and features_Reroute for broken boot IRQs`
  * delete `Processor type and features_IOPERM and IOPL Emulation`
  * delete `Processor type and features_CPU microcode loading support`
  * delete `Device Drivers_Watchdog Timer Support`
  * delete `Device Drivers_Sound card support`
  * delete `Device Drivers_Mailbox Hardware Support`
  * delete `Device Drivers_Multiple devices driver support (RAID and LVM)`


> 实际上上面是我第二次裁剪Linux内核的过程，第一次由于没有考虑到对后续实验的影响，为了裁剪去掉了很多可能有影响的选项。

* 对`Linux`内核大小/编译时间有影响的选项
  * `Kernel hacking` 
  * `Networking support`
  * `General setup_Compiler optimization level (Optimize for size (-Os))`
  * `General setup_Kernel compression mode (LZMA)`

