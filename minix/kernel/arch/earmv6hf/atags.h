/* atags.h */

#ifndef _ATAGS_H
#define _ATAGS_H

#define	ATAG_CORE	0x54410001
#define	ATAG_MEM	0x54410002
#define	ATAG_VIDEOTEXT	0x54410003
#define	ATAG_RAMDISK	0x54410004
#define	ATAG_INITRD2	0x54420005
#define	ATAG_SERIAL	0x54410006
#define	ATAG_REVISION	0x54410007
#define	ATAG_VIDEOLFB	0x54410008
#define	ATAG_CMDLINE	0x54410009
#define	ATAG_NONE	0x00000000

/* structures for each atag */
struct atag_header {
	u32_t	size; /* length of tag in words including this header */
	u32_t	tag;  /* tag type */
};

struct atag_core {
	u32_t	flags;
	u32_t	pagesize;
	u32_t	rootdev;
};

struct atag_mem {
	u32_t	size;
	u32_t	start;
};

struct atag_videotext {
	u8_t	x;
	u8_t	y;
	u16_t	video_page;
	u8_t	video_mode;
	u8_t	video_cols;
	u16_t	video_ega_bx;
	u8_t	video_lines;
	u8_t	video_isvga;
	u16_t	video_points;
};

struct atag_ramdisk {
	u32_t	flags;
	u32_t	size;
	u32_t	start;
};

struct atag_initrd2 {
	u32_t	start;
	u32_t	size;
};

struct atag_serialnr {
	u32_t	low;
	u32_t	high;
};

struct atag_revision {
	u32_t	rev;
};

struct atag_videolfb {
	u16_t	lfb_width;
	u16_t	lfb_height;
	u16_t	lfb_depth;
	u16_t	lfb_linelength;
	u32_t	lfb_base;
	u32_t	lfb_size;
	u8_t	red_size;
	u8_t	red_pos;
	u8_t	green_size;
	u8_t	green_pos;
	u8_t	blue_size;
	u8_t	blue_pos;
	u8_t	rsvd_size;
	u8_t	rsvd_pos;
};

struct atag_cmdline {
	char	cmdline[1];
};

struct atag {
	struct atag_header	hdr;
	union {
		struct atag_core	core;
		struct atag_mem		mem;
		struct atag_videotext	videotext;
		struct atag_ramdisk	ramdisk;
		struct atag_initrd2	initrd2;
		struct atag_serialnr	serialnr;
		struct atag_revision	revision;
		struct atag_videolfb	videolfb;
		struct atag_cmdline	cmdline;
	} u;
};

#define tag_next(t)     ((struct atag *)((u32_t *)(t) + (t)->hdr.size))
#define tag_size(type)  ((sizeof(struct tag_header) + sizeof(struct type)) >> 2)

#endif /* _ATAGS_H */
