cpu0:armv7-a|iset=le
uart0:pl011|baseaddr=0x09000000
hd0:test|blocks=100,etc=123
log_uart:logger|depends=uart0,transport=uart0
