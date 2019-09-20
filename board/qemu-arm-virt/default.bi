uart0:pl011|baseaddr=0x09000000,blocking=true,intio=true,depends=gic,intc=gic,irq=33,trigger=level_hi
log_uart:logger|depends=uart0,transport=bytestream@uart0
input_uart:inputdev|depends=uart0,transport=bytestream@uart0
gic:gicv2|distaddr=0x08000000,cpuaddr=0x08010000