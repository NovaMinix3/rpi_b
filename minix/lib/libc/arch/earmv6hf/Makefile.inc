C_HERE=${NETBSDSRCDIR}/minix/lib/libc/arch/${MACHINE_ARCH}
.PATH: ${C_HERE}

SRCS+= 	get_bp.S	\
	read_tsc.c
