CFLAGS += -Iprocess/lib

CONSTANTS-1 += -DSQLITE_OMIT_LOAD_EXTENSION
CONSTANTS-1 += -DSQLITE_THREADSAFE=0
CONSTANTS-1 += -DSQLITE_OMIT_LOCALTIME
CONSTANTS-1 += -DSQLITE_OMIT_FLOATING_POINT
CONSTANTS-1 += -DSQLITE_OS_OTHER
CONSTANTS-1 += -DSQLITE_ENABLE_MEMSYS5
CONSTANTS-1 += -DSQLITE_ZERO_MALLOC
CONSTANTS-1 += -DSQLITE_OMIT_AUTOINIT

sqliteexample-objs = sqliteexample.o vvfs.o sqlite3.o
sqliteexample-libs = process/lib/$(outa)
sqliteexample-stacksize = 16384

sqliteexample2-objs = sqliteexample2.o vvfs.o sqlite3.o
sqliteexample2-libs = process/lib/$(outa)
sqliteexample2-stacksize = 16384


bins-1 += sqliteexample
bins-1 += sqliteexample2


# The text size of sqlite3.o will be more than 500KB.  If another
# program is added and the sqliteexample is not needed, use = like
# bins-1=program to avoid linking the sqliteexample and reduce the
# binary size.
