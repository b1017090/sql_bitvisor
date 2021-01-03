.section .processes
.long prc_monitor_name, 0, prc_monitor_bin, 0
.quad prc_monitor_bin_end - prc_monitor_bin
.quad +0
.data
prc_monitor_name: .string "monitor"
prc_monitor_bin: .incbin "process/monitor.bin"
prc_monitor_bin_end:
