uart0:pl011|baseaddr=0x09000000,blocking=true,intio=true,intc=@gic,irq=33,trigger=level_hi
log_uart:logger|transport=bytestream@uart0
input_uart:inputdev|transport=bytestream@uart0
gic:gicv2|distaddr=0x08000000,cpuaddr=0x08010000
rtc0:pl031|mmbase=0x09010000,res=1000000000,intio=true,intc=@gic,irq=34,trigger=level_hi