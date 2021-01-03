.section .processes
.long prc_echoctl_name, 0, prc_echoctl_bin, 0
.quad prc_echoctl_bin_end - prc_echoctl_bin
.quad +0
.data
prc_echoctl_name: .string "echoctl"
prc_echoctl_bin: .incbin "process/echoctl.bin"
prc_echoctl_bin_end:
