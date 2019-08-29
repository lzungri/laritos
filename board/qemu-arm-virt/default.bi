uart0:pl011|baseaddr=0x09000000,blocking=true
log_uart:logger|depends=uart0,transport=stream@uart0
input_uart:inputdev|depends=uart0,transport=stream@uart0