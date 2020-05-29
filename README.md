## PinK
```
This is a target on making high-speed in-storage Key-value Store with bounded tails
PinK is based on special device setup as described in Lightstore (ASPLOS'19).

It also has a simple simulation code for those who don't have a special device. 
To building it, Fix "TARGET_LOWER=bdbm_drv" in Makefile to "TARGET_LOWER=posix_memory"
```
## How to run
### Setting
```
PinK$ vim ./include/settings.h
-> set GIGAUNIT variable as device size (e.g. #define GIGAUNIT 32L for 32GB test)

PinK$ vim ./interface/main.c
-> add benchmarks what you want (e.g. bench_add(SEQSET,0,RANGE,RANGE); for sequential write bench)
```

### Make new main file
```
1. copy ./interface/mainfiles/default_main.c [your_main_file]
2. edit the your main file
3. edit Makefile
original Makefile:131
driver: ./interface/mainfiles/default_main.c libdriver.a
	$(CC) $(CFLAGS) -o $@ $^ $(ARCH) $(LIBS)

edited
driver: [your_main_file] libdriver.a
	$(CC) $(CFLAGS) -o $@ $^ $(ARCH) $(LIBS)
```

### Makefile
```
PinK$ vim Makefile
-> You can select module for each layer to operate with.

[Example]
TARGET_INF=interface
TARGET_ALGO=Lsmtree           
TARGET_LOWER=posix_memory   # memory(RAM Drive)
-> Make with interface as Interface module
   Lsmtree as Algorithm module
   posix_memory as Lower module

* Directory name of targets must exist on each layer's directory
  (e.g. algorithm/Lsmtree/ lower/posix_memory/ )
```

### Run
```
PinK$ make driver
Pink$ ./driver
```

