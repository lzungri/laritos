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

# Logger using the uart as output transport
log_uart:logger|transport=bytestream@uart0

# RTC component, using irq 34
rtc0:pl031|mmbase=0x09010000,maxfreq=1,intio=true,intc=@gic,irq=34,trigger=level_hi

# CPUs
cpu0:cortex_a15|id=0,intc=@gic,sched=@rrsched
cpu1:cortex_a15|id=1,intc=@gic,sched=@rrsched
cpu2:cortex_a15|id=2,intc=@gic,sched=@rrsched
cpu3:cortex_a15|id=3,intc=@gic,sched=@rrsched

# UART component using irq 33
uart0:pl011|baseaddr=0x09000000,blocking=true,intio=true,intc=@gic,irq=33,trigger=level_hi

# Input device using the uart as input transport
input_uart:inputdev|transport=bytestream@uart0

# Interrupt controller
gic:gicv2|distaddr=0x08000000,cpuaddr=0x08010000

# OS Ticker
ticker0:generic_ticker|vrtimer=@vrtimer0,ticks_per_sec=1

# Virtual timer component, we are currently using the rtc as a hrtimer until we implement a higher-res timer
vrtimer0:generic_vrtimer|hrtimer=@rtc0,low_power_timer=@rtc0

# Preemptive round robin scheduler
rrsched:preempt_rr|ticker=@ticker0

# Cooperative FIFO scheduler
cfifosched:coop_fifo