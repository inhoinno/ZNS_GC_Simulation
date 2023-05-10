#include "controller.h"

struct nvme_zns_info * femu_zns_info;
struct nvme_id_ns * femu_id_ns;
struct nvme_zns_id_ns * femu_zns_id_ns;
struct nvme_zone_report * femu_zone_report;
struct nvme_zns_desc * femu_zone_desc_list;


int inter_zone_counter = 0;
static const char *namespace_id = "Namespace identifier to use";
//nvme-status.c

static inline __u8 nvme_generic_status_to_errno(__u16 status)
{
	switch (status) {
	case NVME_SC_INVALID_OPCODE:
	case NVME_SC_INVALID_FIELD:
	case NVME_SC_INVALID_NS:
	case NVME_SC_SGL_INVALID_LAST:
	case NVME_SC_SGL_INVALID_COUNT:
	case NVME_SC_SGL_INVALID_DATA:
	case NVME_SC_SGL_INVALID_METADATA:
	case NVME_SC_SGL_INVALID_TYPE:
	case NVME_SC_SGL_INVALID_OFFSET:
	case NVME_SC_CMB_INVALID_USE:
	case NVME_SC_PRP_INVALID_OFFSET:
		return EINVAL;
	case NVME_SC_CMDID_CONFLICT:
		return EADDRINUSE;
	case NVME_SC_DATA_XFER_ERROR:
	case NVME_SC_INTERNAL:
	case NVME_SC_SANITIZE_FAILED:
		return EIO;
	case NVME_SC_POWER_LOSS:
	case NVME_SC_ABORT_REQ:
	case NVME_SC_ABORT_QUEUE:
	case NVME_SC_FUSED_FAIL:
	case NVME_SC_FUSED_MISSING:
		return EWOULDBLOCK;
	case NVME_SC_CMD_SEQ_ERROR:
		return EILSEQ;
	case NVME_SC_SANITIZE_IN_PROGRESS:
	case NVME_SC_FORMAT_IN_PROGRESS:
		return EINPROGRESS;
	case NVME_SC_NS_WRITE_PROTECTED:
	case NVME_SC_NS_NOT_READY:
	case NVME_SC_RESERVATION_CONFLICT:
		return EACCES;
	case NVME_SC_LBA_RANGE:
		return EREMOTEIO;
	case NVME_SC_CAP_EXCEEDED:
		return ENOSPC;
	case NVME_SC_OPERATION_DENIED:
		return EPERM;
	}

	return EIO;
}

static inline __u8 nvme_cmd_specific_status_to_errno(__u16 status)
{
	switch (status) {
	case NVME_SC_CQ_INVALID:
	case NVME_SC_QID_INVALID:
	case NVME_SC_QUEUE_SIZE:
	case NVME_SC_FIRMWARE_SLOT:
	case NVME_SC_FIRMWARE_IMAGE:
	case NVME_SC_INVALID_VECTOR:
	case NVME_SC_INVALID_LOG_PAGE:
	case NVME_SC_INVALID_FORMAT:
	case NVME_SC_INVALID_QUEUE:
	case NVME_SC_NS_INSUFFICIENT_CAP:
	case NVME_SC_NS_ID_UNAVAILABLE:
	case NVME_SC_CTRL_LIST_INVALID:
	case NVME_SC_BAD_ATTRIBUTES:
	case NVME_SC_INVALID_PI:
	case NVME_SC_INVALID_CTRL_ID:
	case NVME_SC_INVALID_SECONDARY_CTRL_STATE:
	case NVME_SC_INVALID_NUM_CTRL_RESOURCE:
	case NVME_SC_INVALID_RESOURCE_ID:
	case NVME_SC_ANA_INVALID_GROUP_ID:
		return EINVAL;
	case NVME_SC_ABORT_LIMIT:
	case NVME_SC_ASYNC_LIMIT:
		return EDQUOT;
	case NVME_SC_FW_NEEDS_CONV_RESET:
	case NVME_SC_FW_NEEDS_SUBSYS_RESET:
	case NVME_SC_FW_NEEDS_MAX_TIME:
		return ERESTART;
	case NVME_SC_FEATURE_NOT_SAVEABLE:
	case NVME_SC_FEATURE_NOT_CHANGEABLE:
	case NVME_SC_FEATURE_NOT_PER_NS:
	case NVME_SC_FW_ACTIVATE_PROHIBITED:
	case NVME_SC_NS_IS_PRIVATE:
	case NVME_SC_BP_WRITE_PROHIBITED:
	case NVME_SC_READ_ONLY:
	case NVME_SC_PMR_SAN_PROHIBITED:
		return EPERM;
	case NVME_SC_OVERLAPPING_RANGE:
	case NVME_SC_NS_NOT_ATTACHED:
		return ENOSPC;
	case NVME_SC_NS_ALREADY_ATTACHED:
		return EALREADY;
	case NVME_SC_THIN_PROV_NOT_SUPP:
		return EOPNOTSUPP;
	case NVME_SC_DEVICE_SELF_TEST_IN_PROGRESS:
		return EINPROGRESS;
	}

	return EIO;
}

/*
 * nvme_status_type - It gives SCT(Status Code Type) in status field in
 *                    completion queue entry.
 * @status: status field located at DW3 in completion queue entry
 */
static inline __u8 nvme_status_type(__u16 status)
{
	return (status & 0x700) >> 8;
}

static inline __u8 nvme_fabrics_status_to_errno(__u16 status)
{
	switch (status) {
	case NVME_SC_CONNECT_FORMAT:
	case NVME_SC_CONNECT_INVALID_PARAM:
		return EINVAL;
	case NVME_SC_CONNECT_CTRL_BUSY:
		return EBUSY;
	case NVME_SC_CONNECT_RESTART_DISC:
		return ERESTART;
	case NVME_SC_CONNECT_INVALID_HOST:
		return ECONNREFUSED;
	case NVME_SC_DISCOVERY_RESTART:
		return EAGAIN;
	case NVME_SC_AUTH_REQUIRED:
		return EPERM;
	}

	return EIO;
}

static inline __u8 nvme_path_status_to_errno(__u16 status)
{
	switch (status) {
	case NVME_SC_INTERNAL_PATH_ERROR:
	case NVME_SC_ANA_PERSISTENT_LOSS:
	case NVME_SC_ANA_INACCESSIBLE:
	case NVME_SC_ANA_TRANSITION:
	case NVME_SC_CTRL_PATHING_ERROR:
	case NVME_SC_HOST_PATHING_ERROR:
	case NVME_SC_HOST_CMD_ABORT:
		return EACCES;
	}

	return EIO;
}

/*
 * nvme_status_to_errno - It converts given status to errno mapped
 * @status: >= 0 for nvme status field in completion queue entry,
 *          < 0 for linux internal errors
 * @fabrics: true if given status is for fabrics
 *
 * Notes: This function will convert a given status to an errno mapped
 */
__u8 nvme_status_to_errno(int status, bool fabrics)
{
	__u8 sct;

	if (!status)
		return 0;

	if (status < 0) {
		if (errno)
			return errno;
		return status;
	}

	/*
	 * The actual status code is enough with masking 0xff, but we need to
	 * cover status code type which is 3bits with 0x7ff.
	 */
	status &= 0x7ff;

	sct = nvme_status_type(status);
	switch (sct) {
	case NVME_SCT_GENERIC:
		return nvme_generic_status_to_errno(status);
	case NVME_SCT_CMD_SPECIFIC:
		if (!fabrics) {
			return nvme_cmd_specific_status_to_errno(status);
		}
		return nvme_fabrics_status_to_errno(status);
	case NVME_SCT_PATH:
		return nvme_path_status_to_errno(status);
	}

	/*
	 * Media, integrity related status, and the others will be mapped to
	 * EIO.
	 */
	return EIO;
}

//nvme.c nvme-ioctl.c 
int nvme_submit_passthru(int fd, unsigned long ioctl_cmd,
			 struct nvme_passthru_cmd *cmd)
{
	return ioctl(fd, ioctl_cmd, cmd);
}
int nvme_submit_io_passthru(int fd, struct nvme_passthru_cmd *cmd)
{
	return ioctl(fd, NVME_IOCTL_IO_CMD, cmd);
}

int nvme_passthru(int fd, unsigned long ioctl_cmd, __u8 opcode,
		  __u8 flags, __u16 rsvd,
		  __u32 nsid, __u32 cdw2, __u32 cdw3, __u32 cdw10, __u32 cdw11,
		  __u32 cdw12, __u32 cdw13, __u32 cdw14, __u32 cdw15,
		  __u32 data_len, void *data, __u32 metadata_len,
		  void *metadata, __u32 timeout_ms, __u32 *result)
{
	struct nvme_passthru_cmd cmd = {
		.opcode		= opcode,
		.flags		= flags,
		.rsvd1		= rsvd,
		.nsid		= nsid,
		.cdw2		= cdw2,
		.cdw3		= cdw3,
		.metadata	= (__u64)(uintptr_t) metadata,
		.addr		= (__u64)(uintptr_t) data,
		.metadata_len	= metadata_len,
		.data_len	= data_len,
		.cdw10		= cdw10,
		.cdw11		= cdw11,
		.cdw12		= cdw12,
		.cdw13		= cdw13,
		.cdw14		= cdw14,
		.cdw15		= cdw15,
		.timeout_ms	= timeout_ms,
		.result		= 0,
	};
	int err;

	err = nvme_submit_passthru(fd, ioctl_cmd, &cmd);
	if (!err && result)
		*result = cmd.result;
	return err;
}

int nvme_submit_admin_passthru(int fd, struct nvme_passthru_cmd *cmd)
{
	return ioctl(fd, NVME_IOCTL_ADMIN_CMD, cmd);
}

int nvme_identify13(int fd, __u32 nsid, __u32 cdw10, __u32 cdw11, void *data)
{
	struct nvme_admin_cmd cmd = {
		.opcode		= nvme_admin_identify,
		.nsid		= nsid,
		.addr		= (__u64)(uintptr_t) data,
		//.data_len	= NVME_IDENTIFY_DATA_SIZE,
        .data_len   =4096,
		.cdw10		= cdw10,
		.cdw11		= cdw11,
	};

	return nvme_submit_admin_passthru(fd, &cmd);
}

int nvme_identify(int fd, __u32 nsid, __u32 cdw10, void *data)
{
	return nvme_identify13(fd, nsid, cdw10, 0, data);
}

int nvme_identify_ns(int fd, __u32 nsid, bool present, void *data)
{
	int cns = present ? NVME_ID_CNS_NS_PRESENT : NVME_ID_CNS_NS;

	return nvme_identify(fd, nsid, cns, data);
}
int nvme_get_nsid(int fd)
{
	static struct stat nvme_stat;
	int err = fstat(fd, &nvme_stat);

	if (err < 0)
		return err;

	return ioctl(fd, NVME_IOCTL_ID);
}
int nvme_zns_identify_ns(int fd, __u32 nsid, void *data)
{
	return nvme_identify13(fd, nsid, NVME_ID_CNS_CSI_ID_NS, 2 << 24, data);
}
//nvme-print.c
void nvme_show_status(__u16 status)
{
	fprintf(stderr, "NVMe status: %s(%#x)\n", nvme_status_to_string(status),
		status);
}

static void *__nvme_alloc(size_t len, bool *huge)
{
	void *p;

	if (!posix_memalign(&p, getpagesize(), len)) {
		*huge = false;
		memset(p, 0, len);
		return p;
	}
	return NULL;
}

void nvme_free(void *p, bool huge)
{
	free(p);
}

void *nvme_alloc(size_t len, bool *huge)
{
	return __nvme_alloc(len, huge);
}

int nvme_zns_mgmt_recv(int fd, __u32 nsid, __u64 slba,
		       enum nvme_zns_recv_action zra, __u8 zrasf,
		       bool zras_feat, __u32 data_len, void *data)
{
	__u32 cdw10 = slba & 0xffffffff;
	__u32 cdw11 = slba >> 32;
	__u32 cdw12 = (data_len >> 2) - 1;
	__u32 cdw13 = zra | zrasf << 8 | zras_feat << 16;

	struct nvme_passthru_cmd cmd = {
		.opcode		= nvme_zns_cmd_mgmt_recv,
		.nsid		= nsid,
		.addr		= (__u64)(uintptr_t)data,
		.data_len	= data_len,
		.cdw10		= cdw10,
		.cdw11		= cdw11,
		.cdw12		= cdw12,
		.cdw13		= cdw13,

	};

	return nvme_submit_io_passthru(fd, &cmd);
}

static int get_zdes_bytes(int fd, __u32 nsid)
{
	struct nvme_zns_id_ns ns;
	struct nvme_id_ns id_ns;
	__u8 lbaf;
	int err;

	err = nvme_identify_ns(fd, nsid,  false, &id_ns);
	if (err > 0) {
		nvme_show_status(err);
		return -1;
	} else if (err < 0) {
		perror("identify namespace");
		return -1;
	}

	err = nvme_zns_identify_ns(fd, nsid,  &ns);
	if (err > 0) {
		nvme_show_status(err);
		return -1;
	} else if (err < 0) {
		perror("zns identify namespace");
		return -1;
	}

	lbaf = id_ns.flbas & NVME_NS_FLBAS_LBA_MASK;
	return ns.lbafe[lbaf].zdes << 6;
}

static char *zone_type_to_string(__u8 cond)
{
	switch (cond) {
	case NVME_ZONE_TYPE_SEQWRITE_REQ:
		return "SEQWRITE_REQ";
	default:
		return "Unknown";
	}
}

static char *zone_state_to_string(__u8 state)
{
	switch (state) {
	case NVME_ZNS_ZS_EMPTY:
		return "EMPTY";
	case NVME_ZNS_ZS_IMPL_OPEN:
		return "IMP_OPENED";
	case NVME_ZNS_ZS_EXPL_OPEN:
		return "EXP_OPENED";
	case NVME_ZNS_ZS_CLOSED:
		return "CLOSED";
	case NVME_ZNS_ZS_READ_ONLY:
		return "READONLY";
	case NVME_ZNS_ZS_FULL:
		return "FULL";
	case NVME_ZNS_ZS_OFFLINE:
		return "OFFLINE";
	default:
		return "Unknown State";
	}
}

int nvme_zns_report_zones(int fd, __u32 nsid, __u64 slba, bool extended,
			  enum nvme_zns_report_options opts, bool partial,
			  __u32 data_len, void *data)
{
	enum nvme_zns_recv_action zra;

	if (extended)
		zra = NVME_ZNS_ZRA_EXTENDED_REPORT_ZONES;
	else
		zra = NVME_ZNS_ZRA_REPORT_ZONES;
	printf("extended %d\n" , extended);	//1
	return nvme_zns_mgmt_recv(fd, nsid, slba, zra, opts, partial,
		data_len, data);
}

void nvme_show_zns_report_zones(void *report, __u32 descs,
	__u8 ext_size, __u32 report_size, unsigned long flags)
{
	struct nvme_zone_report *r = (nvme_zone_report *)report;
	struct nvme_zns_desc *desc;
	int i, verbose = flags & VERBOSE;
	__u64 nr_zones = le64_to_cpu(r->nr_zones);

	if (nr_zones < descs)
		descs = nr_zones;
	/*
	if (flags & BINARY)
		return d_raw((unsigned char *)report, report_size);
	else if (flags & JSON)
		return json_nvme_zns_report_zones(report, descs, 
				ext_size, report_size, nr_zones);
	*/
	for (i = 0; i < descs; i++) {
		desc = (struct nvme_zns_desc *)
			(report + sizeof(*r) + i * (sizeof(*desc) + ext_size));
			printf("SLBA: %#-10"PRIx64" WP: %#-10"PRIx64" Cap: %#-10"PRIx64" State: %-12s Type: %-14s\n",
				(uint64_t)le64_to_cpu(desc->zslba), (uint64_t)le64_to_cpu(desc->wp),
				(uint64_t)le64_to_cpu(desc->zcap), zone_state_to_string(desc->zs >> 4),
				zone_type_to_string(desc->zt));
			//nvme_show_zns_report_zone_attributes(desc->za, desc->zai);
		/*
		if (ext_size && (desc->za & NVME_ZNS_ZA_ZDEV)) {
			printf("Extension Data: ");
			d((unsigned char *)desc + sizeof(*desc), ext_size, 16, 1);
			printf("..\n");
		}*/
	}
}
/**
 * zns_get_info (char * dev)
 * @param dev device path
 * @do
 * fd, nsze, ncap, nuse, niob initialization
 * zone desc list initialization based on max zone cnt
 *
*/
		
int zns_get_info(char * dev)
{
	const char *desc = "Retrieve the Report Zones data structure";
	const char *zslba = "starting LBA of the zone";
	const char *num_descs = "number of descriptors to retrieve (default: all of them)";
	const char *state = "state of zones to list";
	const char *ext = "set to use the extended report zones";
	const char *part = "set to use the partial report";
	const char *verbose = "show report zones verbosity";
    int i;
	int fd, zdes = 0, err = -1;
    void * tempdata = malloc(SECTOR_SIZE);
    void * temp_log_data = malloc(LOG_SIZE);
    //struct nvme_zone_info_entry * zone_entrys;
    //struct nvme_id_ns * id_ns;
	struct nvme_zone_report *r ;
	struct nvme_zns_desc *zdesc;
	struct nvme_zone_report *buff;
	__u32 report_size;
	void *report;
	bool huge = true;
	unsigned int nr_zones_chunks = 4096,   /* 1024 entries * 64 bytes per entry = 64k byte transfer */
		nr_zones_retrieved = 0,
		nr_zones,
		offset,
		log_len;
	int total_nr_zones = 0;
	struct nvme_zns_id_ns * id_zns = (struct nvme_zns_id_ns *) malloc(sizeof(struct nvme_zns_id_ns));
	struct nvme_id_ns * id_ns = (struct nvme_id_ns *)malloc(sizeof(struct nvme_id_ns));
	uint8_t lbaf;
	__le64	zsze;

	struct config {
		char *output_format;
		__u64 zslba;
		__u32 namespace_id;
		int   num_descs;
		int   state;
		int   verbose;
		bool  extended;
		bool  partial;
	};

	struct config cfg = {
		.output_format = "normal",
		.num_descs = -1,
	};
	
	OPT_ARGS(opts) = {
		OPT_UINT("namespace-id",  'n', &cfg.namespace_id,   namespace_id),
		OPT_SUFFIX("start-lba",   's', &cfg.zslba,          zslba),
		OPT_UINT("descs",         'd', &cfg.num_descs,      num_descs),
		OPT_UINT("state",         'S', &cfg.state,          state),
		//OPT_FMT("output-format",  'o', &cfg.output_format,  output_format),
		OPT_FLAG("verbose",       'v', &cfg.verbose,        verbose),
		OPT_FLAG("extended",      'e', &cfg.extended,       ext),
		OPT_FLAG("partial",       'p', &cfg.partial,        part),
		OPT_END()
	};
	dev="/dev/nvme0n1";
	fd = open(dev, O_RDWR);
	err = cfg.namespace_id = nvme_get_nsid(fd);
	if (err < 0) {
		close(fd);
		perror("get-namespace-id");
	}
	//cfg.extended = true;	//report all
	cfg.extended = false;

	zdes = get_zdes_bytes(fd, cfg.namespace_id);
	if (zdes < 0) {
		err = zdes;
		goto close_fd;
	}
	
	err = nvme_identify_ns(fd, cfg.namespace_id, false, id_ns);
	if (err) {
		nvme_show_status(err);
		goto close_fd;
	}

	err = nvme_zns_identify_ns(fd, cfg.namespace_id, id_zns);
	if (!err) {
		/* get zsze field from zns id ns data - needed for offset calculation */
		lbaf = id_ns->flbas & NVME_NS_FLBAS_LBA_MASK;
	    zsze = le64_to_cpu(id_zns->lbafe[lbaf].zsze);
	}
	else {
		nvme_show_status(err);
		goto close_fd;
		//close(fd);
	}

	//
	femu_id_ns = id_ns;
	femu_zns_id_ns = id_zns;
	femu_zns_info = (struct nvme_zns_info * )malloc(sizeof(struct nvme_zns_info));
	femu_zns_info->fd =fd;
	femu_zns_info->zonef.zsze = zsze;
	//
	log_len = sizeof(struct nvme_zone_report);
	buff = (nvme_zone_report * )calloc(1, log_len);
	if (!buff) {
		err = -ENOMEM;
		goto close_fd;
		//goto close_fd;
	}

	err = nvme_zns_report_zones(fd, cfg.namespace_id, 0, 0, (nvme_zns_report_options)cfg.state, 0, log_len, buff);
	if (err > 0) {
		nvme_show_status(err);
		goto free_buff;
	}
	else if (err < 0) {
		perror("zns report-zones");
		goto free_buff;
	}

	total_nr_zones = le64_to_cpu(buff->nr_zones);
	femu_zns_info->max_zone_cnt = total_nr_zones;

	if (cfg.num_descs == -1) {
		cfg.num_descs = total_nr_zones;
	}

	nr_zones = cfg.num_descs;
	if (nr_zones < nr_zones_chunks)
		nr_zones_chunks = nr_zones;

	log_len = sizeof(struct nvme_zone_report) + ((sizeof(struct nvme_zns_desc) * nr_zones_chunks) + (nr_zones_chunks * zdes));
	report_size = log_len;

	report = nvme_alloc(report_size, &huge);
	if (!report) {
		perror("alloc");
		err = -ENOMEM;
		goto close_fd;
	}

	offset = cfg.zslba;
	printf("nr_zones: %"PRIu64"\n", (uint64_t)le64_to_cpu(total_nr_zones));

	while (nr_zones_retrieved < nr_zones) {
		if (nr_zones_retrieved >= nr_zones)
			break;

		if (nr_zones_retrieved + nr_zones_chunks > nr_zones) {
			nr_zones_chunks = nr_zones - nr_zones_retrieved;
			log_len = sizeof(struct nvme_zone_report) + ((sizeof(struct nvme_zns_desc) * nr_zones_chunks) + (nr_zones_chunks * zdes));
		}

		err = nvme_zns_report_zones(fd, cfg.namespace_id, offset,
			cfg.extended, (nvme_zns_report_options)cfg.state, cfg.partial, log_len, report);
		if (err > 0) {
			nvme_show_status(err);
			break;
		}

		if (!err){
			nvme_show_zns_report_zones(report, 10, zdes, log_len, 0);
		}
			//nvme_show_zns_report_zones(report, nr_zones_chunks, zdes, log_len, 0);

		nr_zones_retrieved += nr_zones_chunks;
		offset = (nr_zones_retrieved * zsze);
    }
	// ((struct nvme_zns_desc *)) report >>> nvme_zone_desc_list
	femu_zone_desc_list = (struct nvme_zns_desc *) (report + sizeof(*r));
	printf("zone desc list [0] \n ");
	//printf("zone desc list sze %d \n " sizeof(femu_zone_desc_list));
	printf("SLBA: %#-10"PRIx64" WP: %#-10"PRIx64" Cap: %#-10"PRIx64" State: %-12s Type: %-14s\n",
				(uint64_t)le64_to_cpu(femu_zone_desc_list[0].zslba), (uint64_t)le64_to_cpu(femu_zone_desc_list[0].wp),
				(uint64_t)le64_to_cpu(femu_zone_desc_list[0].zcap), zone_state_to_string(femu_zone_desc_list[0].zs >> 4),
				zone_type_to_string(femu_zone_desc_list[0].zt));

	
	
	for (i = 0; i < 32; i++) {
		/*desc = (struct nvme_zns_desc *) (report + sizeof(*r) + i * (sizeof(*desc) + ext_size));*/
		
			printf("i %#-10"PRIx64" SLBA: %#-10"PRIx64" WP: %#-10"PRIx64" Cap: %#-10"PRIx64" State: %-12s Type: %-14s\n", i,
				(uint64_t)le64_to_cpu(femu_zone_desc_list[i].zslba), (uint64_t)le64_to_cpu(femu_zone_desc_list[i].wp),
				(uint64_t)le64_to_cpu(femu_zone_desc_list[i].zcap), zone_state_to_string(femu_zone_desc_list[i].zs >> 4),
				zone_type_to_string(femu_zone_desc_list[i].zt));

	}
	

    
	//zonelist->zone_list = (struct zns_zone_info *)malloc(sizeof(struct zns_zone_info) * zonelist->totalzones);
    
	std::cout << " fd "<< fd << std::endl;
    //std::cout << " zonelist->zonesize "<< zonelist->zonesize << std::endl;
	std::cout << " id_zns.lbafe[lbaf].zsze "<< zsze << std::endl;
    std::cout << " max active resources id_zns.mar "<< id_zns->mar << "0xFFFFFFFF = 4294967295 = no limit" <<std::endl;
	std::cout << " max open resources id_zns.mor "<< id_zns->mor << "0xFFFFFFFF = 4294967295 = no limit" <<std::endl;
    //std::cout << " zonelist->openzones "<< zonelist->openzones << std::endl;
    std::cout << " totalzones buff->nr_zones "<< buff->nr_zones << std::endl;
	/*
    for(i=0; i<zonelist->totalzones; i++)
    {
        memset(temp_log_data, 0, LOG_SIZE);
        zns_get_log_entry_info(zonelist->fd, temp_log_data, i);
        struct nvme_zone_info_entry * temp_zone_info_entry = (struct nvme_zone_info_entry *)temp_log_data;
        (zonelist->zone_list)[i].zone_entry.zone_condition = temp_zone_info_entry->zone_condition;
        (zonelist->zone_list)[i].zone_entry.zone_capacity = temp_zone_info_entry->zone_capacity;
        (zonelist->zone_list)[i].zone_entry.write_pointer = temp_zone_info_entry->write_pointer;
        (zonelist->zone_list)[i].zone_entry.zone_start_lba = temp_zone_info_entry->zone_start_lba;
        (zonelist->zone_list)[i].zone_entry.cnt_read = temp_zone_info_entry->cnt_read;
        (zonelist->zone_list)[i].zone_entry.cnt_write = temp_zone_info_entry->cnt_write;
        (zonelist->zone_list)[i].zone_entry.cnt_reset = temp_zone_info_entry->cnt_reset;
    }
	*/
    
    free(tempdata);
    free(temp_log_data);
    
    return fd;

free_buff:
	free(buff);
close_fd:
	close(fd);
	return nvme_status_to_errno(err, false);
}
/*
void nvme_show_zns_report_zones(void *report, __u32 descs,
	__u8 ext_size, __u32 report_size, unsigned long flags)
{
	struct nvme_zone_report *r = report;
	struct nvme_zns_desc *desc;
	int i, verbose = flags & VERBOSE;
	__u64 nr_zones = le64_to_cpu(r->nr_zones);

	if (nr_zones < descs)
		descs = nr_zones;

	if (flags & BINARY)
		return d_raw((unsigned char *)report, report_size);
	else if (flags & JSON)
		return json_nvme_zns_report_zones(report, descs, 
				ext_size, report_size, nr_zones);

	for (i = 0; i < descs; i++) {
		desc = (struct nvme_zns_desc *)
			(report + sizeof(*r) + i * (sizeof(*desc) + ext_size));
		if(verbose) {
			printf("SLBA: %#-10"PRIx64" WP: %#-10"PRIx64" Cap: %#-10"PRIx64" State: %-12s Type: %-14s\n",
				(uint64_t)le64_to_cpu(desc->zslba), (uint64_t)le64_to_cpu(desc->wp),
				(uint64_t)le64_to_cpu(desc->zcap), zone_state_to_string(desc->zs >> 4),
				zone_type_to_string(desc->zt));
			nvme_show_zns_report_zone_attributes(desc->za, desc->zai);
		}
		else {
			printf("SLBA: %#-10"PRIx64" WP: %#-10"PRIx64" Cap: %#-10"PRIx64" State: %#-4x Type: %#-4x Attrs: %#-4x AttrsInfo: %#-4x\n",
				(uint64_t)le64_to_cpu(desc->zslba), (uint64_t)le64_to_cpu(desc->wp),
				(uint64_t)le64_to_cpu(desc->zcap), desc->zs, desc->zt,
				desc->za, desc->zai);
		}

		if (ext_size && (desc->za & NVME_ZNS_ZA_ZDEV)) {
			printf("Extension Data: ");
			d((unsigned char *)desc + sizeof(*desc), ext_size, 16, 1);
			printf("..\n");
		}
	}
}*/

/*ZNS */
int report_zones(int argc, char **argv, struct command *cmd, struct plugin *plugin)
//static int report_zones(int argc, char **argv, struct command *cmd, struct plugin *plugin)
{
	const char *desc = "Retrieve the Report Zones data structure";
	const char *zslba = "starting LBA of the zone";
	const char *num_descs = "number of descriptors to retrieve (default: all of them)";
	const char *state = "state of zones to list";
	const char *ext = "set to use the extended report zones";
	const char *part = "set to use the partial report";
	const char *verbose = "show report zones verbosity";
	//char* dev = argv[0];
	char * dev="/dev/nvme0n1";
	enum nvme_print_flags flags;
	int fd, zdes = 0, err = -1;
	__u32 report_size;
	void *report;
	bool huge = false;
	struct nvme_zone_report *buff;

	unsigned int nr_zones_chunks = 1024,   /* 1024 entries * 64 bytes per entry = 64k byte transfer */
			nr_zones_retrieved = 0,
			nr_zones,
			offset,
			log_len;
	int total_nr_zones = 0;
	struct nvme_zns_id_ns id_zns;
	struct nvme_id_ns id_ns;
	uint8_t lbaf;
	__le64	zsze;

	struct config {
		char *output_format;
		__u64 zslba;
		__u32 namespace_id;
		int   num_descs;
		int   state;
		int   verbose;
		bool  extended;
		bool  partial;
	};

	struct config cfg = {
		.output_format = "normal",
		.num_descs = -1,
	};
	
	OPT_ARGS(opts) = {
		OPT_UINT("namespace-id",  'n', &cfg.namespace_id,   namespace_id),
		OPT_SUFFIX("start-lba",   's', &cfg.zslba,          zslba),
		OPT_UINT("descs",         'd', &cfg.num_descs,      num_descs),
		OPT_UINT("state",         'S', &cfg.state,          state),
		//OPT_FMT("output-format",  'o', &cfg.output_format,  output_format),
		OPT_FLAG("verbose",       'v', &cfg.verbose,        verbose),
		OPT_FLAG("extended",      'e', &cfg.extended,       ext),
		OPT_FLAG("partial",       'p', &cfg.partial,        part),
		OPT_END()
	};
	//fd = parse_and_open(argc, argv, desc, opts);
	fd = open(dev, O_RDWR);
	if (fd < 0)
		return errno;
	/*
	flags = validate_output_format(cfg.output_format);
	if (flags < 0)
		goto close_fd;
	if (cfg.verbose)
		flags |= VERBOSE;

	if (!cfg.namespace_id) {
	*/
	err = cfg.namespace_id = nvme_get_nsid(fd);
	if (err < 0) {
		perror("get-namespace-id");
		goto close_fd;
	}
	cfg.extended = false;
	/*
	extended = 0
	if (cfg.extended) {
		zdes = get_zdes_bytes(fd, cfg.namespace_id);
		if (zdes < 0) {
			err = zdes;
			goto close_fd;
		}
	}
	*/

	err = nvme_identify_ns(fd, cfg.namespace_id, false, &id_ns);
	if (err) {
		nvme_show_status(err);
		goto close_fd;
	}

	err = nvme_zns_identify_ns(fd, cfg.namespace_id, &id_zns);
	if (!err) {
		/* get zsze field from zns id ns data - needed for offset calculation */
		lbaf = id_ns.flbas & NVME_NS_FLBAS_LBA_MASK;
	    zsze = le64_to_cpu(id_zns.lbafe[lbaf].zsze);
	}
	else {
		nvme_show_status(err);
		goto close_fd;
	}

	log_len = sizeof(struct nvme_zone_report);
	buff = (struct nvme_zone_report * )calloc(1, log_len);
	if (!buff) {
		err = -ENOMEM;
		goto close_fd;
	}

	err = nvme_zns_report_zones(fd, cfg.namespace_id, 0, 0, (nvme_zns_report_options)cfg.state, 0, log_len, buff);
	if (err > 0) {
		nvme_show_status(err);
		goto free_buff;
	}
	else if (err < 0) {
		perror("zns report-zones");
		goto free_buff;
	}

	total_nr_zones = le64_to_cpu(buff->nr_zones);

	if (cfg.num_descs == -1) {
		cfg.num_descs = total_nr_zones;
	}

	nr_zones = cfg.num_descs;
	if (nr_zones < nr_zones_chunks)
		nr_zones_chunks = nr_zones;

	log_len = sizeof(struct nvme_zone_report) + ((sizeof(struct nvme_zns_desc) * nr_zones_chunks) + (nr_zones_chunks * zdes));
	report_size = log_len;

	report = nvme_alloc(report_size, &huge);
	if (!report) {
		perror("alloc");
		err = -ENOMEM;
		goto close_fd;
	}

	offset = cfg.zslba;
	printf("nr_zones: %"PRIu64"\n", (uint64_t)le64_to_cpu(total_nr_zones));

	while (nr_zones_retrieved < nr_zones) {
		if (nr_zones_retrieved >= nr_zones)
			break;

		if (nr_zones_retrieved + nr_zones_chunks > nr_zones) {
			nr_zones_chunks = nr_zones - nr_zones_retrieved;
			log_len = sizeof(struct nvme_zone_report) + ((sizeof(struct nvme_zns_desc) * nr_zones_chunks) + (nr_zones_chunks * zdes));
		}

		err = nvme_zns_report_zones(fd, cfg.namespace_id, offset,
			cfg.extended, ( nvme_zns_report_options)cfg.state, cfg.partial, log_len, report);
		if (err > 0) {
			nvme_show_status(err);
			break;
		}

		if (!err){
			nvme_show_zns_report_zones(report, nr_zones_chunks, zdes, log_len, flags);
		}
			 //nvme_show_zns_report_zones(report, nr_zones_chunks, zdes, log_len, flags);

		nr_zones_retrieved += nr_zones_chunks;
		offset = (nr_zones_retrieved * zsze);
    }
	std::cout << "zsze" << zsze << std::endl;
	nvme_free(report, huge);

free_buff:
	free(buff);
close_fd:
	close(fd);
	return nvme_status_to_errno(err, false);
}


//ZNS end
int * zns_init_print(struct zns_share_info * zonelist)
{
    int i;
    struct nvme_zone_info_entry temp_zone_info_entry;
    printf("ZNS SSD Infos\n");
    printf("File Descriptor Number : %d\n", zonelist->fd);
	printf("Zone Size\t:\t %lu \n", zonelist->zonesize);
	printf("Active Zones\t:\t%#"PRIx64"\n", zonelist->activezones);
	printf("Open Zones\t:\t%#"PRIx64"\n", zonelist->openzones);
	printf("Total Zones\t:\t%#"PRIx64"\n", zonelist->totalzones);
    
    printf("\nZNS SSD Zone Entry Infos\n");
    for(i=0; i<zonelist->totalzones; i++)
    {
        temp_zone_info_entry = zonelist->zone_list->zone_entry;
            printf("Zone %d { Condition : %#"PRIx64",Capacity : %#"PRIx64", SLBA : %#"PRIx64", WP : %#"PRIx64", WriteCnt : %#"PRIx64", ReadCnt : %#"PRIx64", ResetCnt : %#"PRIx64" } \n",
                                i,
                                le64_to_cpu((zonelist->zone_list)[i].zone_entry.zone_condition),
                                le64_to_cpu((zonelist->zone_list)[i].zone_entry.zone_capacity),
                                le64_to_cpu((zonelist->zone_list)[i].zone_entry.zone_start_lba),
                                le64_to_cpu((zonelist->zone_list)[i].zone_entry.write_pointer),
                                le64_to_cpu((zonelist->zone_list)[i].zone_entry.cnt_write),
                                le64_to_cpu((zonelist->zone_list)[i].zone_entry.cnt_read),
                                le64_to_cpu((zonelist->zone_list)[i].zone_entry.cnt_reset));
    }
    
}

__u64 get_zone_to_slba(struct zns_share_info * zonelist, int zonenumber)
{
    return (zonelist->zone_list)[zonenumber].zone_entry.zone_start_lba;
}

void * zns_info_ctrl(int fd, void * data)
{	
    int result;
    struct nvme_passthru_cmd cmd = {
		.opcode		= 0x06,
		.flags		= 0,
		.rsvd1		= 0,
		.nsid		= 0,
		.cdw2		= 0,
		.cdw3		= 0,
		.metadata	= (__u64)(uintptr_t) 0,
		.addr		= (__u64)(uintptr_t) data,
		.metadata_len	= 0,
		.data_len	= 4096,
		.cdw10		= 1,
		.cdw11		= 0,
		.cdw12		= 0,
		.cdw13		= 0,
		.cdw14		= 0,
		.cdw15		= 0,
		.timeout_ms	= 0,
		.result		= 0,
	};

    result = ioctl(fd, NVME_IOCTL_ADMIN_CMD, &cmd);

    if(result == -1)
    {
        printf("ZNS SSD Ctrl Info Request Fail\n");
        return NULL;
    }

    return data;
}

void * zns_info_ctrl_print(void * data)
{
    struct nvme_id_ctrl * id_ctrl = (struct nvme_id_ctrl *) data;

    printf("vid\t:\t%#"PRIx64"\n", le64_to_cpu(id_ctrl->vid));
	printf("ssvid\t:\t%#"PRIx64"\n", le64_to_cpu(id_ctrl->ssvid));
	printf("oncs\t:\t%#"PRIx64"\n", le64_to_cpu(id_ctrl->oncs));
}

void * zns_info_ns(int fd, void * data)
{	
    int result;
    struct nvme_passthru_cmd cmd = {
		.opcode		= 0x06,
		.flags		= 0,
		.rsvd1		= 0,
		.nsid		= 1,
		.cdw2		= 0,
		.cdw3		= 0,
		.metadata	= (__u64)(uintptr_t) 0,
		.addr		= (__u64)(uintptr_t) data,
		.metadata_len	= 0,
		.data_len	= 4096,
		.cdw10		= 0,
		.cdw11		= 0,
		.cdw12		= 0,
		.cdw13		= 0,
		.cdw14		= 0,
		.cdw15		= 0,
		.timeout_ms	= 0,
		.result		= 0,
	};

    result = ioctl(fd, NVME_IOCTL_ADMIN_CMD, &cmd);

    if(result == -1)
    {
        printf("ZNS SSD Ctrl Info Request Fail\n");
    }
        return NULL;

    return data;
}

/*
void * zns_info_ns_print(void * data)
{
    struct nvme_id_ns * id_ns = (struct nvme_id_ns *)data;

    printf("nsze\t\t:\t%#"PRIx64"\n", le64_to_cpu(id_ns->nsze));
	printf("Zone Size\t:\t%#"PRIx64"\n", id_ns->zonesize);
	printf("activezones\t:\t%#"PRIx64"\n", id_ns->activezones);
	printf("openzones\t:\t%#"PRIx64"\n", id_ns->openzones);
	printf("totalzones\t:\t%#"PRIx64"\n", id_ns->totalzones);
}*/

int zns_write_request(void * write_data, __le16 nblocks, __le32 data_size, __u64 slba)
{
    int result;

    struct nvme_user_io io= {
			.opcode		= 0x01,
			.flags		= 0,
			.control	= 0x0400,
			.nblocks	= nblocks,
			.rsvd		= 0,
			.metadata	= (__u64)(uintptr_t) 0,
			.addr		= (__u64)(uintptr_t) write_data,
			.slba		= slba,
			.dsmgmt		= 0,
			.reftag		= 0,
			.apptag		= 0,
			.appmask	= 0,
        };

    result = ioctl(femu_zns_info -> fd, NVME_IOCTL_SUBMIT_IO, &io);
    if(result != 0)
    {
        printf("ZNS SSD Write Fail, error code : %d\n", result);
        return result;
    }

    return 0;
}

int zns_write(void * write_data, int data_size, int zone_number)
{
	int i;
	int result;
	__le16 nblocks;


	if(data_size % BLOCK_SIZE == 0)
		nblocks = (data_size / BLOCK_SIZE) - 1;
	else
		nblocks = (data_size / BLOCK_SIZE);

	result = zns_write_request(write_data, nblocks, data_size, femu_zone_desc_list[zone_number].wp);
	femu_zone_desc_list[zone_number].wp += nblocks + 1;

    return result;
}
int zns_write_interzone(void * write_data, int data_size, int zone_number, int interleave_zonenumber){
	// if state[zone_number] is not finish then write
	// else zone_number += interleave then write
	if ((zone_number + inter_zone_counter) % interleave_zonenumber != 0){
		return zns_write(write_data, data_size,zone_number + inter_zone_counter);
		
	}else{
		return zns_write(write_data, data_size,zone_number);
	}
}

int zns_read_request(void * read_data, int nblocks, __u64 slba)
{
    int result;

    struct nvme_user_io io = {
            .opcode		= 0x02,
            .flags		= 0,
            .control	= 0,
            .nblocks	= nblocks,
            .rsvd		= 0,
            .metadata	= 0,
            .addr		= (__u64)(uintptr_t) read_data,
            .slba		= slba,
            .dsmgmt		= 0,
            .reftag		= 0,
            .apptag		= 0,
            .appmask	= 0,
        };

    result = ioctl(femu_zns_info->fd, NVME_IOCTL_SUBMIT_IO, &io);
    if(result == -1)
    {
        printf("ZNS SSD Read Fail : %#x\n");
        return -1;
    }

    return 0;
}

int zns_read(void * read_data, int data_size, int zone_number, __u64 offset)
{
	int i;
	int result;
	int nblocks;
	__u64 read_lba;

    if(data_size % BLOCK_SIZE == 0)
        nblocks = data_size / 512 - 1;
    else
        nblocks = data_size / 512;
    
    read_lba = femu_zone_desc_list[zone_number].zslba + offset;
    result = zns_read_request(read_data, nblocks, read_lba);
    //zns_update_zone_info(zonelist, zone_number);

    return result;
}
int zns_management_send(int zone_number, __u8 value)
{
	int result;
	__le64 slba = 0;
	__u32 cdw13 = 0;

	cdw13 = cdw13 | value;

	if(zone_number < 0)
		//Select All 
		cdw13 = cdw13 | (1 << 8);
	else
	{
		slba = zone_number * (femu_zns_info -> zonef.zsze);

		if(value == MAN_OPEN && femu_zns_info -> opened_zone_num + 1 >= femu_zns_info -> max_open_res)
		{
			printf("Opened zone num : %d\n", femu_zns_info -> opened_zone_num);
			printf("ZNS SSD Zone Management Send ZONE MAX OPEN\n");
			return -1;
		}
	}
	__u64 t = 0xffffffff;
	__le32 slba1 = slba & t;
	__le32 slba2 = slba >> 32;

	struct nvme_passthru_cmd cmd = {
		.opcode		= 0x79,
		.flags		= 0,
		.rsvd1		= 0,
		.nsid		= 1,
		.cdw2		= 0,
		.cdw3		= 0,
		.metadata	= (__u64)(uintptr_t) 0,
		.addr		= (__u64)(uintptr_t) 0,
		.metadata_len	= 0,
		.data_len	= 0,
		.cdw10		= slba1,
		.cdw11		= slba2,
		.cdw12		= 0,
		.cdw13		= cdw13,
		.cdw14		= 0,
		.cdw15		= 0,
		.timeout_ms	= 0,
		.result		= 0,
	};

	result = ioctl(femu_zns_info -> fd, NVME_IOCTL_IO_CMD, &cmd);

	if(result == -1)
	{
		printf("ZNS SSD Zone Management Send Fail\n");
		return -1;
	}

	if(value == MAN_OPEN)
		femu_zns_info -> opened_zone_num += 1;
	else if(value == MAN_CLOSE)
		femu_zns_info -> opened_zone_num -= 1;

	return 0;
}
void zns_set_zone(int zone_number, __u8 value)
{
	zns_management_send(zone_number, value);
}

int zns_get_log_entry_info(int fd, void * data, __u64 zid)
{
    __u32 data_len = 64;
    int result;
		//.opcode = 0x02,
	//CONF ZNS SETTING : opcode 0xec
    struct nvme_admin_cmd cmd = {
		.opcode = 0xec,
		.nsid = 1,
		.addr = (__u64)(uintptr_t) data,
		.data_len = data_len,
	};

    __u32 numd = (data_len >> 2) - 1;
	__u16 numdu = numd >> 16, numdl = numd & 0xffff;
	__u8 log_id = 0x82;
	__u8 lsp = 0;
	__u64 lpo = (zid + 1) * 64;
	bool rae = 0;
	__u16 lsi = 0;

	cmd.cdw10 = log_id | (numdl << 16) | (rae ? 1 << 15 : 0);
	if (lsp)
        cmd.cdw10 |= lsp << 8;
	cmd.cdw11 = numdu | (lsi << 16);
	cmd.cdw12 = lpo;
	cmd.cdw13 = (lpo >> 32);

    result = ioctl(fd, NVME_IOCTL_ADMIN_CMD, &cmd);
    if(result == -1)
    {
        printf("Get log entry fail\n");
        return -1;
    }

    return 0;
}

int zns_get_total_log_entry_info(int fd, int from, int nzones)
{
    int i;
    void * data = malloc(64);

    for(i = from; i < from + nzones; i++)
    {
        memset(data, 0, 64);
        zns_get_log_entry_info(fd, data, i);
        zns_log_info_entry_print(i, data);
    }

    return 0;
}

int zns_log_info_entry_print(int num, void * data)
{   
    struct nvme_zone_info_entry * zone_entry = (struct nvme_zone_info_entry *) data;
   	struct nvme_zns_desc *desc;

	printf("Zone %d { Condition : %d, Capacity : %lld, SLBA : %lld, WP : %lld, WriteCnt : %lld, ReadCnt : %lld, ResetCnt : %d } \n",
                                num,
                                zone_entry->zone_condition,
                                zone_entry->zone_capacity,
                                zone_entry->zone_start_lba,
                                zone_entry->write_pointer,
                                zone_entry->cnt_write,
                                zone_entry->cnt_read,
                                zone_entry->cnt_reset);
    /*
	printf("Zone %d { Condition : %#"PRIx64", Capacity : %#"PRIx64", SLBA : %#"PRIx64", WP : %#"PRIx64", WriteCnt : %#"PRIx64", ReadCnt : %#"PRIx64", ResetCnt : %#"PRIx64" } \n",
                                num,
                                le64_to_cpu(zone_entry->zone_condition),
                                le64_to_cpu(zone_entry->zone_capacity),
                                le64_to_cpu(zone_entry->zone_start_lba),
                                le64_to_cpu(zone_entry->write_pointer),
                                le64_to_cpu(zone_entry->cnt_write),
                                le64_to_cpu(zone_entry->cnt_read),
                                le64_to_cpu(zone_entry->cnt_reset));
	*/
    return 0;
}

int zns_zone_io_managemnet(int fd, __u64 slba, __u64 action)
{
    int result;
    __u64 t = 0xffffffff;

    struct nvme_passthru_cmd cmd = {
  		.opcode = 0x20,
		.nsid = 1,
    };
    slba |= action << 61;
    cmd.cdw10 = slba & t;
    cmd.cdw11 = slba >> 32;

    result = ioctl(fd, NVME_IOCTL_IO_CMD, &cmd);
    
    if(result == -1)
    {
        printf("Get finish fail\n");
        return -1;
    }

    return 0;
}

int zns_update_zone_info(struct zns_share_info * zonelist, int zonenumber)
{
    void * temp_log_data = malloc(LOG_SIZE);
    struct nvme_zone_info_entry * zone_entrys;

    zns_get_log_entry_info(zonelist->fd, temp_log_data, zonenumber);
    struct nvme_zone_info_entry  * temp_zone_info_entry = (struct nvme_zone_info_entry  *)temp_log_data;
    (zonelist->zone_list)[zonenumber].zone_entry.zone_condition = temp_zone_info_entry->zone_condition;	//zone state
    (zonelist->zone_list)[zonenumber].zone_entry.zone_capacity = temp_zone_info_entry->zone_capacity;	//zone cap
    (zonelist->zone_list)[zonenumber].zone_entry.write_pointer = temp_zone_info_entry->write_pointer;	//zone wrpt
    (zonelist->zone_list)[zonenumber].zone_entry.zone_start_lba = temp_zone_info_entry->zone_start_lba;	//zone slba
    (zonelist->zone_list)[zonenumber].zone_entry.cnt_read = temp_zone_info_entry->cnt_read;				//cnt_read
    (zonelist->zone_list)[zonenumber].zone_entry.cnt_write = temp_zone_info_entry->cnt_write;			//cnt_write
    (zonelist->zone_list)[zonenumber].zone_entry.cnt_reset = temp_zone_info_entry->cnt_reset;			//cnt_reset

    free(temp_log_data);
    
    return 0;
}

int zns_zone_finish_request(int fd, __u64 slba)
{
    return zns_zone_io_managemnet(fd, slba, 0x02);
}

int zns_zone_open_request(int fd, __u64 slba)
{
    return zns_zone_io_managemnet(fd, slba, 0x03);
}

int zns_zone_reset_request(int fd, __u64 slba)
{
    return zns_zone_io_managemnet(fd, slba, 0x04);
}

int zns_zone_finish(struct zns_share_info * zonelist, int zonenumber)
{
    int result;
    result = zns_zone_finish_request(zonelist->fd, get_zone_to_slba(zonelist, zonenumber));
    zns_update_zone_info(zonelist, zonenumber);
    return result;
}

int zns_zone_open(struct zns_share_info * zonelist, int zonenumber)
{
    int result;
    result = zns_zone_open_request(zonelist->fd, get_zone_to_slba(zonelist, zonenumber));
    zns_update_zone_info(zonelist, zonenumber);
    return result;
}

int zns_zone_reset(struct zns_share_info * zonelist, int zonenumber)
{
    int result;
    result = zns_zone_reset_request(zonelist->fd, get_zone_to_slba(zonelist, zonenumber));
    zns_update_zone_info(zonelist, zonenumber);
    return result;
}


const char *nvme_status_to_string(__u16 status)
{
	switch (status & 0x7ff) {
	case NVME_SC_SUCCESS:
		return "SUCCESS: The command completed successfully";
	case NVME_SC_INVALID_OPCODE:
		return "INVALID_OPCODE: The associated command opcode field is not valid";
	case NVME_SC_INVALID_FIELD:
		return "INVALID_FIELD: A reserved coded value or an unsupported value in a defined field";
	case NVME_SC_CMDID_CONFLICT:
		return "CMDID_CONFLICT: The command identifier is already in use";
	case NVME_SC_DATA_XFER_ERROR:
		return "DATA_XFER_ERROR: Error while trying to transfer the data or metadata";
	case NVME_SC_POWER_LOSS:
		return "POWER_LOSS: Command aborted due to power loss notification";
	case NVME_SC_INTERNAL:
		return "INTERNAL: The command was not completed successfully due to an internal error";
	case NVME_SC_ABORT_REQ:
		return "ABORT_REQ: The command was aborted due to a Command Abort request";
	case NVME_SC_ABORT_QUEUE:
		return "ABORT_QUEUE: The command was aborted due to a Delete I/O Submission Queue request";
	case NVME_SC_FUSED_FAIL:
		return "FUSED_FAIL: The command was aborted due to the other command in a fused operation failing";
	case NVME_SC_FUSED_MISSING:
		return "FUSED_MISSING: The command was aborted due to a Missing Fused Command";
	case NVME_SC_INVALID_NS:
		return "INVALID_NS: The namespace or the format of that namespace is invalid";
	case NVME_SC_CMD_SEQ_ERROR:
		return "CMD_SEQ_ERROR: The command was aborted due to a protocol violation in a multicommand sequence";
	case NVME_SC_SGL_INVALID_LAST:
		return "SGL_INVALID_LAST: The command includes an invalid SGL Last Segment or SGL Segment descriptor.";
	case NVME_SC_SGL_INVALID_COUNT:
		return "SGL_INVALID_COUNT: There is an SGL Last Segment descriptor or an SGL Segment descriptor in a location other than the last descriptor of a segment based on the length indicated.";
	case NVME_SC_SGL_INVALID_DATA:
		return "SGL_INVALID_DATA: This may occur if the length of a Data SGL is too short.";
	case NVME_SC_SGL_INVALID_METADATA:
		return "SGL_INVALID_METADATA: This may occur if the length of a Metadata SGL is too short";
	case NVME_SC_SGL_INVALID_TYPE:
		return "SGL_INVALID_TYPE: The type of an SGL Descriptor is a type that is not supported by the controller.";
	case NVME_SC_CMB_INVALID_USE:
		return "CMB_INVALID_USE: The attempted use of the Controller Memory Buffer is not supported by the controller.";
	case NVME_SC_PRP_INVALID_OFFSET:
		return "PRP_INVALID_OFFSET: The Offset field for a PRP entry is invalid.";
	case NVME_SC_ATOMIC_WRITE_UNIT_EXCEEDED:
		return "ATOMIC_WRITE_UNIT_EXCEEDED: The length specified exceeds the atomic write unit size.";
	case NVME_SC_OPERATION_DENIED:
		return "OPERATION_DENIED: The command was denied due to lack of access rights.";
	case NVME_SC_SGL_INVALID_OFFSET:
		return "SGL_INVALID_OFFSET: The offset specified in a descriptor is invalid.";
	case NVME_SC_INCONSISTENT_HOST_ID:
		return "INCONSISTENT_HOST_ID: The NVM subsystem detected the simultaneous use of 64-bit and 128-bit Host Identifier values on different controllers.";
	case NVME_SC_KEEP_ALIVE_EXPIRED:
		return "KEEP_ALIVE_EXPIRED: The Keep Alive Timer expired.";
	case NVME_SC_KEEP_ALIVE_INVALID:
		return "KEEP_ALIVE_INVALID: The Keep Alive Timeout value specified is invalid.";
	case NVME_SC_PREEMPT_ABORT:
		return "PREEMPT_ABORT: The command was aborted due to a Reservation Acquire command with the Reservation Acquire Action (RACQA) set to 010b (Preempt and Abort).";
	case NVME_SC_SANITIZE_FAILED:
		return "SANITIZE_FAILED: The most recent sanitize operation failed and no recovery actions has been successfully completed";
	case NVME_SC_SANITIZE_IN_PROGRESS:
		return "SANITIZE_IN_PROGRESS: The requested function is prohibited while a sanitize operation is in progress";
	case NVME_SC_SGL_DATA_BLK_GRAN_INVALID:
		return "NVME_SC_SGL_DATA_BLK_GRAN_INVALID: Address alignment or Length granularity for an SGL Data Block descriptor is invalid";
	case NVME_SC_CMD_NOT_SUP_QUEUE_IN_CMB:
		return "NVME_SC_CMD_NOT_SUP_QUEUE_IN_CMB: does not support submission of the command to a SQ or completion to a CQ in the CMB";
	case NVME_SC_IOCS_NOT_SUPPORTED:
		return "IOCS_NOT_SUPPORTED: The I/O command set is not supported";
	case NVME_SC_IOCS_NOT_ENABLED:
		return "IOCS_NOT_ENABLED: The I/O command set is not enabled";
	case NVME_SC_IOCS_COMBINATION_REJECTED:
		return "IOCS_COMBINATION_REJECTED: The I/O command set combination is rejected";
	case NVME_SC_INVALID_IOCS:
		return "INVALID_IOCS: the I/O command set is invalid";
	case NVME_SC_ID_UNAVAILABLE:
		return "NVME_SC_ID_UNAVAILABLE: The number of Endurance Groups or NVM Sets supported has been exceeded.";
	case NVME_SC_LBA_RANGE:
		return "LBA_RANGE: The command references a LBA that exceeds the size of the namespace";
	case NVME_SC_NS_WRITE_PROTECTED:
		return "NS_WRITE_PROTECTED: The command is prohibited while the namespace is write protected by the host.";
	case NVME_SC_TRANSIENT_TRANSPORT:
		return "TRANSIENT_TRANSPORT: A transient transport error was detected.";
	case NVME_SC_PROHIBITED_BY_CMD_AND_FEAT:
		return "NVME_SC_PROHIBITED_BY_CMD_AND_FEAT: command was aborted due to execution being prohibited by the Command and Feature Lockdown";
	case NVME_SC_ADMIN_CMD_MEDIA_NOT_READY:
		return "NVME_SC_ADMIN_CMD_MEDIA_NOT_READY: Admin command requires access to media and the media is not ready";
	case NVME_SC_CAP_EXCEEDED:
		return "CAP_EXCEEDED: The execution of the command has caused the capacity of the namespace to be exceeded";
	case NVME_SC_NS_NOT_READY:
		return "NS_NOT_READY: The namespace is not ready to be accessed as a result of a condition other than a condition that is reported as an Asymmetric Namespace Access condition";
	case NVME_SC_RESERVATION_CONFLICT:
		return "RESERVATION_CONFLICT: The command was aborted due to a conflict with a reservation held on the accessed namespace";
	case NVME_SC_FORMAT_IN_PROGRESS:
		return "FORMAT_IN_PROGRESS: A Format NVM command is in progress on the namespace.";
	case NVME_SC_ZONE_BOUNDARY_ERROR:
		return "ZONE_BOUNDARY_ERROR: Invalid Zone Boundary crossing";
	case NVME_SC_ZONE_IS_FULL:
		return "ZONE_IS_FULL: The accessed zone is in ZSF:Full state";
	case NVME_SC_ZONE_IS_READ_ONLY:
		return "ZONE_IS_READ_ONLY: The accessed zone is in ZSRO:Read Only state";
	case NVME_SC_ZONE_IS_OFFLINE:
		return "ZONE_IS_OFFLINE: The access zone is in ZSO:Offline state";
	case NVME_SC_ZONE_INVALID_WRITE:
		return "ZONE_INVALID_WRITE: The write to zone was not at the write pointer offset";
	case NVME_SC_TOO_MANY_ACTIVE_ZONES:
		return "TOO_MANY_ACTIVE_ZONES: The controller does not allow additional active zones";
	case NVME_SC_TOO_MANY_OPEN_ZONES:
		return "TOO_MANY_OPEN_ZONES: The controller does not allow additional open zones";
	case NVME_SC_ZONE_INVALID_STATE_TRANSITION:
		return "INVALID_ZONE_STATE_TRANSITION: The zone state change was invalid";
	case NVME_SC_CQ_INVALID:
		return "CQ_INVALID: The Completion Queue identifier specified in the command does not exist";
	case NVME_SC_QID_INVALID:
		return "QID_INVALID: The creation of the I/O Completion Queue failed due to an invalid queue identifier specified as part of the command. An invalid queue identifier is one that is currently in use or one that is outside the range supported by the controller";
	case NVME_SC_QUEUE_SIZE:
		return "QUEUE_SIZE: The host attempted to create an I/O Completion Queue with an invalid number of entries";
	case NVME_SC_ABORT_LIMIT:
		return "ABORT_LIMIT: The number of concurrently outstanding Abort commands has exceeded the limit indicated in the Identify Controller data structure";
	case NVME_SC_ABORT_MISSING:
		return "ABORT_MISSING: The abort command is missing";
	case NVME_SC_ASYNC_LIMIT:
		return "ASYNC_LIMIT: The number of concurrently outstanding Asynchronous Event Request commands has been exceeded";
	case NVME_SC_FIRMWARE_SLOT:
		return "FIRMWARE_SLOT: The firmware slot indicated is invalid or read only. This error is indicated if the firmware slot exceeds the number supported";
	case NVME_SC_FIRMWARE_IMAGE:
		return "FIRMWARE_IMAGE: The firmware image specified for activation is invalid and not loaded by the controller";
	case NVME_SC_INVALID_VECTOR:
		return "INVALID_VECTOR: The creation of the I/O Completion Queue failed due to an invalid interrupt vector specified as part of the command";
	case NVME_SC_INVALID_LOG_PAGE:
		return "INVALID_LOG_PAGE: The log page indicated is invalid. This error condition is also returned if a reserved log page is requested";
	case NVME_SC_INVALID_FORMAT:
		return "INVALID_FORMAT: The LBA Format specified is not supported. This may be due to various conditions";
	case NVME_SC_FW_NEEDS_CONV_RESET:
		return "FW_NEEDS_CONVENTIONAL_RESET: The firmware commit was successful, however, activation of the firmware image requires a conventional reset";
	case NVME_SC_INVALID_QUEUE:
		return "INVALID_QUEUE: This error indicates that it is invalid to delete the I/O Completion Queue specified. The typical reason for this error condition is that there is an associated I/O Submission Queue that has not been deleted.";
	case NVME_SC_FEATURE_NOT_SAVEABLE:
		return "FEATURE_NOT_SAVEABLE: The Feature Identifier specified does not support a saveable value";
	case NVME_SC_FEATURE_NOT_CHANGEABLE:
		return "FEATURE_NOT_CHANGEABLE: The Feature Identifier is not able to be changed";
	case NVME_SC_FEATURE_NOT_PER_NS:
		return "FEATURE_NOT_PER_NS: The Feature Identifier specified is not namespace specific. The Feature Identifier settings apply across all namespaces";
	case NVME_SC_FW_NEEDS_SUBSYS_RESET:
		return "FW_NEEDS_SUBSYSTEM_RESET: The firmware commit was successful, however, activation of the firmware image requires an NVM Subsystem";
	case NVME_SC_FW_NEEDS_RESET:
		return "FW_NEEDS_RESET: The firmware commit was successful; however, the image specified does not support being activated without a reset";
	case NVME_SC_FW_NEEDS_MAX_TIME:
		return "FW_NEEDS_MAX_TIME_VIOLATION: The image specified if activated immediately would exceed the Maximum Time for Firmware Activation (MTFA) value reported in Identify Controller. To activate the firmware, the Firmware Commit command needs to be re-issued and the image activated using a reset";
	case NVME_SC_FW_ACTIVATE_PROHIBITED:
		return "FW_ACTIVATION_PROHIBITED: The image specified is being prohibited from activation by the controller for vendor specific reasons";
	case NVME_SC_OVERLAPPING_RANGE:
		return "OVERLAPPING_RANGE: This error is indicated if the firmware image has overlapping ranges";
	case NVME_SC_NS_INSUFFICIENT_CAP:
		return "NS_INSUFFICIENT_CAPACITY: Creating the namespace requires more free space than is currently available. The Command Specific Information field of the Error Information Log specifies the total amount of NVM capacity required to create the namespace in bytes";
	case NVME_SC_NS_ID_UNAVAILABLE:
		return "NS_ID_UNAVAILABLE: The number of namespaces supported has been exceeded";
	case NVME_SC_NS_ALREADY_ATTACHED:
		return "NS_ALREADY_ATTACHED: The controller is already attached to the namespace specified";
	case NVME_SC_NS_IS_PRIVATE:
		return "NS_IS_PRIVATE: The namespace is private and is already attached to one controller";
	case NVME_SC_NS_NOT_ATTACHED:
		return "NS_NOT_ATTACHED: The request to detach the controller could not be completed because the controller is not attached to the namespace";
	case NVME_SC_THIN_PROV_NOT_SUPP:
		return "THIN_PROVISIONING_NOT_SUPPORTED: Thin provisioning is not supported by the controller";
	case NVME_SC_CTRL_LIST_INVALID:
		return "CONTROLLER_LIST_INVALID: The controller list provided is invalid";
	case NVME_SC_DEVICE_SELF_TEST_IN_PROGRESS:
		return "DEVICE_SELF_TEST_IN_PROGRESS: The controller or NVM subsystem already has a device self-test operation in process.";
	case NVME_SC_BP_WRITE_PROHIBITED:
		return "BOOT PARTITION WRITE PROHIBITED: The command is trying to modify a Boot Partition while it is locked";
	case NVME_SC_INVALID_CTRL_ID:
		return "INVALID_CTRL_ID: An invalid Controller Identifier was specified.";
	case NVME_SC_INVALID_SECONDARY_CTRL_STATE:
		return "INVALID_SECONDARY_CTRL_STATE: The action requested for the secondary controller is invalid based on the current state of the secondary controller and its primary controller.";
	case NVME_SC_INVALID_NUM_CTRL_RESOURCE:
		return "INVALID_NUM_CTRL_RESOURCE: The specified number of Flexible Resources is invalid";
	case NVME_SC_INVALID_RESOURCE_ID:
		return "INVALID_RESOURCE_ID: At least one of the specified resource identifiers was invalid";
	case NVME_SC_ANA_INVALID_GROUP_ID:
		return "ANA_INVALID_GROUP_ID: The specified ANA Group Identifier (ANAGRPID) is not supported in the submitted command.";
	case NVME_SC_ANA_ATTACH_FAIL:
		return "ANA_ATTACH_FAIL: The controller is not attached to the namespace as a result of an ANA condition";
	case NVME_SC_INSUFFICIENT_CAP:
		return "NVME_SC_INSUFFICIENT_CAP: Requested operation requires more free space than is currently available";
	case NVME_SC_NS_ATTACHMENT_LIMIT_EXCEEDED:
		return "NVME_SC_NS_ATTACHMENT_LIMIT_EXCEEDED: Attaching the ns to a controller causes max number of ns attachments allowed to be exceeded";
	case NVME_SC_PROHIBIT_CMD_EXEC_NOT_SUPPORTED:
		return "NVME_SC_PROHIBIT_CMD_EXEC_NOT_SUPPORTED: Prohibition of Command Execution Not Supported";
	case NVME_SC_BAD_ATTRIBUTES:
		return "BAD_ATTRIBUTES: Bad attributes were given";
	case NVME_SC_INVALID_PI:
		return "INVALID_PROTECION_INFO: The Protection Information Field settings specified in the command are invalid";
	case NVME_SC_READ_ONLY:
		return "WRITE_ATTEMPT_READ_ONLY_RANGE: The LBA range specified contains read-only blocks";
	case NVME_SC_CMD_SIZE_LIMIT_EXCEEDED:
		return "CMD_SIZE_LIMIT_EXCEEDED: Command size limit exceeded";
	case NVME_SC_WRITE_FAULT:
		return "WRITE_FAULT: The write data could not be committed to the media";
	case NVME_SC_READ_ERROR:
		return "READ_ERROR: The read data could not be recovered from the media";
	case NVME_SC_GUARD_CHECK:
		return "GUARD_CHECK: The command was aborted due to an end-to-end guard check failure";
	case NVME_SC_APPTAG_CHECK:
		return "APPTAG_CHECK: The command was aborted due to an end-to-end application tag check failure";
	case NVME_SC_REFTAG_CHECK:
		return "REFTAG_CHECK: The command was aborted due to an end-to-end reference tag check failure";
	case NVME_SC_COMPARE_FAILED:
		return "COMPARE_FAILED: The command failed due to a miscompare during a Compare command";
	case NVME_SC_ACCESS_DENIED:
		return "ACCESS_DENIED: Access to the namespace and/or LBA range is denied due to lack of access rights";
	case NVME_SC_UNWRITTEN_BLOCK:
		return "UNWRITTEN_BLOCK: The command failed due to an attempt to read from an LBA range containing a deallocated or unwritten logical block";
	case NVME_SC_STORAGE_TAG_CHECK:
		return "NVME_SC_STORAGE_TAG_CHECK: command was aborted due to an end-to-end storage tag check failure";
	case NVME_SC_INTERNAL_PATH_ERROR:
		return "INTERNAL_PATH_ERROT: The command was not completed as the result of a controller internal error";
	case NVME_SC_ANA_PERSISTENT_LOSS:
		return "ASYMMETRIC_NAMESPACE_ACCESS_PERSISTENT_LOSS: The requested function (e.g., command) is not able to be performed as a result of the relationship between the controller and the namespace being in the ANA Persistent Loss state";
	case NVME_SC_ANA_INACCESSIBLE:
		return "ASYMMETRIC_NAMESPACE_ACCESS_INACCESSIBLE: The requested function (e.g., command) is not able to be performed as a result of the relationship between the controller and the namespace being in the ANA Inaccessible state";
	case NVME_SC_ANA_TRANSITION:
		return "ASYMMETRIC_NAMESPACE_ACCESS_TRANSITION: The requested function (e.g., command) is not able to be performed as a result of the relationship between the controller and the namespace transitioning between Asymmetric Namespace Access states";
	case NVME_SC_CTRL_PATHING_ERROR:
		return "CONTROLLER_PATHING_ERROR: A pathing error was detected by the controller";
	case NVME_SC_HOST_PATHING_ERROR:
		return "HOST_PATHING_ERROR: A pathing error was detected by the host";
	case NVME_SC_HOST_CMD_ABORT:
		return "HOST_COMMAND_ABORT: The command was aborted as a result of host action";
	case NVME_SC_CMD_INTERRUPTED:
		return "CMD_INTERRUPTED: Command processing was interrupted and the controller is unable to successfully complete the command. The host should retry the command.";
	case NVME_SC_PMR_SAN_PROHIBITED:
		return "Sanitize Prohibited While Persistent Memory Region is Enabled: A sanitize operation is prohibited while the Persistent Memory Region is enabled.";
	default:
		return "Unknown";
	}
}
