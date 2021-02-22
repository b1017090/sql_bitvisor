.section .processes
.long prc_test_name, 0, prc_test_bin, 0
.quad prc_test_bin_end - prc_test_bin
.quad +0
.data
prc_test_name: .string "test"
prc_test_bin: .incbin "process/test.bin"
prc_test_bin_end:
