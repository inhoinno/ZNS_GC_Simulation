/* 2020. 11. 22 - Date of initial creation
 * Creater : Myounghoon Oh
 * Modifier : Hojin Shin
 * This file is the U.3 ZNS SSD Contorller header
*/

#ifndef u3controller_H
#define u3controller_H

#ifdef _cpluscplus
extern "C" {
#endif

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

#define BLOCK_SIZE 512
#define _192KB 196608
#define SECTOR_SIZE (BLOCK_SIZE * 8)
#define LOG_SIZE 64

//Zone Type
#define Seq_Write_Req 0x2

//Zone Management
#define MAN_CLOSE 0x01
#define MAN_FINISH 0x02
#define MAN_OPEN 0x03
#define MAN_RESET 0x04
#define MAN_OFFLINE 0x05

//Report Zone list
#define REPORT_NUM 1024
#define REPORT_ALL 1
#define REPORT_PARTIAL 0
#define REPORT_ALL_STATE 0x00
#define REPORT_EMPTY 0x01
#define REPORT_IMPL_OPEN 0x02
#define REPORT_EXPL_OPEN 0x03
#define REPORT_CLOSED 0x04
#define REPORT_FULL 0x05
#define REPORT_READ_ONLY 0x06
#define REPORT_OFFLINE 0x07

//Zone State
#define STATE_EMPTY 0x01
#define STATE_IMPL_OPEN 0x02
#define STATE_EXPL_OPEN 0x03
#define STATE_CLOSED 0x04
#define STATE_READ_ONLY 0x0D
#define STATE_FULL 0x0E
#define STATE_OFFLINE 0x0F

//Log Page Identifier
#define Changed_Zone_list 0xBF

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
// __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;

typedef __u16 __bitwise __sum16;
typedef __u32 __bitwise __wsum;

static inline __le16 u3_cpu_to_le16(uint16_t x)
{
	return (__force __le16)htole16(x);
}
static inline __le32 u3_cpu_to_le32(uint32_t x)
{
	return (__force __le32)htole32(x);
}
static inline __le64 u3_cpu_to_le64(uint64_t x)
{
	return (__force __le64)htole64(x);
}

static inline uint16_t u3_le16_to_cpu(__le16 x)
{
	return le16toh((__force __u16)x);
}
static inline uint32_t u3_le32_to_cpu(__le32 x)
{
	return le32toh((__force __u32)x);
}
static inline uint64_t u3_le64_to_cpu(__le64 x)
{
	return le64toh((__force __u64)x);
}

struct u3_nvme_passthru_cmd {
	__u8	opcode;
	__u8	flags;
	__u16	rsvd1;
	__u32	nsid;
	__u32	cdw2;
	__u32	cdw3;
	__u64	metadata;
	__u64	addr;
	__u32	metadata_len;
	__u32	data_len;
	__u32	cdw10;
	__u32	cdw11;
	__u32	cdw12;
	__u32	cdw13;
	__u32	cdw14;
	__u32	cdw15;
	__u32	timeout_ms;
	__u32	result;
};

struct u3_nvme_user_io {
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
};

enum u3_nvme_opcode {
	u3_nvme_cmd_flush		= 0x00,
	u3_nvme_cmd_write		= 0x01,
	u3_nvme_cmd_read		= 0x02,
	u3_nvme_cmd_write_uncor	= 0x04,
	u3_nvme_cmd_compare	= 0x05,
	u3_nvme_cmd_zone_info	= 0x06,
	u3_nvme_cmd_write_zeroes	= 0x08,
	u3_nvme_cmd_dsm		= 0x09,
	u3_nvme_cmd_verify		= 0x0c,
	u3_nvme_cmd_resv_register	= 0x0d,
	u3_nvme_cmd_resv_report	= 0x0e,
	u3_nvme_cmd_resv_acquire	= 0x11,
	u3_nvme_cmd_resv_release	= 0x15,
};

enum u3_nvme_admin_opcode {
	u3_nvme_admin_delete_sq		= 0x00,
	u3_nvme_admin_create_sq		= 0x01,
	u3_nvme_admin_get_log_page		= 0x02,
	u3_nvme_admin_delete_cq		= 0x04,
	u3_nvme_admin_create_cq		= 0x05,
	u3_nvme_admin_identify		= 0x06,
	u3_nvme_admin_abort_cmd		= 0x08,
	u3_nvme_admin_set_features		= 0x09,
	u3_nvme_admin_get_features		= 0x0a,
	u3_nvme_admin_async_event		= 0x0c,
	u3_nvme_admin_ns_mgmt		= 0x0d,
	u3_nvme_admin_activate_fw		= 0x10,
	u3_nvme_admin_download_fw		= 0x11,
	u3_nvme_admin_dev_self_test	= 0x14,
	u3_nvme_admin_ns_attach		= 0x15,
	u3_nvme_admin_keep_alive		= 0x18,
	u3_nvme_admin_directive_send	= 0x19,
	u3_nvme_admin_directive_recv	= 0x1a,
	u3_nvme_admin_virtual_mgmt		= 0x1c,
	u3_nvme_admin_nvme_mi_send		= 0x1d,
	u3_nvme_admin_nvme_mi_recv		= 0x1e,
	u3_nvme_admin_dbbuf		= 0x7C,
	u3_nvme_admin_format_nvm		= 0x80,
	u3_nvme_admin_security_send	= 0x81,
	u3_nvme_admin_security_recv	= 0x82,
	u3_nvme_admin_sanitize_nvm		= 0x84,
};

struct u3_nvme_id_power_state {
	__le16			max_power;	/* centiwatts */
	__u8			rsvd2;
	__u8			flags;
	__le32			entry_lat;	/* microseconds */
	__le32			exit_lat;	/* microseconds */
	__u8			read_tput;
	__u8			read_lat;
	__u8			write_tput;
	__u8			write_lat;
	__le16			idle_power;
	__u8			idle_scale;
	__u8			rsvd19;
	__le16			active_power;
	__u8			active_work_scale;
	__u8			rsvd23[9];
};

struct u3_nvme_id_ctrl {
	__le16			vid;
	__le16			ssvid;
	char			sn[20];
	char			mn[40];
	char			fr[8];
	__u8			rab;
	__u8			ieee[3];
	__u8			cmic;
	__u8			mdts;
	__le16			cntlid;
	__le32			ver;
	__le32			rtd3r;
	__le32			rtd3e;
	__le32			oaes;
	__le32			ctratt;
	__le16			rrls;
	__u8			rsvd102[154];
	__le16			oacs;
	__u8			acl;
	__u8			aerl;
	__u8			frmw;
	__u8			lpa;
	__u8			elpe;
	__u8			npss;
	__u8			avscc;
	__u8			apsta;
	__le16			wctemp;
	__le16			cctemp;
	__le16			mtfa;
	__le32			hmpre;
	__le32			hmmin;
	__u8			tnvmcap[16];
	__u8			unvmcap[16];
	__le32			rpmbs;
	__le16			edstt;
	__u8			dsto;
	__u8			fwug;
	__le16			kas;
	__le16			hctma;
	__le16			mntmt;
	__le16			mxtmt;
	__le32			sanicap;
	__le32			hmminds;
	__le16			hmmaxd;
	__le16			nsetidmax;
	__u8			rsvd340[2];
	__u8			anatt;
	__u8			anacap;
	__le32			anagrpmax;
	__le32			nanagrpid;
	__u8			rsvd352[160];
	__u8			sqes;
	__u8			cqes;
	__le16			maxcmd;
	__le32			nn;
	__le16			oncs;
	__le16			fuses;
	__u8			fna;
	__u8			vwc;
	__le16			awun;
	__le16			awupf;
	__u8			nvscc;
	__u8			nwpc;
	__le16			acwu;
	__u8			rsvd534[2];
	__le32			sgls;
	__le32			mnan;
	__u8			rsvd544[224];
	char			subnqn[256];
	__u8			rsvd1024[768];
	__le32			ioccsz;
	__le32			iorcsz;
	__le16			icdoff;
	__u8			ctrattr;
	__u8			msdbd;
	__u8			rsvd1804[244];
	struct u3_nvme_id_power_state	psd[32];
	__u8			vs[1024];
};

struct u3_nvme_lbaf {
	__le16			ms;
	__u8			ds;
	__u8			rp;
};

struct u3_zone_format {
	__le64			zsze;
	__le16			lbafs;
	__u8        zdes;
	__u8        rsrvd[5];
};

struct u3_nvme_id_ns {
	__le64			nsze;
	__le64			ncap;
	__le64			nuse;//24
	__u8			nsfeat;
	__u8			nlbaf;
	__u8			flbas;
	__u8			mc;
	__u8			dpc;
	__u8			dps;
	__u8			nmic;
	__u8			rescap;
	__u8			fpi;
	__u8			dlfeat;
	__le16			nawun;//36
	__le16			nawupf;
	__le16			nacwu;
	__le16			nabsn;
	__le16			nabo;
	__le16			nabspf;
	__le16			noiob;//48
	__u8			nvmcap[16];//64byte
	__le16			npwg;
	__le16			npwa;
	__le16			npdg;
	__le16			npda;
	__le16			nows;//74
	__u8			rsvd74[18];//92
	__le32    anagrpid;//96
	__u8			rsvd96[3];
	__u8      nsattr;
	__le16			nvmsetid;
	__le16			endgid;
	__u8        nguid[16];//120
	__u8      eui64[8];//128
	struct u3_nvme_lbaf	lbaf[16];//128+64 == 192
	__u8			rsvd192[64];//256byte
	//hoon
	__u8			zfi; //257
	__u8			nzonef; // 258
	__u8			zoc; // 259
	__u8			rsvd259[13];//272
	__le32    mar; // 276
	__le32    mor; // 280
	__u8			rsvd280[8];//288
	__le32    zal; // 292
	__le32    rrl; // 296
	__u8			rsvd296[3480]; //3776
	struct u3_zone_format zonef[4]; //3776 + 64 == 3840
	__u8			vs[256];//4096 byte
	//4096 byte
};

typedef struct NvmeLBAF {
    uint16_t    ms;
    uint8_t     lbads;
    uint8_t     rp;
} NvmeLBAF;

typedef struct NvmeIdNs {
    uint64_t    nsze;
    uint64_t    ncap;
    uint64_t    nuse;
    uint8_t     nsfeat;
    uint8_t     nlbaf;
    uint8_t     flbas;
    uint8_t     mc;
    uint8_t     dpc;
    uint8_t     dps;
    uint8_t     nmic;
    uint8_t     rescap;
    uint8_t     fpi;
    uint8_t     dlfeat;
    uint16_t    nawun;
    uint16_t    nawupf;
    uint16_t    nacwu;
    uint16_t    nabsn;
    uint16_t    nabo;
    uint16_t    nabspf;
    uint16_t    noiob;
    uint8_t     nvmcap[16];
    uint16_t    npwg;
    uint16_t    npwa;
    uint16_t    npdg;
    uint16_t    npda;
    uint16_t    nows;
    uint8_t     rsvd74[30];
    uint8_t     nguid[16];
    uint64_t    eui64;
    NvmeLBAF    lbaf[16];
    uint8_t     rsvd192[192];
    uint8_t     vs[3712];
} NvmeIdNs;

struct u3_controller_identify{
	__u8   zamds;                           //Zone Append Maximum Data Size(ZAMDS), minimum memory size * 2^n
	__u8	 rsvd[4095];                                //Reserved
};

struct u3_zns_info{
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
	struct u3_zone_format zonef;

	unsigned int max_zone_cnt;
};

struct u3_zone_descriptor {
	/*64 bytes*/
	__u8		type : 4;       //Zone Type(ZT)
	__u8		rsvd0 : 4;      //Reserved

	__u8		rsvd1 : 4;      //Reserved
	__u8		state : 4;      //Zone State(ZS)

	//Zone Attribute(ZA)
	__u8		zfc : 1;        //Zone Finished by Controller 
	__u8		fzr : 1;        //Finish Zone Recommended
	__u8		rzr : 1;        //Reset Zone Recommended
	__u8		rsvd2 : 4;      //Reserved
	__u8		zdev : 1;       //Zone Descriptor Extension Valid

	__u8		rsvd3[5];      //Reserved

	__u64		capacity;       //Zone Capacity(ZCAP)

	__u64		start_lba;      //Zone Start Logical Block Address(ZSLBA)

	__u64		wp;             //Write Pointer(WP)

	__u8		rsvd4[32];      //Reserved
};


//get_log
struct u3_zone_identify {
	__u64   zslba;  
};

struct u3_lba_format_extension_identify{
	/*128byte*/
	__u8   zone_size[64];                           //Zone Size
	__u8   zone_descriptor_extension_size[8];       //Zone Descriptor Extension Size
	__u8	 rsvd[56];                                //Reserved
};

struct u3_zns_block {
	__u8 data[BLOCK_SIZE];
};

struct u3_zns_sector {
	struct u3_zns_block data[8];
};

#define u3_nvme_admin_cmd u3_nvme_passthru_cmd

#define U3_NVME_IOCTL_ID		_IO('N', 0x40)
#define U3_NVME_IOCTL_ADMIN_CMD	_IOWR('N', 0x41, struct u3_nvme_admin_cmd)
#define U3_NVME_IOCTL_SUBMIT_IO	_IOW('N', 0x42, struct u3_nvme_user_io)
#define U3_NVME_IOCTL_IO_CMD	_IOWR('N', 0x43, struct u3_nvme_passthru_cmd)
#define U3_NVME_IOCTL_RESET	_IO('N', 0x44)
#define U3_NVME_IOCTL_SUBSYS_RESET	_IO('N', 0x45)
#define U3_NVME_IOCTL_RESCAN	_IO('N', 0x46)

void* u3_identify_ns(int fd, void * data);
void* u3_identify_ctrl(int fd, void * data);
int u3_zns_get_info(char * dev);
void u3_print_zns_info();
int u3_zns_format();
int u3_zns_management_send(int zone_number, __u8 value);
void u3_zns_set_zone(int zone_number, __u8 value);
void* u3_zns_management_recv(unsigned int report_num, unsigned int partial, unsigned int option, unsigned int slba);
void u3_zns_get_zone_desc(unsigned int partial, unsigned int option, unsigned int from_zone_num, unsigned int report_zone_cnt, bool init);
void u3_print_zone_desc(unsigned int total_zone);
int u3_zns_write_request(void * write_data, __le16 nblocks, __le32 data_size, __u64 slba);
int u3_zns_write(void * write_data, int data_size, int zone_number);
int u3_zns_read_request(void * read_data, int nblocks, __u64 slba);
int u3_zns_read(void * read_data, int data_size, int zone_number, __u64 offset);
int u3_zns_set_zone_change_notification();

extern struct u3_zone_descriptor *zone_desc_list;
extern struct u3_zns_info *zns_info;

#ifdef __cpluscplus
}
#endif
#endif