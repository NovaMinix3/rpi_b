.include <bsd.own.mk>

.if ((${MACHINE_ARCH} == "earm") || (${MACHINE_ARCH} == "earmv6hf") )
SUBDIR+=	gpio
.endif # ${MACHINE_ARCH} == "earm"

SUBDIR+=	log
SUBDIR+=	random

.include <bsd.subdir.mk>
