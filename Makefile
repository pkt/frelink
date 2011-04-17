obj-m := frelink.o

KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

all:    default frelink

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

frelink: frelink_app
	@mv frelink_app frelink

frelink_app: frelink_app.o

test: default frelink
	@./run_tests
	
install:
	@mkdir out
	@mv frelink frelink.ko out

clean:
	@$(RM) -rf frelink frelink_app *.ko *.o *.mod* \
		.*.cmd Module.symvers \
		.tmp_versions modules.order out
