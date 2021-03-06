#!/usr/bin/env python3
import os
import sys
import argparse
import subprocess
import tempfile
import shutil

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

CPUS = {
    "CONFIG_SYS_CPU_CORTEX_A15":        "cortex-a15",
    "CONFIG_SYS_CPU_GENERIC_ARMV7A":    "cortex-a15",
    "CONFIG_SYS_CPU_CORTEX_A9":         "cortex-a9",
}


def get_config():
    config_file = os.path.join(SCRIPT_DIR, "..", "..", "bin", "include", "config", "auto.conf")
    try:
        with open(config_file, "r") as f:
            config = {}
            for line in f:
                if line.startswith("#") or line.find("=") < 0:
                    continue
                key, value = line.split("=")
                config[key.strip()] = value.strip()
            return config
    except FileNotFoundError:
        raise Exception("Config file {} not found".format(config_file))


def sdcard_supported(config):
    return config.get("CONFIG_MCI_SUPPORT", "n") == "y"


def main(args):
    config = get_config()

    ram_size = int(config.get("CONFIG_SYS_RAM_SIZE", 0x40000000), 16) / 1024 / 1024
    ram_size = int(ram_size)

    cpus = set(CPUS.keys()) & set(config.keys())
    if not cpus:
        raise Exception("Couldn't find CPU name from auto.conf or cpu not supported")

    cpu = CPUS[cpus.pop()]
    ncpus = config.get("CONFIG_CPU_MAX_CPUS", 1)

    if args.os_debug:
        print("Launching laritOS in debugging mode (listening on :1234)")
        print("Debug with '<your_laritos>/tools/gdb/debug.py'")

    if args.qemu_debug:
        print("Launching qemu in debugging mode (listening on :55555)")
        print("Debug qemu with 'gdb <your_qemu>/bin/arm-softmmu/qemu-system-arm -ex \"target remote :55555\" -q'")

    print("")
    os.system("qemu-system-arm --version | grep version")
    print("---------------------------------------------------------------------------\n")

    with tempfile.NamedTemporaryFile(prefix="laritos") as trace_file:
        basecmd = "\
qemu-system-{arch} -no-reboot --trace events={scriptdir}/trace_events,file={trace} \
-M {machine} -smp {ncpus} -m {ram}M -cpu {cpu} -nographic \
-drive if=pflash,file={scriptdir}/../../bin/kernel.img,format=raw,readonly \
-drive if=pflash,file={scriptdir}/../../bin/system.img,format=raw,readonly" \
            .format(
                arch=args.arch,
                machine=args.machine,
                scriptdir=SCRIPT_DIR,
                trace=trace_file.name,
                ncpus=ncpus,
                cpu=cpu,
                ram=ram_size)

        cmd = [basecmd]
        if args.qemu_debug:
            cmd.insert(0, "gdbserver :55555")
        if args.os_debug:
            cmd.append("-S -s")
        if args.qemu_log:
            cmd.append("-d guest_errors,cpu_reset,int,unimp -D /tmp/qemu.log")
        if sdcard_supported(config):
            cmd.append("-drive if=sd,cache=writeback,file={}/../../bin/data.img,format=raw".format(SCRIPT_DIR))

        cmdstr = " ".join(cmd)
        print(cmdstr + "\n")
        os.system(cmdstr)

        qemudir = os.path.dirname(shutil.which("qemu-system-{}".format(args.arch)) or "")
        if not qemudir:
            raise Exception("Couldn't find qemu's directory to display the trace report")

        treport = subprocess.check_output([os.path.join(qemudir, "..", "scripts", "simpletrace.py"),
                                            os.path.join(qemudir, "..", "trace-events-all"),
                                            trace_file.name])
        if treport:
            print("\nTrace report")
            print(treport.decode("ascii"))


def parse_args(argv):
    parser = argparse.ArgumentParser(description="QEMU launcher for laritOS",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-a", "--arch", default="arm",
                       help="Target architecture")
    parser.add_argument("-M", "--machine", default="virt",
                       help="Emulated machine type")
    parser.add_argument("-i", "--dataimg", default="{}/../../bin/data.img".format(SCRIPT_DIR),
                       help="Filesystem to use as data image. By default, the one under bin/data.img \
                       will be used, which is regenerated everytime you build the OS")
    parser.add_argument("-d", "--os-debug", default=False, action="store_true",
                       help="Launch laritOS in debugging mode (listening on :1234)")
    parser.add_argument("-D", "--qemu-debug", default=False, action="store_true",
                       help="Launch qemu in debugging mode (listening on :55555). \
                       Useful when you want to debug the emulator itself. \
                       Make sure you have the qemu sources and configure the \
                       qemu project with: \
                       configure --target-list=arm-softmmu,arm-linux-user \
                       --enable-debug --enable-trace-backends=simple")
    parser.add_argument("-l", "--qemu-log", default=False, action="store_true",
                       help="Dump QEMU events such as guest errors, interrupts, cpu resets, etc, \
                       in /tmp/qemu.log")
    return parser.parse_args(argv)


if __name__ == "__main__":
    try:
        args = parse_args(sys.argv[1:])
        main(args)
        sys.exit(0)
    except Exception as e:
        print("Error: {}".format(str(e)))
        sys.exit(1)
