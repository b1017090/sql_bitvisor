.section .processes
.long prc_sqliteexample2_name, 0, prc_sqliteexample2_bin, 0
.quad prc_sqliteexample2_bin_end - prc_sqliteexample2_bin
.quad 16384+0
.data
prc_sqliteexample2_name: .string "sqliteexample2"
prc_sqliteexample2_bin: .incbin "process/sqlite/sqliteexample2.bin"
prc_sqliteexample2_bin_end:
