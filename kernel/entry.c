

static int test_bss[100];
static int test_data[100] = { 1, 2, 3, 4, 5 };

void loop(void) {
	while (1) {
		asm("wfi");
	}
}

void kernel_entry(void) {
//	var = "abc";
	asm("mov r0, #0xff");
	int i = 2;

	test_bss[i] = 0xff;
	test_data[0] = 2;

	loop();
}

