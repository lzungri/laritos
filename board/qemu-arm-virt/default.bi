uart0:pl011|baseaddr=0x09000000,blocking=true
log_uart:logger|depends=uart0,transport=chardev@uart0
input0:inputdev|depends=uart0,transport=chardev@uart0