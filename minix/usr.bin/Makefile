.include <bsd.own.mk>

.if ((${MACHINE_ARCH} == "earm") || (${MACHINE_ARCH} == "earmv6hf"))
SUBDIR+=	eepromread
.endif # ${MACHINE_ARCH} == "earm"

SUBDIR+=	grep
SUBDIR+=	ministat
SUBDIR+=	top
SUBDIR+=	toproto
SUBDIR+=	trace

.include <bsd.subdir.mk>
