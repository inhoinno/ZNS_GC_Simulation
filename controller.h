#ifndef ZNS_controller_H
#define ZNS_controller_H

#include "linux/nvme.h"
#include "linux/nvme_ioctl.h"
#include "linux/lightnvm.h"

#include "util/argconfig.h"
#include "util/suffix.h"


#include <assert.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <inttypes.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <asm/types.h>
#include <linux/posix_types.h>
#include <uuid/uuid.h>
#include <stdbool.h>
#include <stdint.h>
#include <endian.h>

#include <cstring>
#include <random>
#include <iostream>

//Zone Management
#define MAN_CLOSE 0x01
#define MAN_FINISH 0x02
#define MAN_OPEN 0x03
#define MAN_RESET 0x04
#define MAN_OFFLINE 0x05

#define BLOCK_SIZE 512
#define SECTOR_SIZE (BLOCK_SIZE * 8)
#define LOG_SIZE 64

#define unlikely(x) x

#ifdef __CHECKER__
#define __bitwise__ __attribute__((bitwise))
#else
#define __bitwise__
#endif
#define __bitwise __bitwise__

#ifdef __CHECKER__
#define __force       __attribute__((force))
#else
#define __force
#endif

typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;

typedef __u16 __bitwise __sum16;
typedef __u32 __bitwise __wsum;

#define __aligned_u64 __u64 __attribute__((aligned(8)))
#define __aligned_be64 __be64 __attribute__((aligned(8)))
#define __aligned_le64 __le64 __attribute__((aligned(8)))

#ifndef _ASM_GENERIC_INT_LL64_H
#define _ASM_GENERIC_INT_LL64_H

#include <asm/bitsperlong.h>

#ifndef __ASSEMBLY__

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

#ifdef __GNUC__
__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
#else
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
#endif

#endif /* __ASSEMBLY__ */

#endif /* _ASM_GENERIC_INT_LL64_H */

typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;

typedef __u16 __bitwise __sum16;
typedef __u32 __bitwise __wsum;


//struct zone_descriptor *zone_desc_list;
//struct zns_info *zns_info;


/*argconfig.h
enum argconfig_types {
	CFG_NONE,
	CFG_STRING,
	CFG_INT,
	CFG_SIZE,
	CFG_LONG,
	CFG_LONG_SUFFIX,
	CFG_DOUBLE,
	CFG_BOOL,
	CFG_BYTE,
	CFG_SHORT,
	CFG_POSITIVE,
	CFG_INCREMENT,
	CFG_SUBOPTS,
	CFG_FILE_A,
	CFG_FILE_W,
	CFG_FILE_R,
	CFG_FILE_AP,
	CFG_FILE_WP,
	CFG_FILE_RP,
};

#define OPT_ARGS(n) \
	const struct argconfig_commandline_options n[]

#define OPT_END() { NULL }

#define OPT_FLAG(l, s, v, d) \
	{l, s, NULL, CFG_NONE, v, no_argument, d}

#define OPT_SUFFIX(l, s, v, d) \
	{l, s, "IONUM", CFG_LONG_SUFFIX, v, required_argument, d}

#define OPT_LONG(l, s, v, d) \
	{l, s, "NUM", CFG_LONG, v, required_argument, d}

#define OPT_UINT(l, s, v, d) \
	{l, s, "NUM", CFG_POSITIVE, v, required_argument, d}

#define OPT_INT(l, s, v, d) \
	{l, s, "NUM", CFG_INT, v, required_argument, d}

#define OPT_LONG(l, s, v, d) \
	{l, s, "NUM", CFG_LONG, v, required_argument, d}

#define OPT_DOUBLE(l, s, v, d) \
	{l, s, "NUM", CFG_DOUBLE, v, required_argument, d}

#define OPT_BYTE(l, s, v, d) \
	{l, s, "NUM", CFG_BYTE, v, required_argument, d}

#define OPT_SHRT(l, s, v, d) \
	{l, s, "NUM", CFG_SHORT, v, required_argument, d}

#define OPT_STRING(l, s, m, v, d) \
	{l, s, m, CFG_STRING, v, required_argument, d}

#define OPT_FMT(l, s, v, d)  OPT_STRING(l, s, "FMT", v, d)
#define OPT_FILE(l, s, v, d) OPT_STRING(l, s, "FILE", v, d)
#define OPT_LIST(l, s, v, d) OPT_STRING(l, s, "LIST", v, d)

struct argconfig_commandline_options {
	const char *option;
	const char short_option;
	const char *meta;
	enum argconfig_types config_type;
	void *default_value;
	int argument_type;
	const char *help;
};	
end argconfig.h
*/
typedef struct nvme_user_io {
	__u8	opcode;
	__u8	flags;
	__u16	control;
	__u16	nblocks;
	__u16	rsvd;
	__u64	metadata;
	__u64	addr;
	__u64	slba;
	__u32	dsmgmt;
	__u32	reftag;
	__u16	apptag;
	__u16	appmask;
}nvme_user_io;

typedef struct zone_format {
	__le64			zsze;
	__le16			lbafs;
	__u8        zdes;
	__u8        rsrvd[5];
}zone_format;

typedef struct nvme_zns_info{
	int fd;
	__le64			ns_size;
	__le64			ns_cap;
	__le64			ns_use;

	__u8			zfi;
	__le16			ns_op_io_bound;
	__le16			ns_pref_wr_gran;
	__le16			ns_pref_wr_align;
	__le16			ns_pref_dealloc_gran;
	__le16			ns_pref_dealloc_align;
	__le16			ns_opot_wr_size;

	__le32 max_active_res;
	__le32 max_open_res;
	unsigned int opened_zone_num;
	struct zone_format zonef;

	unsigned int max_zone_cnt;
}nvme_zns_info;

enum nvme_print_flags {
	NORMAL	= 0,
	VERBOSE	= 1 << 0,	/* verbosely decode complex values for humans */
	JSON	= 1 << 1,	/* display in json format */
	VS	= 1 << 2,	/* hex dump vendor specific data areas */
	BINARY	= 1 << 3,	/* binary dump raw bytes */
};

struct nvme_zone_info_entry {
	/*Gunhee, Choi*/
	/*Zone Information Enry Structure (total 64 bytes)*/
	__u8		zone_condition_rsvd : 4;
	__u8		rsvd0 : 4;
	__u8		rsvd1 : 4;
	__u8		zone_condition : 4;
	__u8		rsvd8[6];
	__u64		zone_capacity;
	__u64		zone_start_lba;
	__u64		write_pointer;
	__u64		cnt_read;
	__u64		cnt_write;
	__u32		cnt_reset;
	__u8		rsvd56[12];
};

struct nvme_zone_info {
	/*Gunhee, Choi*/
	/*Zone Information Enry Structure (total 64 bytes)*/
	__u64	number_zones;
	__u8	rsvd8[56];
	struct nvme_zone_info_entry * zone_entrys;
};
struct zns_zone_info {
	struct nvme_zone_info_entry zone_entry;
	int zone_number;
	int level;
};

struct zns_share_info {
	int fd;
	__u64 zonesize;
	__u32 activezones;
	__u32 openzones;
	__u16 totalzones;
	struct zns_zone_info * zone_list;
};
struct zns_block {
	__u8 data[BLOCK_SIZE];
};

struct zns_sector {
	struct zns_block data[8];
};

typedef struct NvmeSglDescriptor {
    uint64_t addr;
    uint32_t len;
    uint8_t  rsvd[3];
    uint8_t  type;
} NvmeSglDescriptor;

typedef union NvmeCmdDptr {
    struct {
        uint64_t    prp1;
        uint64_t    prp2;
    };

    NvmeSglDescriptor sgl;
} NvmeCmdDptr;

typedef struct NvmeCmd {
    uint16_t    opcode : 8; //	__u8	opcode;
    uint16_t    fuse   : 2; //	__u8	flags;
    uint16_t    res1   : 4; //	__u16	rsvd1;
    uint16_t    psdt   : 2; //	__u32	nsid;
    uint16_t    cid;        //	__u32	nsid;
    uint32_t    nsid;       //	__u32	cdw2;
    uint64_t    res2;       //	__u32	cdw3;	__u64	metadata;
    uint64_t    mptr;       //	__u64	metadata; 	__u64	addr;
    NvmeCmdDptr dptr;
    uint32_t    cdw10;
    uint32_t    cdw11;
    uint32_t    cdw12;
    uint32_t    cdw13;
    uint32_t    cdw14;
    uint32_t    cdw15;
} NvmeCmd;
#define nvme_admin_cmd nvme_passthru_cmd


#define NVME_IOCTL_ID		_IO('N', 0x40)
#define NVME_IOCTL_ADMIN_CMD	_IOWR('N', 0x41, struct nvme_admin_cmd)
#define NVME_IOCTL_SUBMIT_IO	_IOW('N', 0x42, struct nvme_user_io)
#define NVME_IOCTL_IO_CMD	_IOWR('N', 0x43, struct nvme_passthru_cmd)
#define NVME_IOCTL_RESET	_IO('N', 0x44)
#define NVME_IOCTL_SUBSYS_RESET	_IO('N', 0x45)
#define NVME_IOCTL_RESCAN	_IO('N', 0x46)

const char *nvme_status_to_string(__u16 status);

void * zns_info_ctrl(int fd, void * data);
void * zns_info_ctrl_print(void * data);

void * zns_info_ns(int fd, void * data);
void * zns_info_ns_print(void * data);

int zns_get_info(char * dev);
int zns_get_zone_desc();
int * zns_init_print(struct zns_share_info * zonelist);

__u64 get_zone_to_slba(struct zns_share_info * zonelist, int zonenumber);

int zns_write_request(void * write_data, __le16 nblocks, __le32 data_size, __u64 slba);
int zns_write(void * write_data, int data_size, int zone_number);
int zns_write_interzone(void * write_data, int data_size, int zone_number, int interleave_zonenumber);

int zns_read_request(void * read_data, int nblocks, __u64 slba);
int zns_read(void * read_data, int data_size, int zone_number, __u64 offset);
void zns_set_zone(int zone_number, __u8 value);
int zns_management_send(int zone_number, __u8 value);
int zns_get_log_entry_info(int fd, void * data, __u64 zid);
int zns_get_total_log_entry_info(int fd, int from, int nzones);
int zns_log_info_entry_print(int num, void * data);

int zns_update_zone_info(struct zns_share_info * zonelist, int zonenumber);

int zns_zone_io_managemnet(int fd, __u64 slba, __u64 action);
int zns_zone_finish_request(int fd, __u64 slba);
int zns_zone_open_request(int fd, __u64 slba);
int zns_zone_reset_request(int fd, __u64 slba);
int zns_zone_finish(struct zns_share_info * zonelist, int zonenumber);
int zns_zone_open(struct zns_share_info * zonelist, int zonenumber);
int zns_zone_reset(struct zns_share_info * zonelist, int zonenumber);

int report_zones(int argc, char **argv, struct command *cmd, struct plugin *plugin);


#endif 
