PennOS - Virtual-OS-for-Linux. Emulates a virtual FAT FileSystem on a single hostOS file with custom implementations of standard file system APIs. Contains a job scheduler using pthreads, bash-like shell interface with synchronous signaling, redirection, job control, terminal control, and a set of custom-implemented shell built-ins.

## File Structure:
```
.
├── Doxyfile
├── Makefile
├── README.md
├── doc
│   └── manual.pdf
└── src
    ├── fs
    │   ├── fat.c
    │   ├── fat.h
    │   ├── file.c
    │   └── file.h
    ├── include
    │   ├── macros.h
    │   ├── p_errno.c
    │   ├── p_errno.h
    │   ├── parsejob.h
    │   └── parsejob.o
    ├── pennfat
    │   ├── pennfat.c
    │   ├── pennfat.h
    │   ├── pennfathandler.c
    │   └── pennfathandler.h
    └── pennos
        ├── PCB.h
        ├── filedescriptor.c
        ├── filedescriptor.h
        ├── handlejob.c
        ├── handlejob.h
        ├── iter.c
        ├── iter.h
        ├── job.c
        ├── job.h
        ├── jobQueue.c
        ├── jobQueue.h
        ├── jobcontrol.c
        ├── jobcontrol.h
        ├── kernel.c
        ├── kernel.h
        ├── node.c
        ├── node.h
        ├── queue.c
        ├── queue.h
        ├── scheduler.c
        ├── scheduler.h
        ├── shell.c
        ├── shell.h
        ├── signal.h
        ├── token.c
        ├── token.h
        ├── user_level_funcs.c
        └── user_level_funcs.h
```
## Compile Instructions
Run ```make``` to compile both PennFAT and PennOS.
Alternatively, ```make pennfat``` and ```make pennos``` compiles their respective binaries.
```make clean``` deletes the binaries and all .o files created while compiling.

The binaries are compiled in the ```/bin/``` folder.

## Work Accomplished
### PennFAT: Standalone FAT Filesystem
The standalone filesystem is completely functional and has no memory leaks.

It supports all of the commands specified in the project specification document:

```
mkfs    FS_NAME BLOCKS_IN_FAT BLOCK_SIZE_CONFIG
mount   FS_NAME
umount
touch   FILE ...
mv      SOURCE DEST
rm      FILE ...
cat     FILE ... [ -w OUTPUT_FILE ]
cat     FILE ... [ -a OUTPUT_FILE ]
cat     -w OUTPUT_FILE
cat     -a OUTPUT_FILE
cp      [-h] SOURCE DEST
cp      SOURCE [-h] DEST
chmod
ls
```
Additionally, it supports the ```describe``` command which prints out additional information about the currently mounted file system.

### PennOS

PennOS is completely functional in terms of creating a kerner, scheduler, and the main shell process.

Additionally, the shell process supports job control as well as redirections. It supports all of the commands specified in the project specification document:

```
zombify
orphanify
man
bg [job_id]
fg [job_id]
jobs
logout
cat
sleep n
busy
ls
touch file ...
mv src dest
cp src dest
rm file ...
ps
```
Note that from the commands above the following:

```
zombify
orphanify
cat
sleep n
busy
touch file ...
mv src dest
cp src dest
rm file ...
chmod
head
ps
```

run as independent processes while the remaining ones are shell subroutines.

## Code Layout

At the base level, files are divided in four subdirectories of ```/src/```, ```/src/include/, /src/fs/, /src/pennfat/, /src/pennos```:

```
/src/include/   general files shared between multiple directories, e.g. macros.h
/src/fs/        files directly involved in filesystem implementation
/src/pennfat/   files that facilitate the PennFAT standalone program
/src/pennos/    files that facilitate PennOS
```

Some additional directories are:

```
/bin/ 			all the binary files generated
/log/log.txt 			the log file
```

## Other Comments
