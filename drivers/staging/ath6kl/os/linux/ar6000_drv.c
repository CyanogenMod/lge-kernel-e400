//------------------------------------------------------------------------------
// Copyright (c) 2004-2010 Atheros Communications Inc.
// All rights reserved.
//
// 
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
//
//
// Author(s): ="Atheros"
//------------------------------------------------------------------------------

/*
 * This driver is a pseudo ethernet driver to access the Atheros AR6000
 * WLAN Device
 */

#include "ar6000_drv.h"
#ifdef ATH6K_CONFIG_CFG80211
#include "cfg80211.h"
#endif /* ATH6K_CONFIG_CFG80211 */
#include "htc.h"
#include "wmi_filter_linux.h"
#include "epping_test.h"
#include "wlan_config.h"
#include "ar3kconfig.h"
#include "ar6k_pal.h"
#include "AR6002/addrs.h"


/* LINUX_HACK_FUDGE_FACTOR -- this is used to provide a workaround for linux behavior.  When
 *  the meta data was added to the header it was found that linux did not correctly provide
 *  enough headroom.  However when more headroom was requested beyond what was truly needed
 *  Linux gave the requested headroom. Therefore to get the necessary headroom from Linux
 *  the driver requests more than is needed by the amount = LINUX_HACK_FUDGE_FACTOR */
#define LINUX_HACK_FUDGE_FACTOR 16
#define BDATA_BDADDR_OFFSET     28

A_UINT8 bcast_mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
A_UINT8 null_mac[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

#ifdef DEBUG

#define  ATH_DEBUG_DBG_LOG       ATH_DEBUG_MAKE_MODULE_MASK(0)
#define  ATH_DEBUG_WLAN_CONNECT  ATH_DEBUG_MAKE_MODULE_MASK(1)
#define  ATH_DEBUG_WLAN_SCAN     ATH_DEBUG_MAKE_MODULE_MASK(2)
#define  ATH_DEBUG_WLAN_TX       ATH_DEBUG_MAKE_MODULE_MASK(3)
#define  ATH_DEBUG_WLAN_RX       ATH_DEBUG_MAKE_MODULE_MASK(4)
#define  ATH_DEBUG_HTC_RAW       ATH_DEBUG_MAKE_MODULE_MASK(5)
#define  ATH_DEBUG_HCI_BRIDGE    ATH_DEBUG_MAKE_MODULE_MASK(6)

static ATH_DEBUG_MASK_DESCRIPTION driver_debug_desc[] = {
    { ATH_DEBUG_DBG_LOG      , "Target Debug Logs"},
    { ATH_DEBUG_WLAN_CONNECT , "WLAN connect"},
    { ATH_DEBUG_WLAN_SCAN    , "WLAN scan"},
    { ATH_DEBUG_WLAN_TX      , "WLAN Tx"},
    { ATH_DEBUG_WLAN_RX      , "WLAN Rx"},
    { ATH_DEBUG_HTC_RAW      , "HTC Raw IF tracing"},
    { ATH_DEBUG_HCI_BRIDGE   , "HCI Bridge Setup"},
    { ATH_DEBUG_HCI_RECV     , "HCI Recv tracing"},
    { ATH_DEBUG_HCI_DUMP     , "HCI Packet dumps"},
};

ATH_DEBUG_INSTANTIATE_MODULE_VAR(driver,
                                 "driver",
                                 "Linux Driver Interface",
                                 ATH_DEBUG_MASK_DEFAULTS | ATH_DEBUG_WLAN_SCAN |
                                 ATH_DEBUG_HCI_BRIDGE,
                                 ATH_DEBUG_DESCRIPTION_COUNT(driver_debug_desc),
                                 driver_debug_desc);

#endif


#define IS_MAC_NULL(mac) (mac[0]==0 && mac[1]==0 && mac[2]==0 && mac[3]==0 && mac[4]==0 && mac[5]==0)
#define IS_MAC_BCAST(mac) (*mac==0xff)

#define DESCRIPTION "Driver to access the Atheros AR600x Device, version " __stringify(__VER_MAJOR_) "." __stringify(__VER_MINOR_) "." __stringify(__VER_PATCH_) "." __stringify(__BUILD_NUMBER_)

MODULE_AUTHOR("Atheros Communications, Inc.");
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_LICENSE("Dual BSD/GPL");

#ifndef REORG_APTC_HEURISTICS
#undef ADAPTIVE_POWER_THROUGHPUT_CONTROL
#endif /* REORG_APTC_HEURISTICS */

#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
#define APTC_TRAFFIC_SAMPLING_INTERVAL     100  /* msec */
#define APTC_UPPER_THROUGHPUT_THRESHOLD    3000 /* Kbps */
#define APTC_LOWER_THROUGHPUT_THRESHOLD    2000 /* Kbps */

typedef struct aptc_traffic_record {
    A_BOOL timerScheduled;
    struct timeval samplingTS;
    unsigned long bytesReceived;
    unsigned long bytesTransmitted;
} APTC_TRAFFIC_RECORD;

A_TIMER aptcTimer;
APTC_TRAFFIC_RECORD aptcTR;
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

#ifdef EXPORT_HCI_BRIDGE_INTERFACE
// callbacks registered by HCI transport driver
HCI_TRANSPORT_CALLBACKS ar6kHciTransCallbacks = { NULL };
#endif

unsigned int processDot11Hdr = 0;
int bmienable = BMIENABLE_DEFAULT;

char ifname[IFNAMSIZ] = {0,};

int wlaninitmode = WLAN_INIT_MODE_DEFAULT;
unsigned int bypasswmi = 0;
unsigned int debuglevel = 0;
int tspecCompliance = ATHEROS_COMPLIANCE;
unsigned int busspeedlow = 0;
unsigned int onebitmode = 0;
unsigned int skipflash = 0;
unsigned int wmitimeout = 2;
unsigned int wlanNodeCaching = 1;
unsigned int enableuartprint = ENABLEUARTPRINT_DEFAULT;
unsigned int logWmiRawMsgs = 0;
unsigned int enabletimerwar = 0;
unsigned int fwmode = 1;
unsigned int mbox_yield_limit = 99;
unsigned int enablerssicompensation = 0;
int reduce_credit_dribble = 1 + HTC_CONNECT_FLAGS_THRESHOLD_LEVEL_ONE_HALF;
int allow_trace_signal = 0;
#ifdef CONFIG_HOST_TCMD_SUPPORT
unsigned int testmode =0;
#endif

unsigned int irqprocmode = HIF_DEVICE_IRQ_SYNC_ONLY;//HIF_DEVICE_IRQ_ASYNC_SYNC;
unsigned int panic_on_assert = 1;
unsigned int nohifscattersupport = NOHIFSCATTERSUPPORT_DEFAULT;

unsigned int setuphci = SETUPHCI_DEFAULT;
unsigned int setuphcipal = SETUPHCIPAL_DEFAULT;
unsigned int loghci = 0;
unsigned int setupbtdev = SETUPBTDEV_DEFAULT;
#ifndef EXPORT_HCI_BRIDGE_INTERFACE
unsigned int ar3khcibaud = AR3KHCIBAUD_DEFAULT;
unsigned int hciuartscale = HCIUARTSCALE_DEFAULT;
unsigned int hciuartstep = HCIUARTSTEP_DEFAULT;
#endif
#ifdef CONFIG_CHECKSUM_OFFLOAD
unsigned int csumOffload=0;
unsigned int csumOffloadTest=0;
#endif
unsigned int eppingtest=0;

module_param_string(ifname, ifname, sizeof(ifname), 0644);
module_param(wlaninitmode, int, 0644);
module_param(bmienable, int, 0644);
module_param(bypasswmi, uint, 0644);
module_param(debuglevel, uint, 0644);
module_param(tspecCompliance, int, 0644);
module_param(onebitmode, uint, 0644);
module_param(busspeedlow, uint, 0644);
module_param(skipflash, uint, 0644);
module_param(wmitimeout, uint, 0644);
module_param(wlanNodeCaching, uint, 0644);
module_param(logWmiRawMsgs, uint, 0644);
module_param(enableuartprint, uint, 0644);
module_param(enabletimerwar, uint, 0644);
module_param(fwmode, uint, 0644);
module_param(mbox_yield_limit, uint, 0644);
module_param(reduce_credit_dribble, int, 0644);
module_param(allow_trace_signal, int, 0644);
module_param(enablerssicompensation, uint, 0644);
module_param(processDot11Hdr, uint, 0644);
#ifdef CONFIG_CHECKSUM_OFFLOAD
module_param(csumOffload, uint, 0644);
#endif
#ifdef CONFIG_HOST_TCMD_SUPPORT
module_param(testmode, uint, 0644);
#endif
module_param(irqprocmode, uint, 0644);
module_param(nohifscattersupport, uint, 0644);
module_param(panic_on_assert, uint, 0644);
module_param(setuphci, uint, 0644);
module_param(setuphcipal, uint, 0644);
module_param(loghci, uint, 0644);
module_param(setupbtdev, uint, 0644);
#ifndef EXPORT_HCI_BRIDGE_INTERFACE
module_param(ar3khcibaud, uint, 0644);
module_param(hciuartscale, uint, 0644);
module_param(hciuartstep, uint, 0644);
#endif
module_param(eppingtest, uint, 0644);

/* in 2.6.10 and later this is now a pointer to a uint */
unsigned int _mboxnum = HTC_MAILBOX_NUM_MAX;
#define mboxnum &_mboxnum

#ifdef DEBUG
A_UINT32 g_dbg_flags = DBG_DEFAULTS;
unsigned int debugflags = 0;
int debugdriver = 0;
unsigned int debughtc = 0;
unsigned int debugbmi = 0;
unsigned int debughif = 0;
unsigned int txcreditsavailable[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int txcreditsconsumed[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int txcreditintrenable[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int txcreditintrenableaggregate[HTC_MAILBOX_NUM_MAX] = {0};
module_param(debugflags, uint, 0644);
module_param(debugdriver, int, 0644);
module_param(debughtc, uint, 0644);
module_param(debugbmi, uint, 0644);
module_param(debughif, uint, 0644);
module_param_array(txcreditsavailable, uint, mboxnum, 0644);
module_param_array(txcreditsconsumed, uint, mboxnum, 0644);
module_param_array(txcreditintrenable, uint, mboxnum, 0644);
module_param_array(txcreditintrenableaggregate, uint, mboxnum, 0644);

#endif /* DEBUG */

unsigned int resetok = 1;
unsigned int tx_attempt[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int tx_post[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int tx_complete[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int hifBusRequestNumMax = 40;
unsigned int war23838_disabled = 0;
#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
unsigned int enableAPTCHeuristics = 1;
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */
module_param_array(tx_attempt, uint, mboxnum, 0644);
module_param_array(tx_post, uint, mboxnum, 0644);
module_param_array(tx_complete, uint, mboxnum, 0644);
module_param(hifBusRequestNumMax, uint, 0644);
module_param(war23838_disabled, uint, 0644);
module_param(resetok, uint, 0644);
#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
module_param(enableAPTCHeuristics, uint, 0644);
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

#ifdef BLOCK_TX_PATH_FLAG
int blocktx = 0;
module_param(blocktx, int, 0644);
#endif /* BLOCK_TX_PATH_FLAG */

typedef struct user_rssi_compensation_t {
    A_UINT16         customerID;
    union {
    A_UINT16         a_enable;
    A_UINT16         bg_enable;
    A_UINT16         enable;
    };
    A_INT16          bg_param_a;
    A_INT16          bg_param_b;
    A_INT16          a_param_a;
    A_INT16          a_param_b;
    A_UINT32         reserved;
} USER_RSSI_CPENSATION;

static USER_RSSI_CPENSATION rssi_compensation_param;

static A_INT16 rssi_compensation_table[96];

int reconnect_flag = 0;
static ar6k_pal_config_t ar6k_pal_config_g;

/* Function declarations */
static int ar6000_init_module(void);
static void ar6000_cleanup_module(void);

int ar6000_init(struct net_device *dev);
static int ar6000_open(struct net_device *dev);
static int ar6000_close(struct net_device *dev);
static void ar6000_init_control_info(AR_SOFTC_T *ar);
static int ar6000_data_tx(struct sk_buff *skb, struct net_device *dev);

void ar6000_destroy(struct net_device *dev, unsigned int unregister);
static void ar6000_detect_error(unsigned long ptr);
static void	ar6000_set_multicast_list(struct net_device *dev);
static struct net_device_stats *ar6000_get_stats(struct net_device *dev);
static struct iw_statistics *ar6000_get_iwstats(struct net_device * dev);

static void disconnect_timer_handler(unsigned long ptr);

void read_rssi_compensation_param(AR_SOFTC_T *ar);

    /* for android builds we call external APIs that handle firmware download and configuration */
#ifdef ANDROID_ENV
/* !!!! Interim android support to make it easier to patch the default driver for
 * android use. You must define an external source file ar6000_android.c that handles the following
 * APIs */
extern void android_module_init(OSDRV_CALLBACKS *osdrvCallbacks);
extern void android_module_exit(void);
#endif
/*
 * HTC service connection handlers
 */
static A_STATUS ar6000_avail_ev(void *context, void *hif_handle);

static A_STATUS ar6000_unavail_ev(void *context, void *hif_handle);

A_STATUS ar6000_configure_target(AR_SOFTC_T *ar);

static void ar6000_target_failure(void *Instance, A_STATUS Status);

static void ar6000_rx(void *Context, HTC_PACKET *pPacket);

static void ar6000_rx_refill(void *Context,HTC_ENDPOINT_ID Endpoint);

static void ar6000_tx_complete(void *Context, HTC_PACKET_QUEUE *pPackets);

static HTC_SEND_FULL_ACTION ar6000_tx_queue_full(void *Context, HTC_PACKET *pPacket);

#ifdef ATH_AR6K_11N_SUPPORT
static void ar6000_alloc_netbufs(A_NETBUF_QUEUE_T *q, A_UINT16 num);
#endif
static void ar6000_deliver_frames_to_nw_stack(void * dev, void *osbuf);
//static void ar6000_deliver_frames_to_bt_stack(void * dev, void *osbuf);

static HTC_PACKET *ar6000_alloc_amsdu_rxbuf(void *Context, HTC_ENDPOINT_ID Endpoint, int Length);

static void ar6000_refill_amsdu_rxbufs(AR_SOFTC_T *ar, int Count);

static void ar6000_cleanup_amsdu_rxbufs(AR_SOFTC_T *ar);

static ssize_t
ar6000_sysfs_bmi_read(struct file *fp, struct kobject *kobj,
                      struct bin_attribute *bin_attr,
                      char *buf, loff_t pos, size_t count);

static ssize_t
ar6000_sysfs_bmi_write(struct file *fp, struct kobject *kobj,
                       struct bin_attribute *bin_attr,
                       char *buf, loff_t pos, size_t count);

static A_STATUS
ar6000_sysfs_bmi_init(AR_SOFTC_T *ar);

/* HCI PAL callback function declarations */
A_STATUS ar6k_setup_hci_pal(AR_SOFTC_T *ar);
void  ar6k_cleanup_hci_pal(AR_SOFTC_T *ar);

static void
ar6000_sysfs_bmi_deinit(AR_SOFTC_T *ar);

A_STATUS
ar6000_sysfs_bmi_get_config(AR_SOFTC_T *ar, A_UINT32 mode);

/*
 * Static variables
 */

struct net_device *ar6000_devices[MAX_AR6000];
static int is_netdev_registered;
extern struct iw_handler_def ath_iw_handler_def;
DECLARE_WAIT_QUEUE_HEAD(arEvent);
static void ar6000_cookie_init(AR_SOFTC_T *ar);
static void ar6000_cookie_cleanup(AR_SOFTC_T *ar);
static void ar6000_free_cookie(AR_SOFTC_T *ar, struct ar_cookie * cookie);
static struct ar_cookie *ar6000_alloc_cookie(AR_SOFTC_T *ar);

#ifdef USER_KEYS
static A_STATUS ar6000_reinstall_keys(AR_SOFTC_T *ar,A_UINT8 key_op_ctrl);
#endif

#ifdef CONFIG_AP_VIRTUAL_ADAPTER_SUPPORT
struct net_device *arApNetDev;
#endif /* CONFIG_AP_VIRTUAL_ADAPTER_SUPPORT */

static struct ar_cookie s_ar_cookie_mem[MAX_COOKIE_NUM];

#define HOST_INTEREST_ITEM_ADDRESS(ar, item) \
        (((ar)->arTargetType == TARGET_TYPE_AR6002) ? AR6002_HOST_INTEREST_ITEM_ADDRESS(item) : \
        (((ar)->arTargetType == TARGET_TYPE_AR6003) ? AR6003_HOST_INTEREST_ITEM_ADDRESS(item) : 0))


static struct net_device_ops ar6000_netdev_ops = {
    .ndo_init               = NULL,
    .ndo_open               = ar6000_open,
    .ndo_stop               = ar6000_close,
    .ndo_get_stats          = ar6000_get_stats,
    .ndo_do_ioctl           = ar6000_ioctl,
    .ndo_start_xmit         = ar6000_data_tx,
    .ndo_set_multicast_list = ar6000_set_multicast_list,
};

/* Debug log support */

/*
 * Flag to govern whether the debug logs should be parsed in the kernel
 * or reported to the application.
 */
#define REPORT_DEBUG_LOGS_TO_APP

A_STATUS
ar6000_set_host_app_area(AR_SOFTC_T *ar)
{
    A_UINT32 address, data;
    struct host_app_area_s host_app_area;

    /* Fetch the address of the host_app_area_s instance in the host interest area */
    address = TARG_VTOP(ar->arTargetType, HOST_INTEREST_ITEM_ADDRESS(ar, hi_app_host_interest));
    if (ar6000_ReadRegDiag(ar->arHifDevice, &address, &data) != A_OK) {
        return A_ERROR;
    }
    address = TARG_VTOP(ar->arTargetType, data);
    host_app_area.wmi_protocol_ver = WMI_PROTOCOL_VERSION;
    if (ar6000_WriteDataDiag(ar->arHifDevice, address,
                             (A_UCHAR *)&host_app_area,
                             sizeof(struct host_app_area_s)) != A_OK)
    {
        return A_ERROR;
    }

    return A_OK;
}

A_UINT32
dbglog_get_debug_hdr_ptr(AR_SOFTC_T *ar)
{
    A_UINT32 param;
    A_UINT32 address;
    A_STATUS status;

    address = TARG_VTOP(ar->arTargetType, HOST_INTEREST_ITEM_ADDRESS(ar, hi_dbglog_hdr));
    if ((status = ar6000_ReadDataDiag(ar->arHifDevice, address,
                                      (A_UCHAR *)&param, 4)) != A_OK)
    {
        param = 0;
    }

    return param;
}

/*
 * The dbglog module has been initialized. Its ok to access the relevant
 * data stuctures over the diagnostic window.
 */
void
ar6000_dbglog_init_done(AR_SOFTC_T *ar)
{
    ar->dbglog_init_done = TRUE;
}

A_UINT32
dbglog_get_debug_fragment(A_INT8 *datap, A_UINT32 len, A_UINT32 limit)
{
    A_INT32 *buffer;
    A_UINT32 count;
    A_UINT32 numargs;
    A_UINT32 length;
    A_UINT32 fraglen;

    count = fraglen = 0;
    buffer = (A_INT32 *)datap;
    length = (limit >> 2);

    if (len <= limit) {
        fraglen = len;
    } else {
        while (count < length) {
            numargs = DBGLOG_GET_NUMARGS(buffer[count]);
            fraglen = (count << 2);
            count += numargs + 1;
        }
    }

    return fraglen;
}

void
dbglog_parse_debug_logs(A_INT8 *datap, A_UINT32 len)
{
    A_INT32 *buffer;
    A_UINT32 count;
    A_UINT32 timestamp;
    A_UINT32 debugid;
    A_UINT32 moduleid;
    A_UINT32 numargs;
    A_UINT32 length;

    count = 0;
    buffer = (A_INT32 *)datap;
    length = (len >> 2);
    while (count < length) {
        debugid = DBGLOG_GET_DBGID(buffer[count]);
        moduleid = DBGLOG_GET_MODULEID(buffer[count]);
        numargs = DBGLOG_GET_NUMARGS(buffer[count]);
        timestamp = DBGLOG_GET_TIMESTAMP(buffer[count]);
        switch (numargs) {
            case 0:
            AR_DEBUG_PRINTF(ATH_DEBUG_DBG_LOG,("%d %d (%d)\n", moduleid, debugid, timestamp));
            break;

            case 1:
            AR_DEBUG_PRINTF(ATH_DEBUG_DBG_LOG,("%d %d (%d): 0x%x\n", moduleid, debugid,
                            timestamp, buffer[count+1]));
            break;

            case 2:
            AR_DEBUG_PRINTF(ATH_DEBUG_DBG_LOG,("%d %d (%d): 0x%x, 0x%x\n", moduleid, debugid,
                            timestamp, buffer[count+1], buffer[count+2]));
            break;

            default:
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Invalid args: %d\n", numargs));
        }
        count += numargs + 1;
    }
}

int
ar6000_dbglog_get_debug_logs(AR_SOFTC_T *ar)
{
    A_UINT32 data[8]; /* Should be able to accomodate struct dbglog_buf_s */
    A_UINT32 address;
    A_UINT32 length;
    A_UINT32 dropped;
    A_UINT32 firstbuf;
    A_UINT32 debug_hdr_ptr;

    if (!ar->dbglog_init_done) return A_ERROR;


    AR6000_SPIN_LOCK(&ar->arLock, 0);

    if (ar->dbgLogFetchInProgress) {
        AR6000_SPIN_UNLOCK(&ar->arLock, 0);
        return A_EBUSY;
    }

        /* block out others */
    ar->dbgLogFetchInProgress = TRUE;

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    debug_hdr_ptr = dbglog_get_debug_hdr_ptr(ar);
    printk("debug_hdr_ptr: 0x%x\n", debug_hdr_ptr);

    /* Get the contents of the ring buffer */
    if (debug_hdr_ptr) {
        address = TARG_VTOP(ar->arTargetType, debug_hdr_ptr);
        length = 4 /* sizeof(dbuf) */ + 4 /* sizeof(dropped) */;
        A_MEMZERO(data, sizeof(data));
        ar6000_ReadDataDiag(ar->arHifDevice, address, (A_UCHAR *)data, length);
        address = TARG_VTOP(ar->arTargetType, data[0] /* dbuf */);
        firstbuf = address;
        dropped = data[1]; /* dropped */
        length = 4 /* sizeof(next) */ + 4 /* sizeof(buffer) */ + 4 /* sizeof(bufsize) */ + 4 /* sizeof(length) */ + 4 /* sizeof(count) */ + 4 /* sizeof(free) */;
        A_MEMZERO(data, sizeof(data));
        ar6000_ReadDataDiag(ar->arHifDevice, address, (A_UCHAR *)&data, length);

        do {
            address = TARG_VTOP(ar->arTargetType, data[1] /* buffer*/);
            length = data[3]; /* length */
            if ((length) && (length <= data[2] /* bufsize*/)) {
                /* Rewind the index if it is about to overrun the buffer */
                if (ar->log_cnt > (DBGLOG_HOST_LOG_BUFFER_SIZE - length)) {
                    ar->log_cnt = 0;
                }
                if(A_OK != ar6000_ReadDataDiag(ar->arHifDevice, address,
                                    (A_UCHAR *)&ar->log_buffer[ar->log_cnt], length))
                {
                    break;
                }
                ar6000_dbglog_event(ar, dropped, (A_INT8*)&ar->log_buffer[ar->log_cnt], length);
                ar->log_cnt += length;
            } else {
                AR_DEBUG_PRINTF(ATH_DEBUG_DBG_LOG,("Length: %d (Total size: %d)\n",
                                data[3], data[2]));
            }

            address = TARG_VTOP(ar->arTargetType, data[0] /* next */);
            length = 4 /* sizeof(next) */ + 4 /* sizeof(buffer) */ + 4 /* sizeof(bufsize) */ + 4 /* sizeof(length) */ + 4 /* sizeof(count) */ + 4 /* sizeof(free) */;
            A_MEMZERO(data, sizeof(data));
            if(A_OK != ar6000_ReadDataDiag(ar->arHifDevice, address,
                                (A_UCHAR *)&data, length))
            {
                break;
            }

        } while (address != firstbuf);
    }

    ar->dbgLogFetchInProgress = FALSE;

    return A_OK;
}

void
ar6000_dbglog_event(AR_SOFTC_T *ar, A_UINT32 dropped,
                    A_INT8 *buffer, A_UINT32 length)
{
#ifdef REPORT_DEBUG_LOGS_TO_APP
    #define MAX_WIRELESS_EVENT_SIZE 252
    /*
     * Break it up into chunks of MAX_WIRELESS_EVENT_SIZE bytes of messages.
     * There seems to be a limitation on the length of message that could be
     * transmitted to the user app via this mechanism.
     */
    A_UINT32 send, sent;

    sent = 0;
    send = dbglog_get_debug_fragment(&buffer[sent], length - sent,
                                     MAX_WIRELESS_EVENT_SIZE);
    while (send) {
        ar6000_send_event_to_app(ar, WMIX_DBGLOG_EVENTID, (A_UINT8*)&buffer[sent], send);
        sent += send;
        send = dbglog_get_debug_fragment(&buffer[sent], length - sent,
                                         MAX_WIRELESS_EVENT_SIZE);
    }
#else
    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Dropped logs: 0x%x\nDebug info length: %d\n",
                    dropped, length));

    /* Interpret the debug logs */
    dbglog_parse_debug_logs((A_INT8*)buffer, length);
#endif /* REPORT_DEBUG_LOGS_TO_APP */
}


static int __init
ar6000_init_module(void)
{
    static int probed = 0;
    A_STATUS status;
    OSDRV_CALLBACKS osdrvCallbacks;

    a_module_debug_support_init();

#ifdef DEBUG
        /* check for debug mask overrides */
    if (debughtc != 0) {
        ATH_DEBUG_SET_DEBUG_MASK(htc,debughtc);
    }
    if (debugbmi != 0) {
        ATH_DEBUG_SET_DEBUG_MASK(bmi,debugbmi);
    }
    if (debughif != 0) {
        ATH_DEBUG_SET_DEBUG_MASK(hif,debughif);
    }
    if (debugdriver != 0) {
        ATH_DEBUG_SET_DEBUG_MASK(driver,debugdriver);
    }

#endif

    A_REGISTER_MODULE_DEBUG_INFO(driver);

    A_MEMZERO(&osdrvCallbacks,sizeof(osdrvCallbacks));
    osdrvCallbacks.deviceInsertedHandler = ar6000_avail_ev;
    osdrvCallbacks.deviceRemovedHandler = ar6000_unavail_ev;
#ifdef CONFIG_PM
    osdrvCallbacks.deviceSuspendHandler = ar6000_suspend_ev;
    osdrvCallbacks.deviceResumeHandler = ar6000_resume_ev;
    osdrvCallbacks.devicePowerChangeHandler = ar6000_power_change_ev;
#endif

    ar6000_pm_init();

#ifdef ANDROID_ENV
    android_module_init(&osdrvCallbacks);
#endif

#ifdef DEBUG
    /* Set the debug flags if specified at load time */
    if(debugflags != 0)
    {
        g_dbg_flags = debugflags;
    }
#endif

    if (probed) {
        return -ENODEV;
    }
    probed++;

#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
    memset(&aptcTR, 0, sizeof(APTC_TRAFFIC_RECORD));
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

#ifdef CONFIG_HOST_GPIO_SUPPORT
    ar6000_gpio_init();
#endif /* CONFIG_HOST_GPIO_SUPPORT */

    status = HIFInit(&osdrvCallbacks);
    if(status != A_OK)
        return -ENODEV;

    return 0;
}

static void __exit
ar6000_cleanup_module(void)
{
    int i = 0;
    struct net_device *ar6000_netdev;

#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
    /* Delete the Adaptive Power Control timer */
    if (timer_pending(&aptcTimer)) {
        del_timer_sync(&aptcTimer);
    }
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

    for (i=0; i < MAX_AR6000; i++) {
        if (ar6000_devices[i] != NULL) {
            ar6000_netdev = ar6000_devices[i];
            ar6000_devices[i] = NULL;
            ar6000_destroy(ar6000_netdev, 1);
        }
    }

    HIFShutDownDevice(NULL);

    a_module_debug_support_cleanup();

    ar6000_pm_exit();

#ifdef ANDROID_ENV    
    android_module_exit();
#endif

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("ar6000_cleanup: success\n"));
}

#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
void
aptcTimerHandler(unsigned long arg)
{
    A_UINT32 numbytes;
    A_UINT32 throughput;
    AR_SOFTC_T *ar;
    A_STATUS status;

    ar = (AR_SOFTC_T *)arg;
    A_ASSERT(ar != NULL);
    A_ASSERT(!timer_pending(&aptcTimer));

    AR6000_SPIN_LOCK(&ar->arLock, 0);

    /* Get the number of bytes transferred */
    numbytes = aptcTR.bytesTransmitted + aptcTR.bytesReceived;
    aptcTR.bytesTransmitted = aptcTR.bytesReceived = 0;

    /* Calculate and decide based on throughput thresholds */
    throughput = ((numbytes * 8)/APTC_TRAFFIC_SAMPLING_INTERVAL); /* Kbps */
    if (throughput < APTC_LOWER_THROUGHPUT_THRESHOLD) {
        /* Enable Sleep and delete the timer */
        A_ASSERT(ar->arWmiReady == TRUE);
        AR6000_SPIN_UNLOCK(&ar->arLock, 0);
        status = wmi_powermode_cmd(ar->arWmi, REC_POWER);
        AR6000_SPIN_LOCK(&ar->arLock, 0);
        A_ASSERT(status == A_OK);
        aptcTR.timerScheduled = FALSE;
    } else {
        A_TIMEOUT_MS(&aptcTimer, APTC_TRAFFIC_SAMPLING_INTERVAL, 0);
    }

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);
}
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

#ifdef ATH_AR6K_11N_SUPPORT
static void
ar6000_alloc_netbufs(A_NETBUF_QUEUE_T *q, A_UINT16 num)
{
    void * osbuf;

    while(num) {
        if((osbuf = A_NETBUF_ALLOC(AR6000_BUFFER_SIZE))) {
            A_NETBUF_ENQUEUE(q, osbuf);
        } else {
            break;
        }
        num--;
    }

    if(num) {
        A_PRINTF("%s(), allocation of netbuf failed", __func__);
    }
}
#endif

static struct bin_attribute bmi_attr = {
    .attr = {.name = "bmi", .mode = 0600},
    .read = ar6000_sysfs_bmi_read,
    .write = ar6000_sysfs_bmi_write,
};

static ssize_t
ar6000_sysfs_bmi_read(struct file *fp, struct kobject *kobj,
                      struct bin_attribute *bin_attr,
                      char *buf, loff_t pos, size_t count)
{
    int index;
    AR_SOFTC_T *ar;
    HIF_DEVICE_OS_DEVICE_INFO   *osDevInfo;

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("BMI: Read %d bytes\n", (A_UINT32)count));
    for (index=0; index < MAX_AR6000; index++) {
        ar = (AR_SOFTC_T *)ar6k_priv(ar6000_devices[index]);
        osDevInfo = &ar->osDevInfo;
        if (kobj == (&(((struct device *)osDevInfo->pOSDevice)->kobj))) {
            break;
        }
    }

    if (index == MAX_AR6000) return 0;

    if ((BMIRawRead(ar->arHifDevice, (A_UCHAR*)buf, count, TRUE)) != A_OK) {
        return 0;
    }

    return count;
}

static ssize_t
ar6000_sysfs_bmi_write(struct file *fp, struct kobject *kobj,
                       struct bin_attribute *bin_attr,
                       char *buf, loff_t pos, size_t count)
{
    int index;
    AR_SOFTC_T *ar;
    HIF_DEVICE_OS_DEVICE_INFO   *osDevInfo;

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("BMI: Write %d bytes\n", (A_UINT32)count));
    for (index=0; index < MAX_AR6000; index++) {
        ar = (AR_SOFTC_T *)ar6k_priv(ar6000_devices[index]);
        osDevInfo = &ar->osDevInfo;
        if (kobj == (&(((struct device *)osDevInfo->pOSDevice)->kobj))) {
            break;
        }
    }

    if (index == MAX_AR6000) return 0;

    if ((BMIRawWrite(ar->arHifDevice, (A_UCHAR*)buf, count)) != A_OK) {
        return 0;
    }

    return count;
}

static A_STATUS
ar6000_sysfs_bmi_init(AR_SOFTC_T *ar)
{
    A_STATUS status;

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("BMI: Creating sysfs entry\n"));
    A_MEMZERO(&ar->osDevInfo, sizeof(HIF_DEVICE_OS_DEVICE_INFO));

    /* Get the underlying OS device */
    status = HIFConfigureDevice(ar->arHifDevice,
                                HIF_DEVICE_GET_OS_DEVICE,
                                &ar->osDevInfo,
                                sizeof(HIF_DEVICE_OS_DEVICE_INFO));

    if (A_FAILED(status)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("BMI: Failed to get OS device info from HIF\n"));
        return A_ERROR;
    }

    /* Create a bmi entry in the sysfs filesystem */
    if ((sysfs_create_bin_file(&(((struct device *)ar->osDevInfo.pOSDevice)->kobj), &bmi_attr)) < 0)
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMI: Failed to create entry for bmi in sysfs filesystem\n"));
        return A_ERROR;
    }

    return A_OK;
}

static void
ar6000_sysfs_bmi_deinit(AR_SOFTC_T *ar)
{
    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("BMI: Deleting sysfs entry\n"));

    sysfs_remove_bin_file(&(((struct device *)ar->osDevInfo.pOSDevice)->kobj), &bmi_attr);
}

#define bmifn(fn) do { \
    if ((fn) < A_OK) { \
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("BMI operation failed: %d\n", __LINE__)); \
        return A_ERROR; \
    } \
} while(0)

#ifdef INIT_MODE_DRV_ENABLED

#ifdef SOFTMAC_FILE_USED
#define AR6002_MAC_ADDRESS_OFFSET     0x0A
#define AR6003_MAC_ADDRESS_OFFSET     0x16
static
void calculate_crc(A_UINT32 TargetType, A_UCHAR *eeprom_data)
{
    A_UINT16        *ptr_crc;
    A_UINT16        *ptr16_eeprom;
    A_UINT16        checksum;
    A_UINT32        i;
    A_UINT32        eeprom_size;

    if (TargetType == TARGET_TYPE_AR6001)
    {
        eeprom_size = 512;
        ptr_crc = (A_UINT16 *)eeprom_data;
    }
    else if (TargetType == TARGET_TYPE_AR6003)
    {
        eeprom_size = 1024;
        ptr_crc = (A_UINT16 *)((A_UCHAR *)eeprom_data + 0x04);
    }
    else
    {
        eeprom_size = 768;
        ptr_crc = (A_UINT16 *)((A_UCHAR *)eeprom_data + 0x04);
    }


    // Clear the crc
    *ptr_crc = 0;

    // Recalculate new CRC
    checksum = 0;
    ptr16_eeprom = (A_UINT16 *)eeprom_data;
    for (i = 0;i < eeprom_size; i += 2)
    {
        checksum = checksum ^ (*ptr16_eeprom);
        ptr16_eeprom++;
    }
    checksum = 0xFFFF ^ checksum;
    *ptr_crc = checksum;
}

static void 
ar6000_softmac_update(AR_SOFTC_T *ar, A_UCHAR *eeprom_data, size_t size)
{
    const char *source = "random generated";
    const struct firmware *softmac_entry;
    A_UCHAR *ptr_mac;
    switch (ar->arTargetType) {
    case TARGET_TYPE_AR6002:
        ptr_mac = (A_UINT8 *)((A_UCHAR *)eeprom_data + AR6002_MAC_ADDRESS_OFFSET);
        break;
    case TARGET_TYPE_AR6003:
        ptr_mac = (A_UINT8 *)((A_UCHAR *)eeprom_data + AR6003_MAC_ADDRESS_OFFSET);
        break;
    default:
	AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Invalid Target Type\n"));
        return;
    }
	printk(KERN_DEBUG "MAC from EEPROM %pM\n", ptr_mac);

    /* create a random MAC in case we cannot read file from system */
    ptr_mac[0] = 0;
    ptr_mac[1] = 0x03;
    ptr_mac[2] = 0x7F;
    ptr_mac[3] = random32() & 0xff; 
    ptr_mac[4] = random32() & 0xff; 
    ptr_mac[5] = random32() & 0xff; 
    if ((A_REQUEST_FIRMWARE(&softmac_entry, "softmac", ((struct device *)ar->osDevInfo.pOSDevice))) == 0)
    {
        A_CHAR *macbuf = A_MALLOC_NOWAIT(softmac_entry->size+1);
        if (macbuf) {            
            unsigned int softmac[6];
            memcpy(macbuf, softmac_entry->data, softmac_entry->size);
            macbuf[softmac_entry->size] = '\0';
            if (sscanf(macbuf, "%02x:%02x:%02x:%02x:%02x:%02x", 
                        &softmac[0], &softmac[1], &softmac[2],
                        &softmac[3], &softmac[4], &softmac[5])==6) {
                int i;
                for (i=0; i<6; ++i) {
                    ptr_mac[i] = softmac[i] & 0xff;
                }
                source = "softmac file";
            }
            A_FREE(macbuf);
        }
        A_RELEASE_FIRMWARE(softmac_entry);
    }
	printk(KERN_DEBUG "MAC from %s %pM\n", source, ptr_mac);
   calculate_crc(ar->arTargetType, eeprom_data);
}
#endif /* SOFTMAC_FILE_USED */

static A_STATUS
ar6000_transfer_bin_file(AR_SOFTC_T *ar, AR6K_BIN_FILE file, A_UINT32 address, A_BOOL compressed)
{
    A_STATUS status;
    const char *filename;
    const struct firmware *fw_entry;
    A_UINT32 fw_entry_size;

    switch (file) {
        case AR6K_OTP_FILE:
            if (ar->arVersion.target_ver == AR6003_REV1_VERSION) {
                filename = AR6003_REV1_OTP_FILE;
            } else if (ar->arVersion.target_ver == AR6003_REV2_VERSION) {
                filename = AR6003_REV2_OTP_FILE;
            } else {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Unknown firmware revision: %d\n", ar->arVersion.target_ver));
                return A_ERROR;
            }
            break;

        case AR6K_FIRMWARE_FILE:
            if (ar->arVersion.target_ver == AR6003_REV1_VERSION) {
                filename = AR6003_REV1_FIRMWARE_FILE;
            } else if (ar->arVersion.target_ver == AR6003_REV2_VERSION) {
                filename = AR6003_REV2_FIRMWARE_FILE;
            } else {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Unknown firmware revision: %d\n", ar->arVersion.target_ver));
                return A_ERROR;
            }
            
            if (eppingtest) {
                bypasswmi = TRUE;    
                if (ar->arVersion.target_ver == AR6003_REV1_VERSION) {
                    filename = AR6003_REV1_EPPING_FIRMWARE_FILE;
                } else if (ar->arVersion.target_ver == AR6003_REV2_VERSION) {
                    filename = AR6003_REV2_EPPING_FIRMWARE_FILE;
                } else {
                    AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("eppingtest : unsupported firmware revision: %d\n", 
                        ar->arVersion.target_ver));
                    return A_ERROR;
                }
                compressed = 0;
            }
            
#ifdef CONFIG_HOST_TCMD_SUPPORT
            if(testmode) {
                if (ar->arVersion.target_ver == AR6003_REV1_VERSION) {
                    filename = AR6003_REV1_TCMD_FIRMWARE_FILE;
                } else if (ar->arVersion.target_ver == AR6003_REV2_VERSION) {
                    filename = AR6003_REV2_TCMD_FIRMWARE_FILE;
                } else {
                    AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Unknown firmware revision: %d\n", ar->arVersion.target_ver));
                    return A_ERROR;
                }
                compressed = 0;
            }
#endif 
#ifdef HTC_RAW_INTERFACE
            if (!eppingtest && bypasswmi) {
                if (ar->arVersion.target_ver == AR6003_REV1_VERSION) {
                    filename = AR6003_REV1_ART_FIRMWARE_FILE;
                } else if (ar->arVersion.target_ver == AR6003_REV2_VERSION) {
                    filename = AR6003_REV2_ART_FIRMWARE_FILE;
                } else {
                    AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Unknown firmware revision: %d\n", ar->arVersion.target_ver));
                    return A_ERROR;
                }
                compressed = 0;                
            }
#endif 
            break;

        case AR6K_PATCH_FILE:
            if (ar->arVersion.target_ver == AR6003_REV1_VERSION) {
                filename = AR6003_REV1_PATCH_FILE;
            } else if (ar->arVersion.target_ver == AR6003_REV2_VERSION) {
                filename = AR6003_REV2_PATCH_FILE;
            } else {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Unknown firmware revision: %d\n", ar->arVersion.target_ver));
                return A_ERROR;
            }
            break;

        case AR6K_BOARD_DATA_FILE:
            if (ar->arVersion.target_ver == AR6003_REV1_VERSION) {
                filename = AR6003_REV1_BOARD_DATA_FILE;
            } else if (ar->arVersion.target_ver == AR6003_REV2_VERSION) {
                filename = AR6003_REV2_BOARD_DATA_FILE;
            } else {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Unknown firmware revision: %d\n", ar->arVersion.target_ver));
                return A_ERROR;
            }
            break;

        default:
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Unknown file type: %d\n", file));
            return A_ERROR;
    }
    if ((A_REQUEST_FIRMWARE(&fw_entry, filename, ((struct device *)ar->osDevInfo.pOSDevice))) != 0)
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Failed to get %s\n", filename));
        return A_ENOENT;
    }

#ifdef SOFTMAC_FILE_USED
    if (file==AR6K_BOARD_DATA_FILE && fw_entry->data) {
        ar6000_softmac_update(ar, (A_UCHAR *)fw_entry->data, fw_entry->size);
    }
#endif 


    fw_entry_size = fw_entry->size;

    /* Load extended board data for AR6003 */
    if ((file==AR6K_BOARD_DATA_FILE) && (fw_entry->data)) {
        A_UINT32 board_ext_address;
        A_UINT32 board_ext_data_size;
        A_UINT32 board_data_size;

        board_ext_data_size = (((ar)->arTargetType == TARGET_TYPE_AR6002) ? AR6002_BOARD_EXT_DATA_SZ : \
                               (((ar)->arTargetType == TARGET_TYPE_AR6003) ? AR6003_BOARD_EXT_DATA_SZ : 0));

        board_data_size = (((ar)->arTargetType == TARGET_TYPE_AR6002) ? AR6002_BOARD_DATA_SZ : \
                          (((ar)->arTargetType == TARGET_TYPE_AR6003) ? AR6003_BOARD_DATA_SZ : 0));
        
        /* Determine where in Target RAM to write Board Data */
        bmifn(BMIReadMemory(ar->arHifDevice, HOST_INTEREST_ITEM_ADDRESS(ar, hi_board_ext_data), (A_UCHAR *)&board_ext_address, 4));
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("Board extended Data download address: 0x%x\n", board_ext_address));

        /* check whether the target has allocated memory for extended board data and file contains extended board data */
        if ((board_ext_address) && (fw_entry->size == (board_data_size + board_ext_data_size))) {
            A_UINT32 param;

            status = BMIWriteMemory(ar->arHifDevice, board_ext_address, (A_UCHAR *)(fw_entry->data + board_data_size), board_ext_data_size);

            if (status != A_OK) {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("BMI operation failed: %d\n", __LINE__));
                A_RELEASE_FIRMWARE(fw_entry);
                return A_ERROR;
            }

            /* Record the fact that extended board Data IS initialized */
            param = 1;
            bmifn(BMIWriteMemory(ar->arHifDevice, HOST_INTEREST_ITEM_ADDRESS(ar, hi_board_ext_data_initialized), (A_UCHAR *)&param, 4));
        }
        fw_entry_size = board_data_size;
    }

    if (compressed) {
        status = BMIFastDownload(ar->arHifDevice, address, (A_UCHAR *)fw_entry->data, fw_entry_size);
    } else {
        status = BMIWriteMemory(ar->arHifDevice, address, (A_UCHAR *)fw_entry->data, fw_entry_size);
    }

    if (status != A_OK) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("BMI operation failed: %d\n", __LINE__));
        A_RELEASE_FIRMWARE(fw_entry);
        return A_ERROR;
    }
    A_RELEASE_FIRMWARE(fw_entry);
    return A_OK;
}
#endif /* INIT_MODE_DRV_ENABLED */

A_STATUS
ar6000_update_bdaddr(AR_SOFTC_T *ar)
{

        if (setupbtdev != 0) {
            A_UINT32 address;

           if (BMIReadMemory(ar->arHifDevice,
           	HOST_INTEREST_ITEM_ADDRESS(ar, hi_board_data), (A_UCHAR *)&address, 4) != A_OK)
           {
    	      	AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIReadMemory for hi_board_data failed\n"));
           	return A_ERROR;
           }

           if (BMIReadMemory(ar->arHifDevice, address + BDATA_BDADDR_OFFSET, (A_UCHAR *)ar->bdaddr, 6) != A_OK)
           {
    	    	AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIReadMemory for BD address failed\n"));
           	return A_ERROR;
           }
	   AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BDADDR 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", ar->bdaddr[0],
								ar->bdaddr[1], ar->bdaddr[2], ar->bdaddr[3],
								ar->bdaddr[4], ar->bdaddr[5]));
        }

return A_OK;
}

A_STATUS
ar6000_sysfs_bmi_get_config(AR_SOFTC_T *ar, A_UINT32 mode)
{
    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("BMI: Requesting device specific configuration\n"));

    if (mode == WLAN_INIT_MODE_UDEV) {
        A_CHAR version[16];
        const struct firmware *fw_entry;

        /* Get config using udev through a script in user space */
        sprintf(version, "%2.2x", ar->arVersion.target_ver);
        if ((A_REQUEST_FIRMWARE(&fw_entry, version, ((struct device *)ar->osDevInfo.pOSDevice))) != 0)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("BMI: Failure to get configuration for target version: %s\n", version));
            return A_ERROR;
        }

        A_RELEASE_FIRMWARE(fw_entry);
#ifdef INIT_MODE_DRV_ENABLED
    } else {
        /* The config is contained within the driver itself */
        A_STATUS status;
        A_UINT32 param, options, sleep, address;

        /* Temporarily disable system sleep */
        address = MBOX_BASE_ADDRESS + LOCAL_SCRATCH_ADDRESS;
        bmifn(BMIReadSOCRegister(ar->arHifDevice, address, &param));
        options = param;
        param |= AR6K_OPTION_SLEEP_DISABLE;
        bmifn(BMIWriteSOCRegister(ar->arHifDevice, address, param));

        address = RTC_BASE_ADDRESS + SYSTEM_SLEEP_ADDRESS;
        bmifn(BMIReadSOCRegister(ar->arHifDevice, address, &param));
        sleep = param;
        param |= WLAN_SYSTEM_SLEEP_DISABLE_SET(1);
        bmifn(BMIWriteSOCRegister(ar->arHifDevice, address, param));
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("old options: %d, old sleep: %d\n", options, sleep));

        if (ar->arTargetType == TARGET_TYPE_AR6003) {
            /* Program analog PLL register */
            bmifn(BMIWriteSOCRegister(ar->arHifDevice, ANALOG_INTF_BASE_ADDRESS + 0x284, 0xF9104001));
            /* Run at 80/88MHz by default */
            param = CPU_CLOCK_STANDARD_SET(1);
        } else {
            /* Run at 40/44MHz by default */
            param = CPU_CLOCK_STANDARD_SET(0);
        }
        address = RTC_BASE_ADDRESS + CPU_CLOCK_ADDRESS;
        bmifn(BMIWriteSOCRegister(ar->arHifDevice, address, param));

        param = 0;
        if (ar->arTargetType == TARGET_TYPE_AR6002) {
            bmifn(BMIReadMemory(ar->arHifDevice, HOST_INTEREST_ITEM_ADDRESS(ar, hi_ext_clk_detected), (A_UCHAR *)&param, 4));
        }

        /* LPO_CAL.ENABLE = 1 if no external clk is detected */
        if (param != 1) {
            address = RTC_BASE_ADDRESS + LPO_CAL_ADDRESS;
            param = LPO_CAL_ENABLE_SET(1);
            bmifn(BMIWriteSOCRegister(ar->arHifDevice, address, param));
        }

        /* Venus2.0: Lower SDIO pad drive strength,
         * temporary WAR to avoid SDIO CRC error */
        if (ar->arVersion.target_ver == AR6003_REV2_VERSION) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("AR6K: Temporary WAR to avoid SDIO CRC error\n"));
            param = 0x20;
            address = GPIO_BASE_ADDRESS + GPIO_PIN10_ADDRESS;
            bmifn(BMIWriteSOCRegister(ar->arHifDevice, address, param));

            address = GPIO_BASE_ADDRESS + GPIO_PIN11_ADDRESS;
            bmifn(BMIWriteSOCRegister(ar->arHifDevice, address, param));

            address = GPIO_BASE_ADDRESS + GPIO_PIN12_ADDRESS;
            bmifn(BMIWriteSOCRegister(ar->arHifDevice, address, param));

            address = GPIO_BASE_ADDRESS + GPIO_PIN13_ADDRESS;
            bmifn(BMIWriteSOCRegister(ar->arHifDevice, address, param));
        }

#ifdef FORCE_INTERNAL_CLOCK
        /* Ignore external clock, if any, and force use of internal clock */
        if (ar->arTargetType == TARGET_TYPE_AR6003) {
            /* hi_ext_clk_detected = 0 */
            param = 0;
            bmifn(BMIWriteMemory(ar->arHifDevice, HOST_INTEREST_ITEM_ADDRESS(ar, hi_ext_clk_detected), (A_UCHAR *)&param, 4));

            /* CLOCK_CONTROL &= ~LF_CLK32 */
            address = RTC_BASE_ADDRESS + CLOCK_CONTROL_ADDRESS;
            bmifn(BMIReadSOCRegister(ar->arHifDevice, address, &param));
            param &= (~CLOCK_CONTROL_LF_CLK32_SET(1));
            bmifn(BMIWriteSOCRegister(ar->arHifDevice, address, param));
        }
#endif /* FORCE_INTERNAL_CLOCK */

        /* Transfer Board Data from Target EEPROM to Target RAM */
        if (ar->arTargetType == TARGET_TYPE_AR6003) {
            /* Determine where in Target RAM to write Board Data */
            bmifn(BMIReadMemory(ar->arHifDevice, HOST_INTEREST_ITEM_ADDRESS(ar, hi_board_data), (A_UCHAR *)&address, 4));
            AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("Board Data download address: 0x%x\n", address));

            /* Write EEPROM data to Target RAM */
            if ((ar6000_transfer_bin_file(ar, AR6K_BOARD_DATA_FILE, address, FALSE)) != A_OK) {
                return A_ERROR;
            }

            /* Record the fact that Board Data IS initialized */
            param = 1;
            bmifn(BMIWriteMemory(ar->arHifDevice, HOST_INTEREST_ITEM_ADDRESS(ar, hi_board_data_initialized), (A_UCHAR *)&param, 4));

            /* Transfer One time Programmable data */
            AR6K_DATA_DOWNLOAD_ADDRESS(address, ar->arVersion.target_ver);
            status = ar6000_transfer_bin_file(ar, AR6K_OTP_FILE, address, TRUE);
            if (status == A_OK) {
                /* Execute the OTP code */
                param = 0;
                AR6K_APP_START_OVERRIDE_ADDRESS(address, ar->arVersion.target_ver);
                bmifn(BMIExecute(ar->arHifDevice, address, &param));
            } else if (status != A_ENOENT) {
                return A_ERROR;
            } 
        } else {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Programming of board data for chip %d not supported\n", ar->arTargetType));
            return A_ERROR;
        }

        /* Download Target firmware */
        AR6K_DATA_DOWNLOAD_ADDRESS(address, ar->arVersion.target_ver);
        if ((ar6000_transfer_bin_file(ar, AR6K_FIRMWARE_FILE, address, TRUE)) != A_OK) {
            return A_ERROR;
        }

        /* Set starting address for firmware */
        AR6K_APP_START_OVERRIDE_ADDRESS(address, ar->arVersion.target_ver);
        bmifn(BMISetAppStart(ar->arHifDevice, address));

        /* Apply the patches */
        AR6K_PATCH_DOWNLOAD_ADDRESS(address, ar->arVersion.target_ver);
        if ((ar6000_transfer_bin_file(ar, AR6K_PATCH_FILE, address, FALSE)) != A_OK) {
            return A_ERROR;
        }

        param = address;
        bmifn(BMIWriteMemory(ar->arHifDevice, HOST_INTEREST_ITEM_ADDRESS(ar, hi_dset_list_head), (A_UCHAR *)&param, 4));

        if (ar->arTargetType == TARGET_TYPE_AR6003) {
            if (ar->arVersion.target_ver == AR6003_REV1_VERSION) {
                /* Reserve 5.5K of RAM */
                param = 5632;
            } else { /* AR6003_REV2_VERSION */
                /* Reserve 6.5K of RAM */
                param = 6656;
            }
            bmifn(BMIWriteMemory(ar->arHifDevice, HOST_INTEREST_ITEM_ADDRESS(ar, hi_end_RAM_reserve_sz), (A_UCHAR *)&param, 4));
        }

        /* Restore system sleep */
        address = RTC_BASE_ADDRESS + SYSTEM_SLEEP_ADDRESS;
        bmifn(BMIWriteSOCRegister(ar->arHifDevice, address, sleep));

        address = MBOX_BASE_ADDRESS + LOCAL_SCRATCH_ADDRESS;
        param = options | 0x20;
        bmifn(BMIWriteSOCRegister(ar->arHifDevice, address, param));

        if (ar->arTargetType == TARGET_TYPE_AR6003) {
            /* Configure GPIO AR6003 UART */
#ifndef CONFIG_AR600x_DEBUG_UART_TX_PIN
#define CONFIG_AR600x_DEBUG_UART_TX_PIN 8
#endif
            param = CONFIG_AR600x_DEBUG_UART_TX_PIN;
            bmifn(BMIWriteMemory(ar->arHifDevice, HOST_INTEREST_ITEM_ADDRESS(ar, hi_dbg_uart_txpin), (A_UCHAR *)&param, 4));

#if (CONFIG_AR600x_DEBUG_UART_TX_PIN == 23)
            {
                address = GPIO_BASE_ADDRESS + CLOCK_GPIO_ADDRESS;
                bmifn(BMIReadSOCRegister(ar->arHifDevice, address, &param));
                param |= CLOCK_GPIO_BT_CLK_OUT_EN_SET(1);
                bmifn(BMIWriteSOCRegister(ar->arHifDevice, address, param));
            }
#endif

            /* Configure GPIO for BT Reset */
#ifdef ATH6KL_CONFIG_GPIO_BT_RESET
#define CONFIG_AR600x_BT_RESET_PIN	0x16
            param = CONFIG_AR600x_BT_RESET_PIN;
            bmifn(BMIWriteMemory(ar->arHifDevice, HOST_INTEREST_ITEM_ADDRESS(ar, hi_hci_uart_support_pins), (A_UCHAR *)&param, 4));
#endif /* ATH6KL_CONFIG_GPIO_BT_RESET */

            /* Configure UART flow control polarity */
#ifndef CONFIG_ATH6KL_BT_UART_FC_POLARITY
#define CONFIG_ATH6KL_BT_UART_FC_POLARITY 0
#endif

#if (CONFIG_ATH6KL_BT_UART_FC_POLARITY == 1)
            if (ar->arVersion.target_ver == AR6003_REV2_VERSION) {
                param = ((CONFIG_ATH6KL_BT_UART_FC_POLARITY << 1) & 0x2);
                bmifn(BMIWriteMemory(ar->arHifDevice, HOST_INTEREST_ITEM_ADDRESS(ar, hi_hci_uart_pwr_mgmt_params), (A_UCHAR *)&param, 4));
            }
#endif /* CONFIG_ATH6KL_BT_UART_FC_POLARITY */
        }

#ifdef HTC_RAW_INTERFACE
        if (!eppingtest && bypasswmi) {
            /* Don't run BMIDone for ART mode and force resetok=0 */
            resetok = 0;
            msleep(1000);
        }
#endif /* HTC_RAW_INTERFACE */

#endif /* INIT_MODE_DRV_ENABLED */
    }

    return A_OK;
}

A_STATUS
ar6000_configure_target(AR_SOFTC_T *ar)
{
    A_UINT32 param;
    if (enableuartprint) {
        param = 1;
        if (BMIWriteMemory(ar->arHifDevice,
                           HOST_INTEREST_ITEM_ADDRESS(ar, hi_serial_enable),
                           (A_UCHAR *)&param,
                           4)!= A_OK)
        {
             AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for enableuartprint failed \n"));
             return A_ERROR;
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("Serial console prints enabled\n"));
    }

    /* Tell target which HTC version it is used*/
    param = HTC_PROTOCOL_VERSION;
    if (BMIWriteMemory(ar->arHifDevice,
                       HOST_INTEREST_ITEM_ADDRESS(ar, hi_app_host_interest),
                       (A_UCHAR *)&param,
                       4)!= A_OK)
    {
         AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for htc version failed \n"));
         return A_ERROR;
    }

#ifdef CONFIG_HOST_TCMD_SUPPORT
    if(testmode) {
        ar->arTargetMode = AR6000_TCMD_MODE;
    }else {
        ar->arTargetMode = AR6000_WLAN_MODE;
    }
#endif
    if (enabletimerwar) {
        A_UINT32 param;

        if (BMIReadMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_option_flag),
            (A_UCHAR *)&param,
            4)!= A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIReadMemory for enabletimerwar failed \n"));
            return A_ERROR;
        }

        param |= HI_OPTION_TIMER_WAR;

        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_option_flag),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for enabletimerwar failed \n"));
            return A_ERROR;
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("Timer WAR enabled\n"));
    }

    /* set the firmware mode to STA/IBSS/AP */
    {
        A_UINT32 param;

        if (BMIReadMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_option_flag),
            (A_UCHAR *)&param,
            4)!= A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIReadMemory for setting fwmode failed \n"));
            return A_ERROR;
        }

        param |= (fwmode << HI_OPTION_FW_MODE_SHIFT);

        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_option_flag),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for setting fwmode failed \n"));
            return A_ERROR;
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("Firmware mode set\n"));
    }

#ifdef ATH6KL_DISABLE_TARGET_DBGLOGS
    {
        A_UINT32 param;

        if (BMIReadMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_option_flag),
            (A_UCHAR *)&param,
            4)!= A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIReadMemory for disabling debug logs failed\n"));
            return A_ERROR;
        }

        param |= HI_OPTION_DISABLE_DBGLOG;

        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_option_flag),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for HI_OPTION_DISABLE_DBGLOG\n"));
            return A_ERROR;
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("Firmware mode set\n"));
    }
#endif /* ATH6KL_DISABLE_TARGET_DBGLOGS */

    /* 
     * Hardcode the address use for the extended board data 
     * Ideally this should be pre-allocate by the OS at boot time
     * But since it is a new feature and board data is loaded 
     * at init time, we have to workaround this from host.
     * It is difficult to patch the firmware boot code,
     * but possible in theory.
     */
    if (ar->arTargetType == TARGET_TYPE_AR6003) {
        param = AR6003_BOARD_EXT_DATA_ADDRESS; 
        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_board_ext_data),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for hi_board_ext_data failed \n"));
            return A_ERROR;
        }
    }


        /* since BMIInit is called in the driver layer, we have to set the block
         * size here for the target */

    if (A_FAILED(ar6000_set_htc_params(ar->arHifDevice,
                                       ar->arTargetType,
                                       mbox_yield_limit,
                                       0 /* use default number of control buffers */
                                       ))) {
        return A_ERROR;
    }

    if (setupbtdev != 0) {
        if (A_FAILED(ar6000_set_hci_bridge_flags(ar->arHifDevice,
                                                 ar->arTargetType,
                                                 setupbtdev))) {
            return A_ERROR;
        }
    }
    return A_OK;
}

static void
init_netdev(struct net_device *dev, char *name)
{
    dev->netdev_ops = &ar6000_netdev_ops;
    dev->watchdog_timeo = AR6000_TX_TIMEOUT;
    dev->wireless_handlers = &ath_iw_handler_def;

    ath_iw_handler_def.get_wireless_stats = ar6000_get_iwstats; /*Displayed via proc fs */

   /*
    * We need the OS to provide us with more headroom in order to
    * perform dix to 802.3, WMI header encap, and the HTC header
    */
    if (processDot11Hdr) {
        dev->hard_header_len = sizeof(struct ieee80211_qosframe) + sizeof(ATH_LLC_SNAP_HDR) + sizeof(WMI_DATA_HDR) + HTC_HEADER_LEN + WMI_MAX_TX_META_SZ + LINUX_HACK_FUDGE_FACTOR;
    } else {
        dev->hard_header_len = ETH_HLEN + sizeof(ATH_LLC_SNAP_HDR) +
            sizeof(WMI_DATA_HDR) + HTC_HEADER_LEN + WMI_MAX_TX_META_SZ + LINUX_HACK_FUDGE_FACTOR;
    }

    if (name[0])
    {
        strcpy(dev->name, name);
    }

#ifdef SET_MODULE_OWNER
    SET_MODULE_OWNER(dev);
#endif

#ifdef CONFIG_CHECKSUM_OFFLOAD
    if(csumOffload){
        dev->features |= NETIF_F_IP_CSUM; /*advertise kernel capability to do TCP/UDP CSUM offload for IPV4*/
    }
#endif

    return;
}

/*
 * HTC Event handlers
 */
static A_STATUS
ar6000_avail_ev(void *context, void *hif_handle)
{
    int i;
    struct net_device *dev;
    void *ar_netif;
    AR_SOFTC_T *ar;
    int device_index = 0;
    HTC_INIT_INFO  htcInfo;
#ifdef ATH6K_CONFIG_CFG80211
    struct wireless_dev *wdev;
#endif /* ATH6K_CONFIG_CFG80211 */
    A_STATUS init_status = A_OK;

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("ar6000_available\n"));

    for (i=0; i < MAX_AR6000; i++) {
        if (ar6000_devices[i] == NULL) {
            break;
        }
    }

    if (i == MAX_AR6000) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ar6000_available: max devices reached\n"));
        return A_ERROR;
    }

    /* Save this. It gives a bit better readability especially since */
    /* we use another local "i" variable below.                      */
    device_index = i;

#ifdef ATH6K_CONFIG_CFG80211
    wdev = ar6k_cfg80211_init(NULL);
    if (IS_ERR(wdev)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("%s: ar6k_cfg80211_init failed\n", __func__));
        return A_ERROR;
    }
    ar_netif = wdev_priv(wdev);
#else
    dev = alloc_etherdev(sizeof(AR_SOFTC_T));
    if (dev == NULL) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ar6000_available: can't alloc etherdev\n"));
        return A_ERROR;
    }
    ether_setup(dev);
    ar_netif = ar6k_priv(dev);
#endif /* ATH6K_CONFIG_CFG80211 */

    if (ar_netif == NULL) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("%s: Can't allocate ar6k priv memory\n", __func__));
        return A_ERROR;
    }

    A_MEMZERO(ar_netif, sizeof(AR_SOFTC_T));
    ar = (AR_SOFTC_T *)ar_netif;

#ifdef ATH6K_CONFIG_CFG80211
    ar->wdev = wdev;
    wdev->iftype = NL80211_IFTYPE_STATION;

    dev = alloc_netdev_mq(0, "wlan%d", ether_setup, 1);
    if (!dev) {
        printk(KERN_CRIT "AR6K: no memory for network device instance\n");
        ar6k_cfg80211_deinit(ar);
        return A_ERROR;
    }

    dev->ieee80211_ptr = wdev;
    SET_NETDEV_DEV(dev, wiphy_dev(wdev->wiphy));
    wdev->netdev = dev;
    ar->arNetworkType = INFRA_NETWORK;
#endif /* ATH6K_CONFIG_CFG80211 */

    init_netdev(dev, ifname);

#ifdef SET_NETDEV_DEV
    if (ar_netif) { 
        HIF_DEVICE_OS_DEVICE_INFO osDevInfo;
        A_MEMZERO(&osDevInfo, sizeof(osDevInfo));
        if ( A_SUCCESS( HIFConfigureDevice(hif_handle, HIF_DEVICE_GET_OS_DEVICE,
                        &osDevInfo, sizeof(osDevInfo))) ) {
            SET_NETDEV_DEV(dev, osDevInfo.pOSDevice);
        }
    }
#endif 

    ar->arNetDev             = dev;
    ar->arHifDevice          = hif_handle;
    ar->arWlanState          = WLAN_ENABLED;
    ar->arDeviceIndex        = device_index;

    ar->arWlanPowerState     = WLAN_POWER_STATE_ON;
    ar->arWlanOff            = FALSE;   /* We are in ON state */
#ifdef CONFIG_PM
    ar->arWowState           = WLAN_WOW_STATE_NONE;
    ar->arBTOff              = TRUE;   /* BT chip assumed to be OFF */
    ar->arBTSharing          = WLAN_CONFIG_BT_SHARING; 
    ar->arWlanOffConfig      = WLAN_CONFIG_WLAN_OFF;
    ar->arSuspendConfig      = WLAN_CONFIG_PM_SUSPEND;
    ar->arWow2Config         = WLAN_CONFIG_PM_WOW2;
#endif /* CONFIG_PM */

    A_INIT_TIMER(&ar->arHBChallengeResp.timer, ar6000_detect_error, dev);
    ar->arHBChallengeResp.seqNum = 0;
    ar->arHBChallengeResp.outstanding = FALSE;
    ar->arHBChallengeResp.missCnt = 0;
    ar->arHBChallengeResp.frequency = AR6000_HB_CHALLENGE_RESP_FREQ_DEFAULT;
    ar->arHBChallengeResp.missThres = AR6000_HB_CHALLENGE_RESP_MISS_THRES_DEFAULT;

    ar6000_init_control_info(ar);
    init_waitqueue_head(&arEvent);
    sema_init(&ar->arSem, 1);
    ar->bIsDestroyProgress = FALSE;

    INIT_HTC_PACKET_QUEUE(&ar->amsdu_rx_buffer_queue);

#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
    A_INIT_TIMER(&aptcTimer, aptcTimerHandler, ar);
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

    A_INIT_TIMER(&ar->disconnect_timer, disconnect_timer_handler, dev);

    BMIInit();

    if (bmienable) {
        ar6000_sysfs_bmi_init(ar);
    }

    {
        struct bmi_target_info targ_info;

        if (BMIGetTargetInfo(ar->arHifDevice, &targ_info) != A_OK) {
            init_status = A_ERROR;
            goto avail_ev_failed;
        }

        ar->arVersion.target_ver = targ_info.target_ver;
        ar->arTargetType = targ_info.target_type;

            /* do any target-specific preparation that can be done through BMI */
        if (ar6000_prepare_target(ar->arHifDevice,
                                  targ_info.target_type,
                                  targ_info.target_ver) != A_OK) {
            init_status = A_ERROR;
            goto avail_ev_failed;
        }

    }

    if (ar6000_configure_target(ar) != A_OK) {
            init_status = A_ERROR;
            goto avail_ev_failed;
    }

    A_MEMZERO(&htcInfo,sizeof(htcInfo));
    htcInfo.pContext = ar;
    htcInfo.TargetFailure = ar6000_target_failure;

    ar->arHtcTarget = HTCCreate(ar->arHifDevice,&htcInfo);

    if (ar->arHtcTarget == NULL) {
        init_status = A_ERROR;
        goto avail_ev_failed;
    }

    spin_lock_init(&ar->arLock);

#ifdef WAPI_ENABLE
    ar->arWapiEnable = 0;
#endif


#ifdef CONFIG_CHECKSUM_OFFLOAD
    if(csumOffload){
        /*if external frame work is also needed, change and use an extended rxMetaVerion*/
        ar->rxMetaVersion=WMI_META_VERSION_2;
    }
#endif

#ifdef ATH_AR6K_11N_SUPPORT
    if((ar->aggr_cntxt = aggr_init(ar6000_alloc_netbufs)) == NULL) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s() Failed to initialize aggr.\n", __func__));
            init_status = A_ERROR;
            goto avail_ev_failed;
    }

    aggr_register_rx_dispatcher(ar->aggr_cntxt, (void *)dev, ar6000_deliver_frames_to_nw_stack);
#endif

    HIFClaimDevice(ar->arHifDevice, ar);

    /* We only register the device in the global list if we succeed. */
    /* If the device is in the global list, it will be destroyed     */
    /* when the module is unloaded.                                  */
    ar6000_devices[device_index] = dev;

    /* Don't install the init function if BMI is requested */
    if (!bmienable) {
        ar6000_netdev_ops.ndo_init = ar6000_init;
    } else {
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("BMI enabled: %d\n", wlaninitmode));
        if ((wlaninitmode == WLAN_INIT_MODE_UDEV) ||
            (wlaninitmode == WLAN_INIT_MODE_DRV))
        {
            A_STATUS status = A_OK;
            do {
                if ((status = ar6000_sysfs_bmi_get_config(ar, wlaninitmode)) != A_OK)
                {
                    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ar6000_avail: ar6000_sysfs_bmi_get_config failed\n"));
                    break;
                }
#ifdef HTC_RAW_INTERFACE
                break; /* Don't call ar6000_init for ART */
#endif 
                rtnl_lock();
                status = (ar6000_init(dev)==0) ? A_OK : A_ERROR;
                rtnl_unlock();
                if (status != A_OK) {
                    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ar6000_avail: ar6000_init\n"));
                }
            } while (FALSE);

            if (status != A_OK) {
                init_status = status;
                goto avail_ev_failed;
            }
        }
    }

    /* This runs the init function if registered */
    if (register_netdev(dev)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ar6000_avail: register_netdev failed\n"));
        ar6000_destroy(dev, 0);
        return A_ERROR;
    }

	is_netdev_registered = 1;

#ifdef CONFIG_AP_VIRTUAL_ADAPTER_SUPPORT
    arApNetDev = NULL;
#endif /* CONFIG_AP_VIRTUAL_ADAPTER_SUPPORT */
    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("ar6000_avail: name=%s hifdevice=0x%lx, dev=0x%lx (%d), ar=0x%lx\n",
                    dev->name, (unsigned long)ar->arHifDevice, (unsigned long)dev, device_index,
                    (unsigned long)ar));

avail_ev_failed :
    if (A_FAILED(init_status)) {
        if (bmienable) { 
            ar6000_sysfs_bmi_deinit(ar);  
        }
    }

    return init_status;
}

static void ar6000_target_failure(void *Instance, A_STATUS Status)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)Instance;
    WMI_TARGET_ERROR_REPORT_EVENT errEvent;
    static A_BOOL sip = FALSE;

    if (Status != A_OK) {

        printk(KERN_ERR "ar6000_target_failure: target asserted \n");

        if (timer_pending(&ar->arHBChallengeResp.timer)) {
            A_UNTIMEOUT(&ar->arHBChallengeResp.timer);
        }

        /* try dumping target assertion information (if any) */
        ar6000_dump_target_assert_info(ar->arHifDevice,ar->arTargetType);

        /*
         * Fetch the logs from the target via the diagnostic
         * window.
         */
        ar6000_dbglog_get_debug_logs(ar);

        /* Report the error only once */
        if (!sip) {
            sip = TRUE;
            errEvent.errorVal = WMI_TARGET_COM_ERR |
                                WMI_TARGET_FATAL_ERR;
            ar6000_send_event_to_app(ar, WMI_ERROR_REPORT_EVENTID,
                                     (A_UINT8 *)&errEvent,
                                     sizeof(WMI_TARGET_ERROR_REPORT_EVENT));
        }
    }
}

static A_STATUS
ar6000_unavail_ev(void *context, void *hif_handle)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)context;
        /* NULL out it's entry in the global list */
    ar6000_devices[ar->arDeviceIndex] = NULL;
    ar6000_destroy(ar->arNetDev, 1);

    return A_OK;
}

void
ar6000_restart_endpoint(struct net_device *dev)
{
    A_STATUS status = A_OK;
    AR_SOFTC_T *ar = (AR_SOFTC_T *)ar6k_priv(dev);

    BMIInit();
    do {
        if ( (status=ar6000_configure_target(ar))!=A_OK)
            break;
        if ( (status=ar6000_sysfs_bmi_get_config(ar, wlaninitmode)) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ar6000_avail: ar6000_sysfs_bmi_get_config failed\n"));
            break;
        }
        rtnl_lock();
        status = (ar6000_init(dev)==0) ? A_OK : A_ERROR;
        rtnl_unlock();

        if (status!=A_OK) {
            break;
        }
        if (ar->arSsidLen && ar->arWlanState == WLAN_ENABLED) {
            ar6000_connect_to_ap(ar);
        }  
    } while (0);

    if (status==A_OK) {
        return;
    }

    ar6000_devices[ar->arDeviceIndex] = NULL;
    ar6000_destroy(ar->arNetDev, 1);
}

void
ar6000_stop_endpoint(struct net_device *dev, A_BOOL keepprofile, A_BOOL getdbglogs)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)ar6k_priv(dev);

    /* Stop the transmit queues */
    netif_stop_queue(dev);

    /* Disable the target and the interrupts associated with it */
    if (ar->arWmiReady == TRUE)
    {
        if (!bypasswmi)
        {
            if (ar->arConnected == TRUE || ar->arConnectPending == TRUE)
            {
                AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s(): Disconnect\n", __func__));
                if (!keepprofile) {
                    AR6000_SPIN_LOCK(&ar->arLock, 0);
                    ar6000_init_profile_info(ar);
                    AR6000_SPIN_UNLOCK(&ar->arLock, 0);
                }
                wmi_disconnect_cmd(ar->arWmi);
            }

            A_UNTIMEOUT(&ar->disconnect_timer);

            if (getdbglogs) {
                ar6000_dbglog_get_debug_logs(ar);
            }

            ar->arWmiReady  = FALSE;
            wmi_shutdown(ar->arWmi);
            ar->arWmiEnabled = FALSE;
            ar->arWmi = NULL;
            /* 
             * After wmi_shudown all WMI events will be dropped.
             * We need to cleanup the buffers allocated in AP mode
             * and give disconnect notification to stack, which usually
             * happens in the disconnect_event. 
             * Simulate the disconnect_event by calling the function directly.
             * Sometimes disconnect_event will be received when the debug logs 
             * are collected.
             */
            if (ar->arConnected == TRUE || ar->arConnectPending == TRUE) {
                if(ar->arNetworkType & AP_NETWORK) {
                    ar6000_disconnect_event(ar, DISCONNECT_CMD, bcast_mac, 0, NULL, 0);
                } else {
                    ar6000_disconnect_event(ar, DISCONNECT_CMD, ar->arBssid, 0, NULL, 0);
                }
                ar->arConnected = FALSE;
                ar->arConnectPending = FALSE;
            }
#ifdef USER_KEYS
            ar->user_savedkeys_stat = USER_SAVEDKEYS_STAT_INIT;
            ar->user_key_ctrl      = 0;
#endif
        }

         AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s(): WMI stopped\n", __func__));
    }
    else
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s(): WMI not ready 0x%lx 0x%lx\n",
            __func__, (unsigned long) ar, (unsigned long) ar->arWmi));

        /* Shut down WMI if we have started it */
        if(ar->arWmiEnabled == TRUE)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("%s(): Shut down WMI\n", __func__));
            wmi_shutdown(ar->arWmi);
            ar->arWmiEnabled = FALSE;
            ar->arWmi = NULL;
        }
    }

    if (ar->arHtcTarget != NULL) {
#ifdef EXPORT_HCI_BRIDGE_INTERFACE
        if (NULL != ar6kHciTransCallbacks.cleanupTransport) {
            ar6kHciTransCallbacks.cleanupTransport(NULL);
        }
#else
        // FIXME: workaround to reset BT's UART baud rate to default
        if (NULL != ar->exitCallback) {
            AR3K_CONFIG_INFO ar3kconfig;
            A_STATUS status;

            A_MEMZERO(&ar3kconfig,sizeof(ar3kconfig));
            ar6000_set_default_ar3kconfig(ar, (void *)&ar3kconfig);
            status = ar->exitCallback(&ar3kconfig);
            if (A_OK != status) {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Failed to reset AR3K baud rate! \n"));
            }
        }
        // END workaround
        if (setuphci)
        	ar6000_cleanup_hci(ar);
#endif
#ifdef EXPORT_HCI_PAL_INTERFACE
        if (setuphcipal && (NULL != ar6kHciPalCallbacks_g.cleanupTransport)) {
           ar6kHciPalCallbacks_g.cleanupTransport(ar);
        }
#else
				/* cleanup hci pal driver data structures */
        if(setuphcipal)
          ar6k_cleanup_hci_pal(ar);
#endif
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,(" Shutting down HTC .... \n"));
        /* stop HTC */
        HTCStop(ar->arHtcTarget);
    }

    if (resetok) {
        /* try to reset the device if we can
         * The driver may have been configure NOT to reset the target during
         * a debug session */
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,(" Attempting to reset target on instance destroy.... \n"));
        if (ar->arHifDevice != NULL) {
            A_BOOL coldReset = (ar->arTargetType == TARGET_TYPE_AR6003) ? TRUE: FALSE;
            ar6000_reset_device(ar->arHifDevice, ar->arTargetType, TRUE, coldReset);
        }
    } else {
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,(" Host does not want target reset. \n"));
    }
       /* Done with cookies */
    ar6000_cookie_cleanup(ar);
}
/*
 * We need to differentiate between the surprise and planned removal of the
 * device because of the following consideration:
 * - In case of surprise removal, the hcd already frees up the pending
 *   for the device and hence there is no need to unregister the function
 *   driver inorder to get these requests. For planned removal, the function
 *   driver has to explictly unregister itself to have the hcd return all the
 *   pending requests before the data structures for the devices are freed up.
 *   Note that as per the current implementation, the function driver will
 *   end up releasing all the devices since there is no API to selectively
 *   release a particular device.
 * - Certain commands issued to the target can be skipped for surprise
 *   removal since they will anyway not go through.
 */
void
ar6000_destroy(struct net_device *dev, unsigned int unregister)
{
    AR_SOFTC_T *ar;

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("+ar6000_destroy \n"));
    
    if((dev == NULL) || ((ar = ar6k_priv(dev)) == NULL))
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s(): Failed to get device structure.\n", __func__));
        return;
    }

    ar->bIsDestroyProgress = TRUE;

    if (down_interruptible(&ar->arSem)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s(): down_interruptible failed \n", __func__));
        return;
    }

    if (ar->arWlanPowerState != WLAN_POWER_STATE_CUT_PWR) {
        /* only stop endpoint if we are not stop it in suspend_ev */
        ar6000_stop_endpoint(dev, FALSE, TRUE);
    } else {
        /* clear up the platform power state before rmmod */
        plat_setup_power(1,0);
    }

    ar->arWlanState = WLAN_DISABLED;
    if (ar->arHtcTarget != NULL) {
        /* destroy HTC */
        HTCDestroy(ar->arHtcTarget);
    }
    if (ar->arHifDevice != NULL) {
        /*release the device so we do not get called back on remove incase we
         * we're explicity destroyed by module unload */
        HIFReleaseDevice(ar->arHifDevice);
        HIFShutDownDevice(ar->arHifDevice);
    }
#ifdef ATH_AR6K_11N_SUPPORT
    aggr_module_destroy(ar->aggr_cntxt);
#endif

       /* Done with cookies */
    ar6000_cookie_cleanup(ar);

        /* cleanup any allocated AMSDU buffers */
    ar6000_cleanup_amsdu_rxbufs(ar);

    if (bmienable) {
        ar6000_sysfs_bmi_deinit(ar);
    }

    /* Cleanup BMI */
    BMICleanup();

    /* Clear the tx counters */
    memset(tx_attempt, 0, sizeof(tx_attempt));
    memset(tx_post, 0, sizeof(tx_post));
    memset(tx_complete, 0, sizeof(tx_complete));

#ifdef HTC_RAW_INTERFACE
    if (ar->arRawHtc) {
        A_FREE(ar->arRawHtc);
        ar->arRawHtc = NULL;
    }
#endif 
    /* Free up the device data structure */
    if (unregister && is_netdev_registered) {		
        unregister_netdev(dev);
        is_netdev_registered = 0;
    }
    free_netdev(dev);

#ifdef ATH6K_CONFIG_CFG80211
    ar6k_cfg80211_deinit(ar);
#endif /* ATH6K_CONFIG_CFG80211 */

#ifdef CONFIG_AP_VIRTUL_ADAPTER_SUPPORT
    ar6000_remove_ap_interface();
#endif /*CONFIG_AP_VIRTUAL_ADAPTER_SUPPORT */

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("-ar6000_destroy \n"));
}

static void disconnect_timer_handler(unsigned long ptr)
{
    struct net_device *dev = (struct net_device *)ptr;
    AR_SOFTC_T *ar = (AR_SOFTC_T *)ar6k_priv(dev);

    A_UNTIMEOUT(&ar->disconnect_timer);

    ar6000_init_profile_info(ar);
    wmi_disconnect_cmd(ar->arWmi);
}

static void ar6000_detect_error(unsigned long ptr)
{
    struct net_device *dev = (struct net_device *)ptr;
    AR_SOFTC_T *ar = (AR_SOFTC_T *)ar6k_priv(dev);
    WMI_TARGET_ERROR_REPORT_EVENT errEvent;

    AR6000_SPIN_LOCK(&ar->arLock, 0);

    if (ar->arHBChallengeResp.outstanding) {
        ar->arHBChallengeResp.missCnt++;
    } else {
        ar->arHBChallengeResp.missCnt = 0;
    }

    if (ar->arHBChallengeResp.missCnt > ar->arHBChallengeResp.missThres) {
        /* Send Error Detect event to the application layer and do not reschedule the error detection module timer */
        ar->arHBChallengeResp.missCnt = 0;
        ar->arHBChallengeResp.seqNum = 0;
        errEvent.errorVal = WMI_TARGET_COM_ERR | WMI_TARGET_FATAL_ERR;
        AR6000_SPIN_UNLOCK(&ar->arLock, 0);
        ar6000_send_event_to_app(ar, WMI_ERROR_REPORT_EVENTID,
                                 (A_UINT8 *)&errEvent,
                                 sizeof(WMI_TARGET_ERROR_REPORT_EVENT));
        return;
    }

    /* Generate the sequence number for the next challenge */
    ar->arHBChallengeResp.seqNum++;
    ar->arHBChallengeResp.outstanding = TRUE;

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    /* Send the challenge on the control channel */
    if (wmi_get_challenge_resp_cmd(ar->arWmi, ar->arHBChallengeResp.seqNum, DRV_HB_CHALLENGE) != A_OK) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Unable to send heart beat challenge\n"));
    }


    /* Reschedule the timer for the next challenge */
    A_TIMEOUT_MS(&ar->arHBChallengeResp.timer, ar->arHBChallengeResp.frequency * 1000, 0);
}

void ar6000_init_profile_info(AR_SOFTC_T *ar)
{
    ar->arSsidLen            = 0;
    A_MEMZERO(ar->arSsid, sizeof(ar->arSsid));

    switch(fwmode) {
        case HI_OPTION_FW_MODE_IBSS:
            ar->arNetworkType = ar->arNextMode = ADHOC_NETWORK;
            break;
        case HI_OPTION_FW_MODE_BSS_STA:
            ar->arNetworkType = ar->arNextMode = INFRA_NETWORK;
            break;
        case HI_OPTION_FW_MODE_AP:
            ar->arNetworkType = ar->arNextMode = AP_NETWORK;
            break;
    }

    ar->arDot11AuthMode      = OPEN_AUTH;
    ar->arAuthMode           = NONE_AUTH;
    ar->arPairwiseCrypto     = NONE_CRYPT;
    ar->arPairwiseCryptoLen  = 0;
    ar->arGroupCrypto        = NONE_CRYPT;
    ar->arGroupCryptoLen     = 0;
    A_MEMZERO(ar->arWepKeyList, sizeof(ar->arWepKeyList));
    A_MEMZERO(ar->arReqBssid, sizeof(ar->arReqBssid));
    A_MEMZERO(ar->arBssid, sizeof(ar->arBssid));
    ar->arBssChannel = 0;
    ar->arConnected = FALSE;
}

static void
ar6000_init_control_info(AR_SOFTC_T *ar)
{
    ar->arWmiEnabled         = FALSE;
    ar6000_init_profile_info(ar);
    ar->arDefTxKeyIndex      = 0;
    A_MEMZERO(ar->arWepKeyList, sizeof(ar->arWepKeyList));
    ar->arChannelHint        = 0;
    ar->arListenIntervalT    = A_DEFAULT_LISTEN_INTERVAL;
    ar->arListenIntervalB    = 0;
    ar->arVersion.host_ver   = AR6K_SW_VERSION;
    ar->arRssi               = 0;
    ar->arTxPwr              = 0;
    ar->arTxPwrSet           = FALSE;
    ar->arSkipScan           = 0;
    ar->arBeaconInterval     = 0;
    ar->arBitRate            = 0;
    ar->arMaxRetries         = 0;
    ar->arWmmEnabled         = TRUE;
    ar->intra_bss            = 1;
    ar->scan_triggered       = 0;
    A_MEMZERO(&ar->scParams, sizeof(ar->scParams));
    ar->scParams.shortScanRatio = WMI_SHORTSCANRATIO_DEFAULT;
    ar->scParams.scanCtrlFlags = DEFAULT_SCAN_CTRL_FLAGS;

    /* Initialize the AP mode state info */
    {
        A_UINT8 ctr;
        A_MEMZERO((A_UINT8 *)ar->sta_list, AP_MAX_NUM_STA * sizeof(sta_t));

        /* init the Mutexes */
        A_MUTEX_INIT(&ar->mcastpsqLock);

        /* Init the PS queues */
        for (ctr=0; ctr < AP_MAX_NUM_STA ; ctr++) {
            A_MUTEX_INIT(&ar->sta_list[ctr].psqLock);
            A_NETBUF_QUEUE_INIT(&ar->sta_list[ctr].psq);
        }

        ar->ap_profile_flag = 0;
        A_NETBUF_QUEUE_INIT(&ar->mcastpsq);

        A_MEMCPY(ar->ap_country_code, DEF_AP_COUNTRY_CODE, 3);
        ar->ap_wmode = DEF_AP_WMODE_G;
        ar->ap_dtim_period = DEF_AP_DTIM;
        ar->ap_beacon_interval = DEF_BEACON_INTERVAL;
    }
}

static int
ar6000_open(struct net_device *dev)
{
    unsigned long  flags;
    AR_SOFTC_T    *ar = (AR_SOFTC_T *)ar6k_priv(dev);

    spin_lock_irqsave(&ar->arLock, flags);

#ifdef ATH6K_CONFIG_CFG80211
    if(ar->arWlanState == WLAN_DISABLED) {
        ar->arWlanState = WLAN_ENABLED;
    }
#endif /* ATH6K_CONFIG_CFG80211 */

    if( ar->arConnected || bypasswmi) {
        netif_carrier_on(dev);
        /* Wake up the queues */
        netif_wake_queue(dev);
    }
    else
        netif_carrier_off(dev);

    spin_unlock_irqrestore(&ar->arLock, flags);
    return 0;
}

static int
ar6000_close(struct net_device *dev)
{
#ifdef ATH6K_CONFIG_CFG80211
    AR_SOFTC_T    *ar = (AR_SOFTC_T *)ar6k_priv(dev);
#endif /* ATH6K_CONFIG_CFG80211 */
    netif_stop_queue(dev);

#ifdef ATH6K_CONFIG_CFG80211
    AR6000_SPIN_LOCK(&ar->arLock, 0);
    if (ar->arConnected == TRUE || ar->arConnectPending == TRUE) {
        AR6000_SPIN_UNLOCK(&ar->arLock, 0);
        wmi_disconnect_cmd(ar->arWmi);
    } else {
        AR6000_SPIN_UNLOCK(&ar->arLock, 0);
    }

    if(ar->arWmiReady == TRUE) {
        if (wmi_scanparams_cmd(ar->arWmi, 0xFFFF, 0,
                               0, 0, 0, 0, 0, 0, 0, 0) != A_OK) {
            return -EIO;
        }
        ar->arWlanState = WLAN_DISABLED;
    }
#endif /* ATH6K_CONFIG_CFG80211 */

    return 0;
}

/* connect to a service */
static A_STATUS ar6000_connectservice(AR_SOFTC_T               *ar,
                                      HTC_SERVICE_CONNECT_REQ  *pConnect,
                                      char                     *pDesc)
{
    A_STATUS                 status;
    HTC_SERVICE_CONNECT_RESP response;

    do {

        A_MEMZERO(&response,sizeof(response));

        status = HTCConnectService(ar->arHtcTarget,
                                   pConnect,
                                   &response);

        if (A_FAILED(status)) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,(" Failed to connect to %s service status:%d \n",
                              pDesc, status));
            break;
        }
        switch (pConnect->ServiceID) {
            case WMI_CONTROL_SVC :
                if (ar->arWmiEnabled) {
                        /* set control endpoint for WMI use */
                    wmi_set_control_ep(ar->arWmi, response.Endpoint);
                }
                    /* save EP for fast lookup */
                ar->arControlEp = response.Endpoint;
                break;
            case WMI_DATA_BE_SVC :
                arSetAc2EndpointIDMap(ar, WMM_AC_BE, response.Endpoint);
                break;
            case WMI_DATA_BK_SVC :
                arSetAc2EndpointIDMap(ar, WMM_AC_BK, response.Endpoint);
                break;
            case WMI_DATA_VI_SVC :
                arSetAc2EndpointIDMap(ar, WMM_AC_VI, response.Endpoint);
                 break;
           case WMI_DATA_VO_SVC :
                arSetAc2EndpointIDMap(ar, WMM_AC_VO, response.Endpoint);
                break;
           default:
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ServiceID not mapped %d\n", pConnect->ServiceID));
                status = A_EINVAL;
            break;
        }

    } while (FALSE);

    return status;
}

void ar6000_TxDataCleanup(AR_SOFTC_T *ar)
{
        /* flush all the data (non-control) streams
         * we only flush packets that are tagged as data, we leave any control packets that
         * were in the TX queues alone */
    HTCFlushEndpoint(ar->arHtcTarget,
                     arAc2EndpointID(ar, WMM_AC_BE),
                     AR6K_DATA_PKT_TAG);
    HTCFlushEndpoint(ar->arHtcTarget,
                     arAc2EndpointID(ar, WMM_AC_BK),
                     AR6K_DATA_PKT_TAG);
    HTCFlushEndpoint(ar->arHtcTarget,
                     arAc2EndpointID(ar, WMM_AC_VI),
                     AR6K_DATA_PKT_TAG);
    HTCFlushEndpoint(ar->arHtcTarget,
                     arAc2EndpointID(ar, WMM_AC_VO),
                     AR6K_DATA_PKT_TAG);
}

HTC_ENDPOINT_ID
ar6000_ac2_endpoint_id ( void * devt, A_UINT8 ac)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;
    return(arAc2EndpointID(ar, ac));
}

A_UINT8
ar6000_endpoint_id2_ac(void * devt, HTC_ENDPOINT_ID ep )
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;
    return(arEndpoint2Ac(ar, ep ));
}

/* This function does one time initialization for the lifetime of the device */
int ar6000_init(struct net_device *dev)
{
    AR_SOFTC_T *ar;
    A_STATUS    status;
    A_INT32     timeleft;
    A_INT16     i;
    int         ret = 0;
#if defined(INIT_MODE_DRV_ENABLED) && defined(ENABLE_COEXISTENCE)
    WMI_SET_BTCOEX_COLOCATED_BT_DEV_CMD sbcb_cmd;
    WMI_SET_BTCOEX_FE_ANT_CMD sbfa_cmd;
#endif /* INIT_MODE_DRV_ENABLED && ENABLE_COEXISTENCE */

    if((ar = ar6k_priv(dev)) == NULL)
    {
        return -EIO;
    }

    if (wlaninitmode == WLAN_INIT_MODE_USR || wlaninitmode == WLAN_INIT_MODE_DRV) {
    
        ar6000_update_bdaddr(ar);

        if (enablerssicompensation) {
            ar6000_copy_cust_data_from_target(ar->arHifDevice, ar->arTargetType);
            read_rssi_compensation_param(ar);
            for (i=-95; i<=0; i++) {
                rssi_compensation_table[0-i] = rssi_compensation_calc(ar,i);
            }
        }
    }

    dev_hold(dev);
    rtnl_unlock();

    /* Do we need to finish the BMI phase */
    if ((wlaninitmode == WLAN_INIT_MODE_USR || wlaninitmode == WLAN_INIT_MODE_DRV) && 
        (BMIDone(ar->arHifDevice) != A_OK))
    {
        ret = -EIO;
        goto ar6000_init_done;
    }

    if (!bypasswmi)
    {
#if 0 /* TBDXXX */
        if (ar->arVersion.host_ver != ar->arVersion.target_ver) {
            A_PRINTF("WARNING: Host version 0x%x does not match Target "
                    " version 0x%x!\n",
                    ar->arVersion.host_ver, ar->arVersion.target_ver);
        }
#endif

        /* Indicate that WMI is enabled (although not ready yet) */
        ar->arWmiEnabled = TRUE;
        if ((ar->arWmi = wmi_init((void *) ar)) == NULL)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s() Failed to initialize WMI.\n", __func__));
            ret = -EIO;
            goto ar6000_init_done;
        }

        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s() Got WMI @ 0x%lx.\n", __func__,
            (unsigned long) ar->arWmi));
    }

    do {
        HTC_SERVICE_CONNECT_REQ connect;

            /* the reason we have to wait for the target here is that the driver layer
             * has to init BMI in order to set the host block size,
             */
        status = HTCWaitTarget(ar->arHtcTarget);

        if (A_FAILED(status)) {
            break;
        }

        A_MEMZERO(&connect,sizeof(connect));
            /* meta data is unused for now */
        connect.pMetaData = NULL;
        connect.MetaDataLength = 0;
            /* these fields are the same for all service endpoints */
        connect.EpCallbacks.pContext = ar;
        connect.EpCallbacks.EpTxCompleteMultiple = ar6000_tx_complete;
        connect.EpCallbacks.EpRecv = ar6000_rx;
        connect.EpCallbacks.EpRecvRefill = ar6000_rx_refill;
        connect.EpCallbacks.EpSendFull = ar6000_tx_queue_full;
            /* set the max queue depth so that our ar6000_tx_queue_full handler gets called.
             * Linux has the peculiarity of not providing flow control between the
             * NIC and the network stack. There is no API to indicate that a TX packet
             * was sent which could provide some back pressure to the network stack.
             * Under linux you would have to wait till the network stack consumed all sk_buffs
             * before any back-flow kicked in. Which isn't very friendly.
             * So we have to manage this ourselves */
        connect.MaxSendQueueDepth = MAX_DEFAULT_SEND_QUEUE_DEPTH;
        connect.EpCallbacks.RecvRefillWaterMark = AR6000_MAX_RX_BUFFERS / 4; /* set to 25 % */
        if (0 == connect.EpCallbacks.RecvRefillWaterMark) {
            connect.EpCallbacks.RecvRefillWaterMark++;
        }
            /* connect to control service */
        connect.ServiceID = WMI_CONTROL_SVC;
        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI CONTROL");
        if (A_FAILED(status)) {
            break;
        }

        connect.LocalConnectionFlags |= HTC_LOCAL_CONN_FLAGS_ENABLE_SEND_BUNDLE_PADDING;
            /* limit the HTC message size on the send path, although we can receive A-MSDU frames of
             * 4K, we will only send ethernet-sized (802.3) frames on the send path. */
        connect.MaxSendMsgSize = WMI_MAX_TX_DATA_FRAME_LENGTH;

            /* to reduce the amount of committed memory for larger A_MSDU frames, use the recv-alloc threshold
             * mechanism for larger packets */
        connect.EpCallbacks.RecvAllocThreshold = AR6000_BUFFER_SIZE;
        connect.EpCallbacks.EpRecvAllocThresh = ar6000_alloc_amsdu_rxbuf;

            /* for the remaining data services set the connection flag to reduce dribbling,
             * if configured to do so */
        if (reduce_credit_dribble) {
            connect.ConnectionFlags |= HTC_CONNECT_FLAGS_REDUCE_CREDIT_DRIBBLE;
            /* the credit dribble trigger threshold is (reduce_credit_dribble - 1) for a value
             * of 0-3 */
            connect.ConnectionFlags &= ~HTC_CONNECT_FLAGS_THRESHOLD_LEVEL_MASK;
            connect.ConnectionFlags |=
                        ((A_UINT16)reduce_credit_dribble - 1) & HTC_CONNECT_FLAGS_THRESHOLD_LEVEL_MASK;
        }
            /* connect to best-effort service */
        connect.ServiceID = WMI_DATA_BE_SVC;

        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI DATA BE");
        if (A_FAILED(status)) {
            break;
        }

            /* connect to back-ground
             * map this to WMI LOW_PRI */
        connect.ServiceID = WMI_DATA_BK_SVC;
        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI DATA BK");
        if (A_FAILED(status)) {
            break;
        }

            /* connect to Video service, map this to
             * to HI PRI */
        connect.ServiceID = WMI_DATA_VI_SVC;
        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI DATA VI");
        if (A_FAILED(status)) {
            break;
        }

            /* connect to VO service, this is currently not
             * mapped to a WMI priority stream due to historical reasons.
             * WMI originally defined 3 priorities over 3 mailboxes
             * We can change this when WMI is reworked so that priorities are not
             * dependent on mailboxes */
        connect.ServiceID = WMI_DATA_VO_SVC;
        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI DATA VO");
        if (A_FAILED(status)) {
            break;
        }

        A_ASSERT(arAc2EndpointID(ar,WMM_AC_BE) != 0);
        A_ASSERT(arAc2EndpointID(ar,WMM_AC_BK) != 0);
        A_ASSERT(arAc2EndpointID(ar,WMM_AC_VI) != 0);
        A_ASSERT(arAc2EndpointID(ar,WMM_AC_VO) != 0);

            /* setup access class priority mappings */
        ar->arAcStreamPriMap[WMM_AC_BK] = 0; /* lowest  */
        ar->arAcStreamPriMap[WMM_AC_BE] = 1; /*         */
        ar->arAcStreamPriMap[WMM_AC_VI] = 2; /*         */
        ar->arAcStreamPriMap[WMM_AC_VO] = 3; /* highest */

#ifdef EXPORT_HCI_BRIDGE_INTERFACE
        if (setuphci && (NULL != ar6kHciTransCallbacks.setupTransport)) {
            HCI_TRANSPORT_MISC_HANDLES hciHandles;

            hciHandles.netDevice = ar->arNetDev;
            hciHandles.hifDevice = ar->arHifDevice;
            hciHandles.htcHandle = ar->arHtcTarget;
            status = (A_STATUS)(ar6kHciTransCallbacks.setupTransport(&hciHandles));
        }
#else
        if (setuphci) {
                /* setup HCI */
            status = ar6000_setup_hci(ar);
        }
#endif
#ifdef EXPORT_HCI_PAL_INTERFACE
        if (setuphcipal && (NULL != ar6kHciPalCallbacks_g.setupTransport))
          status = ar6kHciPalCallbacks_g.setupTransport(ar);
#else
        if(setuphcipal)
          status = ar6k_setup_hci_pal(ar);
#endif

    } while (FALSE);

    if (A_FAILED(status)) {
        ret = -EIO;
        goto ar6000_init_done;
    }

    /*
     * give our connected endpoints some buffers
     */

    ar6000_rx_refill(ar, ar->arControlEp);
    ar6000_rx_refill(ar, arAc2EndpointID(ar,WMM_AC_BE));

    /*
     * We will post the receive buffers only for SPE or endpoint ping testing so we are
     * making it conditional on the 'bypasswmi' flag.
     */
    if (bypasswmi) {
        ar6000_rx_refill(ar,arAc2EndpointID(ar,WMM_AC_BK));
        ar6000_rx_refill(ar,arAc2EndpointID(ar,WMM_AC_VI));
        ar6000_rx_refill(ar,arAc2EndpointID(ar,WMM_AC_VO));
    }

    /* allocate some buffers that handle larger AMSDU frames */
    ar6000_refill_amsdu_rxbufs(ar,AR6000_MAX_AMSDU_RX_BUFFERS);

        /* setup credit distribution */
    ar6000_setup_credit_dist(ar->arHtcTarget, &ar->arCreditStateInfo);

    /* Since cookies are used for HTC transports, they should be */
    /* initialized prior to enabling HTC.                        */
    ar6000_cookie_init(ar);

    /* start HTC */
    status = HTCStart(ar->arHtcTarget);

    if (status != A_OK) {
        if (ar->arWmiEnabled == TRUE) {
            wmi_shutdown(ar->arWmi);
            ar->arWmiEnabled = FALSE;
            ar->arWmi = NULL;
        }
        ar6000_cookie_cleanup(ar);
        ret = -EIO;
        goto ar6000_init_done;
    }

    if (!bypasswmi) {
        /* Wait for Wmi event to be ready */
        timeleft = wait_event_interruptible_timeout(arEvent,
            (ar->arWmiReady == TRUE), wmitimeout * HZ);

        if (ar->arVersion.abi_ver != AR6K_ABI_VERSION) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ABI Version mismatch: Host(0x%x), Target(0x%x)\n", AR6K_ABI_VERSION, ar->arVersion.abi_ver));
#ifndef ATH6K_SKIP_ABI_VERSION_CHECK
            ret = -EIO;
            goto ar6000_init_done;
#endif /* ATH6K_SKIP_ABI_VERSION_CHECK */
        }

        if(!timeleft || signal_pending(current))
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("WMI is not ready or wait was interrupted\n"));
            ret = -EIO;
            goto ar6000_init_done;
        }

        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s() WMI is ready\n", __func__));

        /* Communicate the wmi protocol verision to the target */
        if ((ar6000_set_host_app_area(ar)) != A_OK) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Unable to set the host app area\n"));
        }

        /* configure the device for rx dot11 header rules 0,0 are the default values
         * therefore this command can be skipped if the inputs are 0,FALSE,FALSE.Required
         if checksum offload is needed. Set RxMetaVersion to 2*/
        if ((wmi_set_rx_frame_format_cmd(ar->arWmi,ar->rxMetaVersion, processDot11Hdr, processDot11Hdr)) != A_OK) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Unable to set the rx frame format.\n"));
        }

#if defined(INIT_MODE_DRV_ENABLED) && defined(ENABLE_COEXISTENCE)
        /* Configure the type of BT collocated with WLAN */
        A_MEMZERO(&sbcb_cmd, sizeof(WMI_SET_BTCOEX_COLOCATED_BT_DEV_CMD));
#ifdef CONFIG_AR600x_BT_QCOM
        sbcb_cmd.btcoexCoLocatedBTdev = 1;
#elif defined(CONFIG_AR600x_BT_CSR)
        sbcb_cmd.btcoexCoLocatedBTdev = 2;
#elif defined(CONFIG_AR600x_BT_AR3001)
        sbcb_cmd.btcoexCoLocatedBTdev = 3;
#else
#error Unsupported Bluetooth Type
#endif /* Collocated Bluetooth Type */

        if ((wmi_set_btcoex_colocated_bt_dev_cmd(ar->arWmi, &sbcb_cmd)) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Unable to set collocated BT type\n"));
        }

        /* Configure the type of BT collocated with WLAN */
        A_MEMZERO(&sbfa_cmd, sizeof(WMI_SET_BTCOEX_FE_ANT_CMD));
#ifdef CONFIG_AR600x_DUAL_ANTENNA
        sbfa_cmd.btcoexFeAntType = 2;
#elif defined(CONFIG_AR600x_SINGLE_ANTENNA)
        sbfa_cmd.btcoexFeAntType = 1;
#else
#error Unsupported Front-End Antenna Configuration
#endif /* AR600x Front-End Antenna Configuration */

        if ((wmi_set_btcoex_fe_ant_cmd(ar->arWmi, &sbfa_cmd)) != A_OK) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Unable to set fornt end antenna configuration\n"));
        }
#endif /* INIT_MODE_DRV_ENABLED && ENABLE_COEXISTENCE */
    }

    ar->arNumDataEndPts = 1;

    if (bypasswmi) {
            /* for tests like endpoint ping, the MAC address needs to be non-zero otherwise
             * the data path through a raw socket is disabled */
        dev->dev_addr[0] = 0x00;
        dev->dev_addr[1] = 0x01;
        dev->dev_addr[2] = 0x02;
        dev->dev_addr[3] = 0xAA;
        dev->dev_addr[4] = 0xBB;
        dev->dev_addr[5] = 0xCC;
    }

ar6000_init_done:
    rtnl_lock();
    dev_put(dev);

    return ret;
}


void
ar6000_bitrate_rx(void *devt, A_INT32 rateKbps)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;

    ar->arBitRate = rateKbps;
    wake_up(&arEvent);
}

void
ar6000_ratemask_rx(void *devt, A_UINT32 ratemask)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;

    ar->arRateMask = ratemask;
    wake_up(&arEvent);
}

void
ar6000_txPwr_rx(void *devt, A_UINT8 txPwr)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;

    ar->arTxPwr = txPwr;
    wake_up(&arEvent);
}


void
ar6000_channelList_rx(void *devt, A_INT8 numChan, A_UINT16 *chanList)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;

    A_MEMCPY(ar->arChannelList, chanList, numChan * sizeof (A_UINT16));
    ar->arNumChannels = numChan;

    wake_up(&arEvent);
}

A_UINT8
ar6000_ibss_map_epid(struct sk_buff *skb, struct net_device *dev, A_UINT32 * mapNo)
{
    AR_SOFTC_T      *ar = (AR_SOFTC_T *)ar6k_priv(dev);
    A_UINT8         *datap;
    ATH_MAC_HDR     *macHdr;
    A_UINT32         i, eptMap;

    (*mapNo) = 0;
    datap = A_NETBUF_DATA(skb);
    macHdr = (ATH_MAC_HDR *)(datap + sizeof(WMI_DATA_HDR));
    if (IEEE80211_IS_MULTICAST(macHdr->dstMac)) {
        return ENDPOINT_2;
    }

    eptMap = -1;
    for (i = 0; i < ar->arNodeNum; i ++) {
        if (IEEE80211_ADDR_EQ(macHdr->dstMac, ar->arNodeMap[i].macAddress)) {
            (*mapNo) = i + 1;
            ar->arNodeMap[i].txPending ++;
            return ar->arNodeMap[i].epId;
        }

        if ((eptMap == -1) && !ar->arNodeMap[i].txPending) {
            eptMap = i;
        }
    }

    if (eptMap == -1) {
        eptMap = ar->arNodeNum;
        ar->arNodeNum ++;
        A_ASSERT(ar->arNodeNum <= MAX_NODE_NUM);
    }

    A_MEMCPY(ar->arNodeMap[eptMap].macAddress, macHdr->dstMac, IEEE80211_ADDR_LEN);

    for (i = ENDPOINT_2; i <= ENDPOINT_5; i ++) {
        if (!ar->arTxPending[i]) {
            ar->arNodeMap[eptMap].epId = i;
            break;
        }
        // No free endpoint is available, start redistribution on the inuse endpoints.
        if (i == ENDPOINT_5) {
            ar->arNodeMap[eptMap].epId = ar->arNexEpId;
            ar->arNexEpId ++;
            if (ar->arNexEpId > ENDPOINT_5) {
                ar->arNexEpId = ENDPOINT_2;
            }
        }
    }

    (*mapNo) = eptMap + 1;
    ar->arNodeMap[eptMap].txPending ++;

    return ar->arNodeMap[eptMap].epId;
}

#ifdef DEBUG
static void ar6000_dump_skb(struct sk_buff *skb)
{
   u_char *ch;
   for (ch = A_NETBUF_DATA(skb);
        (unsigned long)ch < ((unsigned long)A_NETBUF_DATA(skb) +
        A_NETBUF_LEN(skb)); ch++)
    {
         AR_DEBUG_PRINTF(ATH_DEBUG_WARN,("%2.2x ", *ch));
    }
    AR_DEBUG_PRINTF(ATH_DEBUG_WARN,("\n"));
}
#endif

#ifdef HTC_TEST_SEND_PKTS
static void DoHTCSendPktsTest(AR_SOFTC_T *ar, int MapNo, HTC_ENDPOINT_ID eid, struct sk_buff *skb);
#endif

static int
ar6000_data_tx(struct sk_buff *skb, struct net_device *dev)
{
#define AC_NOT_MAPPED   99
    AR_SOFTC_T        *ar = (AR_SOFTC_T *)ar6k_priv(dev);
    A_UINT8            ac = AC_NOT_MAPPED;
    HTC_ENDPOINT_ID    eid = ENDPOINT_UNUSED;
    A_UINT32          mapNo = 0;
    int               len;
    struct ar_cookie *cookie;
    A_BOOL            checkAdHocPsMapping = FALSE,bMoreData = FALSE;
    HTC_TX_TAG        htc_tag = AR6K_DATA_PKT_TAG;
    A_UINT8           dot11Hdr = processDot11Hdr;
#ifdef CONFIG_PM
    if (ar->arWowState != WLAN_WOW_STATE_NONE) {
        A_NETBUF_FREE(skb);
        return 0;
    }
#endif /* CONFIG_PM */

    AR_DEBUG_PRINTF(ATH_DEBUG_WLAN_TX,("ar6000_data_tx start - skb=0x%lx, data=0x%lx, len=0x%x\n",
                     (unsigned long)skb, (unsigned long)A_NETBUF_DATA(skb),
                     A_NETBUF_LEN(skb)));

    /* If target is not associated */
    if( (!ar->arConnected && !bypasswmi)
#ifdef CONFIG_HOST_TCMD_SUPPORT
     /* TCMD doesnt support any data, free the buf and return */
    || (ar->arTargetMode == AR6000_TCMD_MODE)
#endif
                                            ) {
        A_NETBUF_FREE(skb);
        return 0;
    }

    do {

        if (ar->arWmiReady == FALSE && bypasswmi == 0) {
            break;
        }

#ifdef BLOCK_TX_PATH_FLAG
        if (blocktx) {
            break;
        }
#endif /* BLOCK_TX_PATH_FLAG */

        /* AP mode Power save processing */
        /* If the dst STA is in sleep state, queue the pkt in its PS queue */

        if (ar->arNetworkType == AP_NETWORK) {
            ATH_MAC_HDR *datap = (ATH_MAC_HDR *)A_NETBUF_DATA(skb);
            sta_t *conn = NULL;

            /* If the dstMac is a Multicast address & atleast one of the
             * associated STA is in PS mode, then queue the pkt to the
             * mcastq
             */
            if (IEEE80211_IS_MULTICAST(datap->dstMac)) {
                A_UINT8 ctr=0;
                A_BOOL qMcast=FALSE;


                for (ctr=0; ctr<AP_MAX_NUM_STA; ctr++) {
                    if (STA_IS_PWR_SLEEP((&ar->sta_list[ctr]))) {
                        qMcast = TRUE;
                    }
                }
                if(qMcast) {

                    /* If this transmit is not because of a Dtim Expiry q it */
                    if (ar->DTIMExpired == FALSE) {
                        A_BOOL isMcastqEmpty = FALSE;

                        A_MUTEX_LOCK(&ar->mcastpsqLock);
                        isMcastqEmpty = A_NETBUF_QUEUE_EMPTY(&ar->mcastpsq);
                        A_NETBUF_ENQUEUE(&ar->mcastpsq, skb);
                        A_MUTEX_UNLOCK(&ar->mcastpsqLock);

                        /* If this is the first Mcast pkt getting queued
                         * indicate to the target to set the BitmapControl LSB
                         * of the TIM IE.
                         */
                        if (isMcastqEmpty) {
                             wmi_set_pvb_cmd(ar->arWmi, MCAST_AID, 1);
                        }
                        return 0;
                    } else {
                     /* This transmit is because of Dtim expiry. Determine if
                      * MoreData bit has to be set.
                      */
                         A_MUTEX_LOCK(&ar->mcastpsqLock);
                         if(!A_NETBUF_QUEUE_EMPTY(&ar->mcastpsq)) {
                             bMoreData = TRUE;
                         }
                         A_MUTEX_UNLOCK(&ar->mcastpsqLock);
                    }
                }
            } else {
                conn = ieee80211_find_conn(ar, datap->dstMac);
                if (conn) {
                    if (STA_IS_PWR_SLEEP(conn)) {
                        /* If this transmit is not because of a PsPoll q it*/
                        if (!STA_IS_PS_POLLED(conn)) {
                            A_BOOL isPsqEmpty = FALSE;
                            /* Queue the frames if the STA is sleeping */
                            A_MUTEX_LOCK(&conn->psqLock);
                            isPsqEmpty = A_NETBUF_QUEUE_EMPTY(&conn->psq);
                            A_NETBUF_ENQUEUE(&conn->psq, skb);
                            A_MUTEX_UNLOCK(&conn->psqLock);

                            /* If this is the first pkt getting queued
                             * for this STA, update the PVB for this STA
                             */
                            if (isPsqEmpty) {
                                wmi_set_pvb_cmd(ar->arWmi, conn->aid, 1);
                            }

                            return 0;
                         } else {
                         /* This tx is because of a PsPoll. Determine if
                          * MoreData bit has to be set
                          */
                             A_MUTEX_LOCK(&conn->psqLock);
                             if (!A_NETBUF_QUEUE_EMPTY(&conn->psq)) {
                                 bMoreData = TRUE;
                             }
                             A_MUTEX_UNLOCK(&conn->psqLock);
                         }
                    }
                } else {

                    /* non existent STA. drop the frame */
                    A_NETBUF_FREE(skb);
                    return 0;
                }
            }
        }

        if (ar->arWmiEnabled) {
#ifdef CONFIG_CHECKSUM_OFFLOAD
        A_UINT8 csumStart=0;
        A_UINT8 csumDest=0;
        A_UINT8 csum=skb->ip_summed;
        if(csumOffload && (csum==CHECKSUM_PARTIAL)){
            csumStart = (skb->head + skb->csum_start - skb_network_header(skb) +
			 sizeof(ATH_LLC_SNAP_HDR));
            csumDest=skb->csum_offset+csumStart;
        }
#endif
            if (A_NETBUF_HEADROOM(skb) < dev->hard_header_len - LINUX_HACK_FUDGE_FACTOR) {
                struct sk_buff  *newbuf;

                /*
                 * We really should have gotten enough headroom but sometimes
                 * we still get packets with not enough headroom.  Copy the packet.
                 */
                len = A_NETBUF_LEN(skb);
                newbuf = A_NETBUF_ALLOC(len);
                if (newbuf == NULL) {
                    break;
                }
                A_NETBUF_PUT(newbuf, len);
                A_MEMCPY(A_NETBUF_DATA(newbuf), A_NETBUF_DATA(skb), len);
                A_NETBUF_FREE(skb);
                skb = newbuf;
                /* fall through and assemble header */
            }

            if (dot11Hdr) {
                if (wmi_dot11_hdr_add(ar->arWmi,skb,ar->arNetworkType) != A_OK) {
                    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ar6000_data_tx-wmi_dot11_hdr_add failed\n"));
                    break;
                }
            } else {
                if (wmi_dix_2_dot3(ar->arWmi, skb) != A_OK) {
                    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ar6000_data_tx - wmi_dix_2_dot3 failed\n"));
                    break;
                }
            }
#ifdef CONFIG_CHECKSUM_OFFLOAD
            if(csumOffload && (csum ==CHECKSUM_PARTIAL)){
                WMI_TX_META_V2  metaV2;
                metaV2.csumStart =csumStart;
                metaV2.csumDest = csumDest;
                metaV2.csumFlags = 0x1;/*instruct target to calculate checksum*/
                if (wmi_data_hdr_add(ar->arWmi, skb, DATA_MSGTYPE, bMoreData, dot11Hdr,
                                        WMI_META_VERSION_2,&metaV2) != A_OK) {
                    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ar6000_data_tx - wmi_data_hdr_add failed\n"));
                    break;
                }

            }
            else
#endif
            {
                if (wmi_data_hdr_add(ar->arWmi, skb, DATA_MSGTYPE, bMoreData, dot11Hdr,0,NULL) != A_OK) {
                    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ar6000_data_tx - wmi_data_hdr_add failed\n"));
                    break;
                }
            }


            if ((ar->arNetworkType == ADHOC_NETWORK) &&
                ar->arIbssPsEnable && ar->arConnected) {
                    /* flag to check adhoc mapping once we take the lock below: */
                checkAdHocPsMapping = TRUE;

            } else {
                    /* get the stream mapping */
                ac  =  wmi_implicit_create_pstream(ar->arWmi, skb, 0, ar->arWmmEnabled);
            }

        } else {
            EPPING_HEADER    *eppingHdr;

            eppingHdr = A_NETBUF_DATA(skb);

            if (IS_EPPING_PACKET(eppingHdr)) {
                    /* the stream ID is mapped to an access class */
                ac = eppingHdr->StreamNo_h;
                    /* some EPPING packets cannot be dropped no matter what access class it was
                     * sent on.  We can change the packet tag to guarantee it will not get dropped */
                if (IS_EPING_PACKET_NO_DROP(eppingHdr)) {
                    htc_tag = AR6K_CONTROL_PKT_TAG;
                }

                if (ac == HCI_TRANSPORT_STREAM_NUM) {
                        /* pass this to HCI */
#ifndef EXPORT_HCI_BRIDGE_INTERFACE
                    if (A_SUCCESS(hci_test_send(ar,skb))) {
                        return 0;
                    }
#endif
                        /* set AC to discard this skb */
                    ac = AC_NOT_MAPPED;
                } else {
                    /* a quirk of linux, the payload of the frame is 32-bit aligned and thus the addition
                     * of the HTC header will mis-align the start of the HTC frame, so we add some
                     * padding which will be stripped off in the target */
                    if (EPPING_ALIGNMENT_PAD > 0) {
                        A_NETBUF_PUSH(skb, EPPING_ALIGNMENT_PAD);
                    }
                }

            } else {
                    /* not a ping packet, drop it */
                ac = AC_NOT_MAPPED;
            }
        }

    } while (FALSE);

        /* did we succeed ? */
    if ((ac == AC_NOT_MAPPED) && !checkAdHocPsMapping) {
            /* cleanup and exit */
        A_NETBUF_FREE(skb);
        AR6000_STAT_INC(ar, tx_dropped);
        AR6000_STAT_INC(ar, tx_aborted_errors);
        return 0;
    }

    cookie = NULL;

        /* take the lock to protect driver data */
    AR6000_SPIN_LOCK(&ar->arLock, 0);

    do {

        if (checkAdHocPsMapping) {
            eid = ar6000_ibss_map_epid(skb, dev, &mapNo);
        }else {
            eid = arAc2EndpointID (ar, ac);
        }
            /* validate that the endpoint is connected */
        if (eid == 0 || eid == ENDPOINT_UNUSED ) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,(" eid %d is NOT mapped!\n", eid));
            break;
        }
            /* allocate resource for this packet */
        cookie = ar6000_alloc_cookie(ar);

        if (cookie != NULL) {
                /* update counts while the lock is held */
            ar->arTxPending[eid]++;
            ar->arTotalTxDataPending++;
        }

    } while (FALSE);

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    if (cookie != NULL) {
        cookie->arc_bp[0] = (unsigned long)skb;
        cookie->arc_bp[1] = mapNo;
        SET_HTC_PACKET_INFO_TX(&cookie->HtcPkt,
                               cookie,
                               A_NETBUF_DATA(skb),
                               A_NETBUF_LEN(skb),
                               eid,
                               htc_tag);

#ifdef DEBUG
        if (debugdriver >= 3) {
            ar6000_dump_skb(skb);
        }
#endif
#ifdef HTC_TEST_SEND_PKTS
        DoHTCSendPktsTest(ar,mapNo,eid,skb);
#endif
            /* HTC interface is asynchronous, if this fails, cleanup will happen in
             * the ar6000_tx_complete callback */
        HTCSendPkt(ar->arHtcTarget, &cookie->HtcPkt);
    } else {
            /* no packet to send, cleanup */
        A_NETBUF_FREE(skb);
        AR6000_STAT_INC(ar, tx_dropped);
        AR6000_STAT_INC(ar, tx_aborted_errors);
    }

    return 0;
}

int
ar6000_acl_data_tx(struct sk_buff *skb, struct net_device *dev)
{
    AR_SOFTC_T        *ar = (AR_SOFTC_T *)ar6k_priv(dev);
    struct ar_cookie *cookie;
    HTC_ENDPOINT_ID    eid = ENDPOINT_UNUSED;

    cookie = NULL;
    AR6000_SPIN_LOCK(&ar->arLock, 0);

        /* For now we send ACL on BE endpoint: We can also have a dedicated EP */
        eid = arAc2EndpointID (ar, 0);
        /* allocate resource for this packet */
        cookie = ar6000_alloc_cookie(ar);

        if (cookie != NULL) {
            /* update counts while the lock is held */
            ar->arTxPending[eid]++;
            ar->arTotalTxDataPending++;
        }


    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

        if (cookie != NULL) {
            cookie->arc_bp[0] = (unsigned long)skb;
            cookie->arc_bp[1] = 0;
            SET_HTC_PACKET_INFO_TX(&cookie->HtcPkt,
                            cookie,
                            A_NETBUF_DATA(skb),
                            A_NETBUF_LEN(skb),
                            eid,
                            AR6K_DATA_PKT_TAG);

            /* HTC interface is asynchronous, if this fails, cleanup will happen in
             * the ar6000_tx_complete callback */
            HTCSendPkt(ar->arHtcTarget, &cookie->HtcPkt);
        } else {
            /* no packet to send, cleanup */
            A_NETBUF_FREE(skb);
            AR6000_STAT_INC(ar, tx_dropped);
            AR6000_STAT_INC(ar, tx_aborted_errors);
        }
    return 0;
}


#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
static void
tvsub(register struct timeval *out, register struct timeval *in)
{
    if((out->tv_usec -= in->tv_usec) < 0) {
        out->tv_sec--;
        out->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}

void
applyAPTCHeuristics(AR_SOFTC_T *ar)
{
    A_UINT32 duration;
    A_UINT32 numbytes;
    A_UINT32 throughput;
    struct timeval ts;
    A_STATUS status;

    AR6000_SPIN_LOCK(&ar->arLock, 0);

    if ((enableAPTCHeuristics) && (!aptcTR.timerScheduled)) {
        do_gettimeofday(&ts);
        tvsub(&ts, &aptcTR.samplingTS);
        duration = ts.tv_sec * 1000 + ts.tv_usec / 1000; /* ms */
        numbytes = aptcTR.bytesTransmitted + aptcTR.bytesReceived;

        if (duration > APTC_TRAFFIC_SAMPLING_INTERVAL) {
            /* Initialize the time stamp and byte count */
            aptcTR.bytesTransmitted = aptcTR.bytesReceived = 0;
            do_gettimeofday(&aptcTR.samplingTS);

            /* Calculate and decide based on throughput thresholds */
            throughput = ((numbytes * 8) / duration);
            if (throughput > APTC_UPPER_THROUGHPUT_THRESHOLD) {
                /* Disable Sleep and schedule a timer */
                A_ASSERT(ar->arWmiReady == TRUE);
                AR6000_SPIN_UNLOCK(&ar->arLock, 0);
                status = wmi_powermode_cmd(ar->arWmi, MAX_PERF_POWER);
                AR6000_SPIN_LOCK(&ar->arLock, 0);
                A_TIMEOUT_MS(&aptcTimer, APTC_TRAFFIC_SAMPLING_INTERVAL, 0);
                aptcTR.timerScheduled = TRUE;
            }
        }
    }

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);
}
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

static HTC_SEND_FULL_ACTION ar6000_tx_queue_full(void *Context, HTC_PACKET *pPacket)
{
    AR_SOFTC_T     *ar = (AR_SOFTC_T *)Context;
    HTC_SEND_FULL_ACTION    action = HTC_SEND_FULL_KEEP;
    A_BOOL                  stopNet = FALSE;
    HTC_ENDPOINT_ID         Endpoint = HTC_GET_ENDPOINT_FROM_PKT(pPacket);

    do {

        if (bypasswmi) {
            int accessClass;

            if (HTC_GET_TAG_FROM_PKT(pPacket) == AR6K_CONTROL_PKT_TAG) {
                    /* don't drop special control packets */
                break;
            }

            accessClass = arEndpoint2Ac(ar,Endpoint);
                /* for endpoint ping testing drop Best Effort and Background */
            if ((accessClass == WMM_AC_BE) || (accessClass == WMM_AC_BK)) {
                action = HTC_SEND_FULL_DROP;
                stopNet = FALSE;
            } else {
                    /* keep but stop the netqueues */
                stopNet = TRUE;
            }
            break;
        }

        if (Endpoint == ar->arControlEp) {
                /* under normal WMI if this is getting full, then something is running rampant
                 * the host should not be exhausting the WMI queue with too many commands
                 * the only exception to this is during testing using endpointping */
            AR6000_SPIN_LOCK(&ar->arLock, 0);
                /* set flag to handle subsequent messages */
            ar->arWMIControlEpFull = TRUE;
            AR6000_SPIN_UNLOCK(&ar->arLock, 0);
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("WMI Control Endpoint is FULL!!! \n"));
                /* no need to stop the network */
            stopNet = FALSE;
            break;
        }

        /* if we get here, we are dealing with data endpoints getting full */

        if (HTC_GET_TAG_FROM_PKT(pPacket) == AR6K_CONTROL_PKT_TAG) {
            /* don't drop control packets issued on ANY data endpoint */
            break;
        }

        if (ar->arNetworkType == ADHOC_NETWORK) {
            /* in adhoc mode, we cannot differentiate traffic priorities so there is no need to
             * continue, however we should stop the network */
            stopNet = TRUE;
            break;
        }
        /* the last MAX_HI_COOKIE_NUM "batch" of cookies are reserved for the highest
         * active stream */
        if (ar->arAcStreamPriMap[arEndpoint2Ac(ar,Endpoint)] < ar->arHiAcStreamActivePri &&
            ar->arCookieCount <= MAX_HI_COOKIE_NUM) {
                /* this stream's priority is less than the highest active priority, we
                 * give preference to the highest priority stream by directing
                 * HTC to drop the packet that overflowed */
            action = HTC_SEND_FULL_DROP;
                /* since we are dropping packets, no need to stop the network */
  