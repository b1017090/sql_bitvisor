.section .processes
.long prc_sqliteexample_name, 0, prc_sqliteexample_bin, 0
.quad prc_sqliteexample_bin_end - prc_sqliteexample_bin
.quad 16384+0
.data
prc_sqliteexample_name: .string "sqliteexample"
prc_sqliteexample_bin: .incbin "process/sqlite/sqliteexample.bin"
prc_sqliteexample_bin_end:
