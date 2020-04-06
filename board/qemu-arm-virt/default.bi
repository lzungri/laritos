# qemu-arm-virt board information
# -------------------------------

# Syntax:
#    component_id1:driver1|attr1=attr1value,attrN=attrNvalue
#    component_id2:driver2|attr1=attr1value
#    component_id3:driver3
#
# To reference another component from an attribute, use @ followed by the component id.
# That will automatically create a dependency with the referenced component, forcing it
# to be loaded before the current one.
# For example:
#    comp1:driver1
#    comp2:driver2|attr1=@comp1
#
# Boolean attributes can take one of the following values: n, false, 0, y, true, 1

# CPUs
# PMU unit sends a PPI #23 to cpu #0 to notify about cycle counter overflow
# QEMU virtualizes arm cpus running at 1GHz
# Load it first, since it is probably used by the other components
cpu0:cortex_a15|id=0,freq=1000000000,intc=@gic,sched=@rrsched,default=y,pmu_irq=23,pmu_trigger=level_hi
cpu1:cortex_a15|id=1,freq=1000000000,intc=@gic,sched=@rrsched,pmu_irq=54,pmu_trigger=level_hi
cpu2:cortex_a15|id=2,freq=1000000000,intc=@gic,sched=@rrsched,pmu_irq=85,pmu_trigger=level_hi
cpu3:cortex_a15|id=3,freq=1000000000,intc=@gic,sched=@rrsched,pmu_irq=116,pmu_trigger=level_hi

# Virtual timer component
vrtimer0:generic_vrtimer|hrtimer=@hrtimer0,low_power_timer=@rtc0

# Logger using the uart as output transport
log_uart:generic_logger|transport=bytestream@uart0

# RTC component, using irq 34
rtc0:pl031|mmbase=0x09010000,maxfreq=1,intio=true,intc=@gic,irq=34,trigger=level_hi

# ARM v7 generic timer. One timer per processor. Each timer signals its interrupts to
# the GIC via a private peripheral interrupt (PPI). IRQ 30 is for cpu #0
hrtimer0:armv7_generic_timer|maxfreq=62500000,intio=true,intc=@gic,irq=30,trigger=level_hi

# UART component using irq 33
uart0:pl011|baseaddr=0x09000000,blocking=true,intio=true,intc=@gic,irq=33,trigger=level_hi

# Input device using the uart as input transport
input_uart:gen_input|transport=bytestream@uart0

# Interrupt controller
gic:gicv2|distaddr=0x08000000,cpuaddr=0x08010000

# OS Ticker
ticker0:generic_ticker|vrtimer=@vrtimer0,ticks_per_sec=100

# Preemptive round robin scheduler
rrsched:preempt_rr|ticker=@ticker0

# Cooperative FIFO scheduler
cfifosched:coop_fifo

# The QEMU arm virtual board maps two flash memories (see qemu/hw/arm/virt.c#virt_flash_map()):
#   pflash0: base=0x00000000 size=0x04000000 (here we put the kernel image)
#   pflash1: base=0x04000000 size=0x04000000 (here we put the system image)
flash0:flash_cfi|baseaddr=0x00000000,sectorsize=262144,nsectors=256,default=y
flash1:flash_cfi|baseaddr=0x04000000,sectorsize=262144,nsectors=256

# Multimedia card interface (pl181)
mci0:pl181|baseaddr=0x09090000,intc=@gic,irq=42,trigger=level_hi

# The system image is the first real filesystem mounted by the OS, thus it must be
# statically mounted during boot. Once it is done, we could dinamically mount other
# filesystems by reading from /sys/conf/mount.conf
system.img:static_fs_mount|mntpoint=/sys,dev=@flash1,type=ext2,mode=r
