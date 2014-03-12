#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2011, Code Aurora Forum. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Code Aurora nor
#       the names of its contributors may be used to endorse or promote
#       products derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Invoke gcc, looking for warnings, and causing a failure if there are
# non-whitelisted warnings.

import errno
import re
import os
import sys
import subprocess

# Note that gcc uses unicode, which may depend on the locale.  TODO:
# force LANG to be set to en_US.UTF-8 to get consistent warnings.

allowed_warnings = set([
    "alignment.c:720",
    "async.c:122",
    "async.c:270",
    "dir.c:43",
    "dm.c:1053",
    "dm.c:1080",
    "dm-table.c:1120",
    "dm-table.c:1126",
    "drm_edid.c:1303",
    "eventpoll.c:1143",
    "f_mass_storage.c:3368",
    "inode.c:72",
    "inode.c:73",
    "inode.c:74",
    "msm_sdcc.c:126",
    "msm_sdcc.c:128",
    "nf_conntrack_netlink.c:790",
    "nf_nat_standalone.c:118",
    "return_address.c:61",
    "soc-core.c:1719",
    "xt_log.h:50",
    "dma-mapping.c:229" ,
    "dma-mapping.c:275" ,
    "alignment.c:298" ,
    "smd.c:2473" ,
    "smd_rpcrouter_device.c:419" ,
    "adsp.c:394" ,
    "audpp.c:372" ,
    "audio_voicememo.c:383" ,
    "rpc_server_misc.c:558" ,
    "LG_rapi_client.c:929" ,
    "LG_rapi_client.c:921" ,
    "LG_rapi_client.c:1194" ,
    "LG_rapi_client.c:1242" ,
    "LG_rapi_client.c:1289" ,
    "swab.h:36" ,
    "lg_diag_testmode.c:272" ,
    "lg_diag_testmode.c:1025" ,
    "lg_diag_testmode.c:1143" ,
    "lg_diag_testmode.c:2714" ,
    "lg_diag_dload.c:325" ,
    "sched.c:8268" ,
    "sched.c:8359" ,
    "params.c:830" ,
    "wakelock.c:137" ,
    "fbearlysuspend.c:45" ,
    "module.c:1372" ,
    "cgroup.c:718" ,
    "page_alloc.c:5440" ,
    "backing-dev.c:72" ,
    "memory.c:733" ,
    "mmap.c:1687",
    "extents.c:3111" ,
    "extents.c:3298" ,
    "mballoc.c:2922" ,
    "dnotify.c:91" ,
    "inotify_fsnotify.c:98" ,
    "inotify_user.c:520" ,
    "check.c:624" ,
    "eventpoll.c:1185" ,
    "binfmt_elf.c:573" ,
    "blk-core.c:1792" ,
    "module.c:36" , 
    "lg_diag_kernel_service.c:1353" ,
    "kgsl_sharedmem.c:175" ,
    "bmm050_driver.c:1230" ,
    "bmm050_driver.c:1251" ,
    "leds-msm-pmic-e0.c:39" ,
    "leds-msm-pmic-e0.c:42" ,
    "leds-msm-pmic-e0.c:45" ,
    "leds-msm-pmic-e0.c:48" ,
    "leds-msm-pmic-e0.c:51" ,
    "leds-msm-pmic-e0.c:54" ,
    "leds-msm-pmic-e0.c:57" ,
    "leds-msm-pmic-e0.c:60" ,
    "dm.c:1389" ,
    "msm_camera.c:2147" ,
    "pmem.c:1405" ,
    "lge_tty_atcmd.c:271" ,
    "msm_sdcc.c:1301" ,
    "msm_sdcc.c:1719" ,
    "msm_rmnet.c:773" ,
    "core.c:2650" ,
    "sd.c:2356" ,
    "sg.c:720" ,
    "serial_core.c:1982" ,
    "msm_serial.c:158" ,
    "msm_serial.c:924" ,
    "vt.c:862" ,
    "sysfs.c:849" ,
    "ehci-hub.c:564" ,
    "ehci-q.c:1098" ,
    "ehci-sched.c:1051" ,
    "ehci-sched.c:1749" ,
    "ehci-sched.c:2161" ,
    "alauda.c:468" ,
    "freecom.c:427" ,
    "sddr09.c:877" ,
    "sddr55.c:795" ,
    "initializers.c:49" ,
    "initializers.c:98" ,
    "sierra_ms.c:133" ,
    "sierra_ms.c:129" ,
    "f_rmnet.c:576" ,
    "f_mass_storage.c:1160" ,
    "u_serial.c:467" ,
    "u_serial.c:981" ,
    "u_bam.c:827" ,
    "u_rmnet_ctrl_smd.c:349" ,
    "bu61800_bl.c:889" ,
    "mdp.c:273" ,
    "mdp_debugfs.c:146" ,
    "mdp_debugfs.c:540" ,
    "mdp_debugfs.c:673" ,
    "mdp_debugfs.c:887" ,
    "mdp_dma_lcdc.c:58" ,
    "mdp_dma_lcdc.c:289" ,
    "mdp_dma_dsi_video.c:40" ,
    "mdp_dma.c:78" ,
    "mdp_dma.c:63" ,
    "mdp_dma.c:62" ,
    "mdp_dma.c:57" ,
    "mdp_dma.c:581" ,
    "pcm_native.c:1522" ,
    "pcm_native.c:2060" ,
    "init.c:345" ,
    "control.c:599" ,
    "msm7k-pcm.c:399" ,
    "sco.c:744" ,
    "route.c:1528" ,
    "nf_nat_standalone.c:286" ,
    "nf_nat_core.c:520" ,
    "nf_nat_core.c:734" ,
    "nf_nat_core.c:735" ,
    "nf_nat_core.c:736" ,
    "nf_nat_core.c:737" ,
    "nf_nat_core.c:746" ,
    "nf_nat_core.c:748" ,
    "nf_nat_core.c:751" ,
    "nf_nat_amanda.c:80" ,
    "nf_nat_ftp.c:123" ,
    "nf_nat_h323.c:584" ,
    "nf_nat_h323.c:585" ,
    "nf_nat_h323.c:586" ,
    "nf_nat_h323.c:587" ,
    "nf_nat_h323.c:588" ,
    "nf_nat_h323.c:589" ,
    "nf_nat_h323.c:590" ,
    "nf_nat_h323.c:591" ,
    "nf_nat_h323.c:592" ,
    "nf_nat_irc.c:85" ,
    "nf_nat_pptp.c:285" ,
    "nf_nat_pptp.c:288" ,
    "nf_nat_pptp.c:291" ,
    "nf_nat_pptp.c:294" ,
    "nf_nat_sip.c:550" ,
    "nf_nat_sip.c:551" ,
    "nf_nat_sip.c:552" ,
    "nf_nat_sip.c:553" ,
    "nf_nat_sip.c:554" ,
    "nf_nat_sip.c:555" ,
    "nf_nat_sip.c:556",
    "nf_nat_tftp.c:46" ,
    "nf_nat_proto_sctp.c:39" ,
    "ip6_output.c:277" ,
    "addrconf.c:721" ,
    "route.c:1033" ,
    "mcast.c:322" ,
    "mcast.c:458" ,
    "mcast.c:55" ,
    "mcast.c:552" ,
    "xfrm6_tunnel.c:244" ,
    "xfrm6_mode_beet.c:44" ,
    "ip6table_mangle.c:39" ,
    "nf_conntrack_core.c:1556" ,
 ])

# Capture the name of the object file, can find it.
ofile = None

warning_re = re.compile(r'''(.*/|)([^/]+\.[a-z]+:\d+):(\d+:)? warning:''')
def interpret_warning(line):
    """Decode the message from gcc.  The messages we care about have a filename, and a warning"""
    line = line.rstrip('\n')
    m = warning_re.match(line)
    if m and m.group(2) not in allowed_warnings:
        print "error, forbidden warning:", m.group(2)

        # If there is a warning, remove any object if it exists.
        if ofile:
            try:
                os.remove(ofile)
            except OSError:
                pass
        sys.exit(1)

def run_gcc():
    args = sys.argv[1:]
    # Look for -o
    try:
        i = args.index('-o')
        global ofile
        ofile = args[i+1]
    except (ValueError, IndexError):
        pass

    compiler = sys.argv[0]

    try:
        proc = subprocess.Popen(args, stderr=subprocess.PIPE)
        for line in proc.stderr:
            print line,
            interpret_warning(line)

        result = proc.wait()
    except OSError as e:
        result = e.errno
        if result == errno.ENOENT:
            print args[0] + ':',e.strerror
            print 'Is your PATH set correctly?'
        else:
            print ' '.join(args), str(e)

    return result

if __name__ == '__main__':
    status = run_gcc()
    sys.exit(status)
