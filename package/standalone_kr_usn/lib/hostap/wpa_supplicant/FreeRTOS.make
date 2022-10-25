ifdef CONFIG_TESTING_OPTIONS
CFLAGS += -DCONFIG_TESTING_OPTIONS
CONFIG_WPS_TESTING=y
CONFIG_TDLS_TESTING=y
endif

ifeq ($(CONFIG_WPA_SUPP_CRYPTO), mbedtls)
CONFIG_TLS=mbedtls
else
CONFIG_TLS=internal
endif #CONFIG_MBEDTLS

ifeq ($(CONFIG_SOFT_AP), y)
CONFIG_AP=y
NEED_AP_MLME=y
endif

BINALL=wpa_supplicant wpa_cli

ifndef CONFIG_NO_WPA_PASSPHRASE
BINALL += wpa_passphrase
endif

ALL = $(BINALL)
ALL += systemd/wpa_supplicant.service
ALL += systemd/wpa_supplicant@.service
ALL += systemd/wpa_supplicant-nl80211@.service
ALL += systemd/wpa_supplicant-wired@.service
ALL += dbus/fi.w1.wpa_supplicant1.service
ifdef CONFIG_BUILD_WPA_CLIENT_SO
ALL += libwpa_client.so
endif

ifdef CONFIG_FIPS
CONFIG_NO_RANDOM_POOL=
CONFIG_OPENSSL_CMAC=y
endif

WPA_SUPP_CSRCS += config.c
WPA_SUPP_CSRCS += notify.c
WPA_SUPP_CSRCS += bss.c
WPA_SUPP_CSRCS += eap_register.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/common.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/wpa_debug.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/wpabuf.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/bitfield.c
WPA_SUPP_CSRCS += op_classes.c
WPA_SUPP_CSRCS_p = rrm.c
WPA_SUPP_CSRCS_p += wpa_passphrase.c
WPA_SUPP_CSRCS_p += $(WPA_SUPP_ROOT)/src/utils/common.c
WPA_SUPP_CSRCS_p += $(WPA_SUPP_ROOT)/src/utils/wpa_debug.c
WPA_SUPP_CSRCS_p += $(WPA_SUPP_ROOT)/src/utils/wpabuf.c
WPA_SUPP_CSRCS_c = wpa_cli.c $(WPA_SUPP_ROOT)/src/common/wpa_ctrl.c
WPA_SUPP_CSRCS_c += $(WPA_SUPP_ROOT)/src/utils/wpa_debug.c
WPA_SUPP_CSRCS_c += $(WPA_SUPP_ROOT)/src/utils/common.c
WPA_SUPP_CSRCS_c += $(WPA_SUPP_ROOT)/src/common/cli.c
WPA_SUPP_CSRCS += wmm_ac.c

ifndef CONFIG_OS
ifdef CONFIG_NATIVE_WINDOWS
CONFIG_OS=win32
else
CONFIG_OS=unix
endif
endif

ifeq ($(CONFIG_OS), internal)
CFLAGS += -DOS_NO_C_LIB_DEFINES
endif

WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/os_$(CONFIG_OS).c
WPA_SUPP_CSRCS_p += $(WPA_SUPP_ROOT)/src/utils/os_$(CONFIG_OS).c
WPA_SUPP_CSRCS_c += $(WPA_SUPP_ROOT)/src/utils/os_$(CONFIG_OS).c

ifdef CONFIG_WPA_TRACE
CFLAGS += -DWPA_TRACE
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/trace.c
WPA_SUPP_CSRCS_p += $(WPA_SUPP_ROOT)/src/utils/trace.c
WPA_SUPP_CSRCS_c += $(WPA_SUPP_ROOT)/src/utils/trace.c
WPA_SUPP_CSRCS_priv += $(WPA_SUPP_ROOT)/src/utils/trace.c
LIBCTRL += $(WPA_SUPP_ROOT)/src/utils/trace.c
LIBCTRLSO += $(WPA_SUPP_ROOT)/src/utils/trace.c
LDFLAGS += -rdynamic
CFLAGS += -funwind-tables
ifdef CONFIG_WPA_TRACE_BFD
CFLAGS += -DPACKAGE="wpa_supplicant" -DWPA_TRACE_BFD
LIBS += -lbfd -ldl -liberty -lz
LIBS_p += -lbfd -ldl -liberty -lz
LIBS_c += -lbfd -ldl -liberty -lz
endif
endif

ifndef CONFIG_ELOOP
CONFIG_ELOOP=eloop
endif
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/$(CONFIG_ELOOP).c
WPA_SUPP_CSRCS_c += $(WPA_SUPP_ROOT)/src/utils/$(CONFIG_ELOOP).c

ifndef CONFIG_OSX
ifeq ($(CONFIG_ELOOP), eloop)
# Using glibc < 2.17 requires -lrt for clock_gettime()
# OS X has an alternate implementation
LIBS += -lrt
LIBS_c += -lrt
LIBS_p += -lrt
endif
endif

ifdef CONFIG_ELOOP_POLL
CFLAGS += -DCONFIG_ELOOP_POLL
endif

ifdef CONFIG_ELOOP_EPOLL
CFLAGS += -DCONFIG_ELOOP_EPOLL
endif

ifdef CONFIG_ELOOP_KQUEUE
CFLAGS += -DCONFIG_ELOOP_KQUEUE
endif

ifdef CONFIG_EAPOL_TEST
CFLAGS += -Werror -DEAPOL_TEST
endif

ifdef CONFIG_CODE_COVERAGE
CFLAGS += -O0 -fprofile-arcs -ftest-coverage
LIBS += -lgcov
LIBS_c += -lgcov
LIBS_p += -lgcov
endif

ifdef CONFIG_HT_OVERRIDES
CFLAGS += -DCONFIG_HT_OVERRIDES
endif

ifdef CONFIG_VHT_OVERRIDES
CFLAGS += -DCONFIG_VHT_OVERRIDES
endif

ifndef CONFIG_BACKEND
CONFIG_BACKEND=file
endif

ifeq ($(CONFIG_BACKEND), file)
WPA_SUPP_CSRCS += config_file.c
ifndef CONFIG_NO_CONFIG_BLOBS
NEED_BASE64=y
endif
CFLAGS += -DCONFIG_BACKEND_FILE
endif

ifeq ($(CONFIG_BACKEND), winreg)
WPA_SUPP_CSRCS += config_winreg.c
endif

ifeq ($(CONFIG_BACKEND), none)
WPA_SUPP_CSRCS += config_none.c
endif

ifeq ($(CONFIG_BACKEND), nrc)
#Include the file only when config_nrc.c is exist
WPA_SUPP_CSRCS += $(shell find $(WPA_SUPP_ROOT)/wpa_supplicant -name config_nrc.c)
WPA_SUPP_CSRCS += config_nrc_util.c
endif

ifdef CONFIG_NO_CONFIG_WRITE
CFLAGS += -DCONFIG_NO_CONFIG_WRITE
endif

ifdef CONFIG_NO_CONFIG_BLOBS
CFLAGS += -DCONFIG_NO_CONFIG_BLOBS
endif

ifdef CONFIG_NO_SCAN_PROCESSING
CFLAGS += -DCONFIG_NO_SCAN_PROCESSING
endif

ifdef CONFIG_SUITEB
CFLAGS += -DCONFIG_SUITEB
NEED_SHA256=y
NEED_AES_OMAC1=y
endif

ifdef CONFIG_SUITEB192
CFLAGS += -DCONFIG_SUITEB192
NEED_SHA384=y
endif

ifdef CONFIG_OCV
CFLAGS += -DCONFIG_OCV
OBJS += ../src/common/ocv.o
CONFIG_IEEE80211W=y
endif

ifdef CONFIG_IEEE80211W
CFLAGS += -DCONFIG_IEEE80211W
NEED_SHA256=y
NEED_AES_OMAC1=y
endif

ifdef CONFIG_IEEE80211R
CFLAGS += -DCONFIG_IEEE80211R
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/rsn_supp/wpa_ft.c
NEED_SHA256=y
NEED_AES_OMAC1=y
endif

ifdef CONFIG_MESH
NEED_80211_COMMON=y
NEED_SHA256=y
NEED_AES_SIV=y
CONFIG_SAE=y
CONFIG_AP=y
CFLAGS += -DCONFIG_MESH
WPA_SUPP_CSRCS += mesh.c
WPA_SUPP_CSRCS += mesh_mpm.c
WPA_SUPP_CSRCS += mesh_rsn.c
endif

ifdef CONFIG_SAE
CFLAGS += -DCONFIG_SAE
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/common/sae.c
NEED_SHA256=y
NEED_ECC=y
NEED_DH_GROUPS=y
NEED_DRAGONFLY=y
endif

ifdef CONFIG_DPP
CFLAGS += -DCONFIG_DPP
OBJS += ../src/common/dpp.o
OBJS += dpp_supplicant.o
NEED_AES_SIV=y
NEED_HMAC_SHA256_KDF=y
NEED_HMAC_SHA384_KDF=y
NEED_HMAC_SHA512_KDF=y
NEED_SHA256=y
NEED_SHA384=y
NEED_SHA512=y
NEED_JSON=y
NEED_GAS_SERVER=y
NEED_BASE64=y
ifdef CONFIG_DPP2
CFLAGS += -DCONFIG_DPP2
endif
endif

ifdef CONFIG_OWE
CFLAGS += -DCONFIG_OWE
NEED_ECC=y
NEED_HMAC_SHA256_KDF=y
NEED_HMAC_SHA384_KDF=y
NEED_HMAC_SHA512_KDF=y
NEED_SHA256=y
NEED_SHA384=y
NEED_SHA512=y
endif

ifdef CONFIG_FILS
CFLAGS += -DCONFIG_FILS
NEED_SHA384=y
NEED_AES_SIV=y
ifdef CONFIG_FILS_SK_PFS
CFLAGS += -DCONFIG_FILS_SK_PFS
NEED_ECC=y
endif
endif

ifdef CONFIG_MBO
CONFIG_WNM=y
endif

ifdef CONFIG_WNM
CFLAGS += -DCONFIG_WNM
WPA_SUPP_CSRCS += wnm_sta.c
endif

ifdef CONFIG_TDLS
CFLAGS += -DCONFIG_TDLS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/rsn_supp/tdls.c
NEED_SHA256=y
NEED_AES_OMAC1=y
endif

ifdef CONFIG_TDLS_TESTING
CFLAGS += -DCONFIG_TDLS_TESTING
endif

ifdef CONFIG_PMKSA_CACHE_EXTERNAL
CFLAGS += -DCONFIG_PMKSA_CACHE_EXTERNAL
endif

ifndef CONFIG_NO_WPA
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/rsn_supp/wpa.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/rsn_supp/preauth.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/rsn_supp/pmksa_cache.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/rsn_supp/wpa_ie.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/common/wpa_common.c
NEED_AES=y
NEED_SHA1=y
NEED_MD5=y
NEED_RC4=y
else
CFLAGS += -DCONFIG_NO_WPA
ifeq ($(CONFIG_TLS), internal)
NEED_SHA1=y
NEED_MD5=y
endif
endif

ifeq ($(CONFIG_TLS), mbedtls)
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/crypto_mbedtls.c
endif

ifdef CONFIG_IBSS_RSN
NEED_RSN_AUTHENTICATOR=y
CFLAGS += -DCONFIG_IBSS_RSN
CFLAGS += -DCONFIG_NO_VLAN
WPA_SUPP_CSRCS += ibss_rsn.c
endif

ifdef CONFIG_MATCH_IFACE
CFLAGS += -DCONFIG_MATCH_IFACE
endif

#CONFIG_P2P=y

ifdef CONFIG_P2P
WPA_SUPP_CSRCS += p2p_supplicant.c
WPA_SUPP_CSRCS += p2p_supplicant_sd.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/p2p/p2p.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/p2p/p2p_utils.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/p2p/p2p_parse.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/p2p/p2p_build.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/p2p/p2p_go_neg.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/p2p/p2p_sd.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/p2p/p2p_pd.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/p2p/p2p_invitation.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/p2p/p2p_dev_disc.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/p2p/p2p_group.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/p2p_hostapd.c
CFLAGS += -DCONFIG_P2P
NEED_GAS=y
NEED_OFFCHANNEL=y

CONFIG_AP=y
ifdef CONFIG_P2P_STRICT
CFLAGS += -DCONFIG_P2P_STRICT
endif
endif

ifdef CONFIG_WIFI_DISPLAY
CFLAGS += -DCONFIG_WIFI_DISPLAY
WPA_SUPP_CSRCS += wifi_display.c
endif

ifdef CONFIG_HS20
WPA_SUPP_CSRCS += hs20_supplicant.c
CFLAGS += -DCONFIG_HS20
CONFIG_INTERWORKING=y
NEED_AES_OMAC1=y
endif

ifdef CONFIG_INTERWORKING
WPA_SUPP_CSRCS += interworking.c
CFLAGS += -DCONFIG_INTERWORKING
NEED_GAS=y
endif

ifdef CONFIG_NO_ROAMING
CFLAGS += -DCONFIG_NO_ROAMING
endif

include $(WPA_SUPP_ROOT)/src/drivers/drivers.mak
ifdef CONFIG_AP
DRV_OBJS += $(DRV_BOTH_WPA_SUPP_CSRCS)
CFLAGS += $(DRV_BOTH_CFLAGS)
LDFLAGS += $(DRV_BOTH_LDFLAGS)
LIBS += $(DRV_BOTH_LIBS)
else
NEED_AP_MLME=
DRV_OBJS += $(DRV_WPA_WPA_SUPP_CSRCS)
CFLAGS += $(DRV_WPA_CFLAGS)
LDFLAGS += $(DRV_WPA_LDFLAGS)
LIBS += $(DRV_WPA_LIBS)
endif

ifndef CONFIG_L2_PACKET
CONFIG_L2_PACKET=linux
endif

#WPA_SUPP_CSRCS_l2 += $(WPA_SUPP_ROOT)/src/l2_packet/l2_packet_$(CONFIG_L2_PACKET).c

ifeq ($(CONFIG_L2_PACKET), pcap)
ifdef CONFIG_WINPCAP
CFLAGS += -DCONFIG_WINPCAP
LIBS += -lwpcap -lpacket
LIBS_w += -lwpcap
else
LIBS += -ldnet -lpcap
endif
endif

ifeq ($(CONFIG_L2_PACKET), winpcap)
LIBS += -lwpcap -lpacket
LIBS_w += -lwpcap
endif

ifeq ($(CONFIG_L2_PACKET), freebsd)
LIBS += -lpcap
endif

ifeq ($(CONFIG_L2_PACKET), freeRTOS)
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/drivers/l2_packet_freeRTOS.c
endif

ifdef CONFIG_ERP
CFLAGS += -DCONFIG_ERP
NEED_SHA256=y
NEED_HMAC_SHA256_KDF=y
endif

ifdef CONFIG_EAP_TLS
# EAP-TLS
ifeq ($(CONFIG_EAP_TLS), dyn)
CFLAGS += -DEAP_TLS_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_tls.so
else
CFLAGS += -DEAP_TLS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_tls.c
endif
TLS_FUNCS=y
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_UNAUTH_TLS
# EAP-UNAUTH-TLS
CFLAGS += -DEAP_UNAUTH_TLS
ifndef CONFIG_EAP_TLS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_tls.c
TLS_FUNCS=y
endif
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_PEAP
# EAP-PEAP
ifeq ($(CONFIG_EAP_PEAP), dyn)
CFLAGS += -DEAP_PEAP_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_peap.so
else
CFLAGS += -DEAP_PEAP
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_peap.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_common/eap_peap_common.c
endif
TLS_FUNCS=y
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_TTLS
# EAP-TTLS
ifeq ($(CONFIG_EAP_TTLS), dyn)
CFLAGS += -DEAP_TTLS_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_ttls.so
else
CFLAGS += -DEAP_TTLS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_ttls.c
endif
TLS_FUNCS=y
ifndef CONFIG_FIPS
MS_FUNCS=y
CHAP=y
endif
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_MD5
# EAP-MD5
ifeq ($(CONFIG_EAP_MD5), dyn)
CFLAGS += -DEAP_MD5_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_md5.so
else
CFLAGS += -DEAP_MD5
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_md5.c
endif
CHAP=y
CONFIG_IEEE8021X_EAPOL=y
endif

# backwards compatibility for old spelling
ifdef CONFIG_MSCHAPV2
ifndef CONFIG_EAP_MSCHAPV2
CONFIG_EAP_MSCHAPV2=y
endif
endif

ifdef CONFIG_EAP_MSCHAPV2
# EAP-MSCHAPv2
ifeq ($(CONFIG_EAP_MSCHAPV2), dyn)
CFLAGS += -DEAP_MSCHAPv2_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_mschapv2.so
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/mschapv2.so
else
CFLAGS += -DEAP_MSCHAPv2
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_mschapv2.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/mschapv2.c
endif
MS_FUNCS=y
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_GTC
# EAP-GTC
ifeq ($(CONFIG_EAP_GTC), dyn)
CFLAGS += -DEAP_GTC_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_gtc.so
else
CFLAGS += -DEAP_GTC
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_gtc.c
endif
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_OTP
# EAP-OTP
ifeq ($(CONFIG_EAP_OTP), dyn)
CFLAGS += -DEAP_OTP_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_otp.so
else
CFLAGS += -DEAP_OTP
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_otp.c
endif
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_SIM
# EAP-SIM
ifeq ($(CONFIG_EAP_SIM), dyn)
CFLAGS += -DEAP_SIM_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_sim.so
else
CFLAGS += -DEAP_SIM
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_sim.c
endif
CONFIG_IEEE8021X_EAPOL=y
CONFIG_EAP_SIM_COMMON=y
NEED_AES_CBC=y
endif

ifdef CONFIG_EAP_LEAP
# EAP-LEAP
ifeq ($(CONFIG_EAP_LEAP), dyn)
CFLAGS += -DEAP_LEAP_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_leap.so
else
CFLAGS += -DEAP_LEAP
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_leap.c
endif
MS_FUNCS=y
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_PSK
# EAP-PSK
ifeq ($(CONFIG_EAP_PSK), dyn)
CFLAGS += -DEAP_PSK_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_psk.so
else
CFLAGS += -DEAP_PSK
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_psk.c $(WPA_SUPP_ROOT)/src/eap_common/eap_psk_common.c
endif
CONFIG_IEEE8021X_EAPOL=y
NEED_AES=y
NEED_AES_OMAC1=y
NEED_AES_ENCBLOCK=y
NEED_AES_EAX=y
endif

ifdef CONFIG_EAP_AKA
# EAP-AKA
ifeq ($(CONFIG_EAP_AKA), dyn)
CFLAGS += -DEAP_AKA_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_aka.so
else
CFLAGS += -DEAP_AKA
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_aka.c
endif
CONFIG_IEEE8021X_EAPOL=y
CONFIG_EAP_SIM_COMMON=y
NEED_AES_CBC=y
endif

ifdef CONFIG_EAP_PROXY
CFLAGS += -DCONFIG_EAP_PROXY
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_proxy_$(CONFIG_EAP_PROXY).c
include eap_proxy_$(CONFIG_EAP_PROXY).mak
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_AKA_PRIME
# EAP-AKA'
ifeq ($(CONFIG_EAP_AKA_PRIME), dyn)
CFLAGS += -DEAP_AKA_PRIME_DYNAMIC
else
CFLAGS += -DEAP_AKA_PRIME
endif
NEED_SHA256=y
endif

ifdef CONFIG_EAP_SIM_COMMON
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_common/eap_sim_common.c
NEED_AES=y
NEED_FIPS186_2_PRF=y
endif

ifdef CONFIG_EAP_FAST
# EAP-FAST
ifeq ($(CONFIG_EAP_FAST), dyn)
CFLAGS += -DEAP_FAST_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_fast.so
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_common/eap_fast_common.c
else
CFLAGS += -DEAP_FAST
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_fast.c $(WPA_SUPP_ROOT)/src/eap_peer/eap_fast_pac.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_common/eap_fast_common.c
endif
TLS_FUNCS=y
CONFIG_IEEE8021X_EAPOL=y
NEED_T_PRF=y
endif

ifdef CONFIG_EAP_TEAP
# EAP-TEAP
ifeq ($(CONFIG_EAP_TEAP), dyn)
CFLAGS += -DEAP_TEAP_DYNAMIC
EAPDYN += ../src/eap_peer/eap_teap.so
EAPDYN += ../src/eap_common/eap_teap_common.o
else
CFLAGS += -DEAP_TEAP
OBJS += ../src/eap_peer/eap_teap.o ../src/eap_peer/eap_teap_pac.o
OBJS += ../src/eap_common/eap_teap_common.o
endif
TLS_FUNCS=y
CONFIG_IEEE8021X_EAPOL=y
NEED_T_PRF=y
NEED_SHA384=y
endif

ifdef CONFIG_EAP_PAX
# EAP-PAX
ifeq ($(CONFIG_EAP_PAX), dyn)
CFLAGS += -DEAP_PAX_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_pax.so
else
CFLAGS += -DEAP_PAX
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_pax.c $(WPA_SUPP_ROOT)/src/eap_common/eap_pax_common.c
endif
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_SAKE
# EAP-SAKE
ifeq ($(CONFIG_EAP_SAKE), dyn)
CFLAGS += -DEAP_SAKE_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_sake.so
else
CFLAGS += -DEAP_SAKE
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_sake.c $(WPA_SUPP_ROOT)/src/eap_common/eap_sake_common.c
endif
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_GPSK
# EAP-GPSK
ifeq ($(CONFIG_EAP_GPSK), dyn)
CFLAGS += -DEAP_GPSK_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_gpsk.so
else
CFLAGS += -DEAP_GPSK
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_gpsk.c $(WPA_SUPP_ROOT)/src/eap_common/eap_gpsk_common.c
endif
CONFIG_IEEE8021X_EAPOL=y
ifdef CONFIG_EAP_GPSK_SHA256
CFLAGS += -DEAP_GPSK_SHA256
endif
NEED_SHA256=y
NEED_AES_OMAC1=y
endif

ifdef CONFIG_EAP_PWD
CFLAGS += -DEAP_PWD
ifeq ($(CONFIG_TLS), wolfssl)
CFLAGS += -DCONFIG_ECC
endif
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_pwd.c $(WPA_SUPP_ROOT)/src/eap_common/eap_pwd_common.c
CONFIG_IEEE8021X_EAPOL=y
NEED_SHA256=y
NEED_ECC=y
NEED_DRAGONFLY=y
endif

ifdef CONFIG_EAP_EKE
# EAP-EKE
ifeq ($(CONFIG_EAP_EKE), dyn)
CFLAGS += -DEAP_EKE_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_eke.so
else
CFLAGS += -DEAP_EKE
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_eke.c $(WPA_SUPP_ROOT)/src/eap_common/eap_eke_common.c
endif
CONFIG_IEEE8021X_EAPOL=y
NEED_DH_GROUPS=y
NEED_DH_GROUPS_ALL=y
NEED_SHA256=y
NEED_AES_CBC=y
endif

ifdef CONFIG_WPS
# EAP-WSC
CFLAGS += -DCONFIG_WPS -DEAP_WSC
WPA_SUPP_CSRCS += wps_supplicant.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/uuid.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_wsc.c $(WPA_SUPP_ROOT)/src/eap_common/eap_wsc_common.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_common.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_attr_parse.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_attr_build.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_attr_process.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_dev_attr.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_enrollee.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_registrar.c
CONFIG_IEEE8021X_EAPOL=y
# NEED_DH_GROUPS=y
# CONFIG_INTERNAL_DH_GROUP5=y
# NEED_SHA256=y
NEED_BASE64=y
# NEED_AES_CBC=y
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/dh_group5.c
NEED_MODEXP=y



ifdef CONFIG_WPS_NFC
CFLAGS += -DCONFIG_WPS_NFC
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/ndef.c
NEED_WPS_OOB=y
endif

ifdef NEED_WPS_OOB
CFLAGS += -DCONFIG_WPS_OOB
endif

ifdef CONFIG_WPS_ER
CONFIG_WPS_UPNP=y
CFLAGS += -DCONFIG_WPS_ER
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_er.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_er_ssdp.c
endif

ifdef CONFIG_WPS_UPNP
CFLAGS += -DCONFIG_WPS_UPNP
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_upnp.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_upnp_ssdp.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_upnp_web.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_upnp_event.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_upnp_ap.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/upnp_xml.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/httpread.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/http_client.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/http_server.c
endif

ifdef CONFIG_WPS_STRICT
CFLAGS += -DCONFIG_WPS_STRICT
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_validate.c
endif

ifdef CONFIG_WPS_TESTING
CFLAGS += -DCONFIG_WPS_TESTING
endif

ifdef CONFIG_WPS_REG_DISABLE_OPEN
CFLAGS += -DCONFIG_WPS_REG_DISABLE_OPEN
endif

endif

ifdef CONFIG_EAP_IKEV2
# EAP-IKEv2
ifeq ($(CONFIG_EAP_IKEV2), dyn)
CFLAGS += -DEAP_IKEV2_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_ikev2.so $(WPA_SUPP_ROOT)/src/eap_peer/ikev2.c
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_common/eap_ikev2_common.c $(WPA_SUPP_ROOT)/src/eap_common/ikev2_common.c
else
CFLAGS += -DEAP_IKEV2
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_ikev2.c $(WPA_SUPP_ROOT)/src/eap_peer/ikev2.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_common/eap_ikev2_common.c $(WPA_SUPP_ROOT)/src/eap_common/ikev2_common.c
endif
CONFIG_IEEE8021X_EAPOL=y
NEED_DH_GROUPS=y
NEED_DH_GROUPS_ALL=y
NEED_MODEXP=y
NEED_CIPHER=y
endif

ifdef CONFIG_EAP_VENDOR_TEST
ifeq ($(CONFIG_EAP_VENDOR_TEST), dyn)
CFLAGS += -DEAP_VENDOR_TEST_DYNAMIC
EAPDYN += $(WPA_SUPP_ROOT)/src/eap_peer/eap_vendor_test.so
else
CFLAGS += -DEAP_VENDOR_TEST
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_vendor_test.c
endif
CONFIG_IEEE8021X_EAPOL=y
endif

ifdef CONFIG_EAP_TNC
# EAP-TNC
CFLAGS += -DEAP_TNC
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_tnc.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/tncc.c
NEED_BASE64=y
ifndef CONFIG_NATIVE_WINDOWS
ifndef CONFIG_DRIVER_BSD
LIBS += -ldl
endif
endif
endif

ifdef CONFIG_MACSEC
CFLAGS += -DCONFIG_MACSEC
CONFIG_IEEE8021X_EAPOL=y
NEED_AES_ENCBLOCK=y
NEED_AES_UNWRAP=y
NEED_AES_WRAP=y
NEED_AES_OMAC1=y
OBJS += wpas_kay.o
OBJS += ../src/pae/ieee802_1x_cp.o
OBJS += ../src/pae/ieee802_1x_kay.o
OBJS += ../src/pae/ieee802_1x_key.o
OBJS += ../src/pae/ieee802_1x_secy_ops.o
ifdef CONFIG_AP
OBJS += ../src/ap/wpa_auth_kay.o
endif
endif

ifdef CONFIG_IEEE8021X_EAPOL
# IEEE 802.1X/EAPOL state machines (e.g., for RADIUS authentication)
CFLAGS += -DIEEE8021X_EAPOL
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eapol_supp/eapol_supp_sm.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap.c $(WPA_SUPP_ROOT)/src/eap_peer/eap_methods.c
NEED_EAP_COMMON=y
ifdef CONFIG_DYNAMIC_EAP_METHODS
CFLAGS += -DCONFIG_DYNAMIC_EAP_METHODS
LIBS += -ldl -rdynamic
endif
endif

ifdef CONFIG_AP
NEED_EAP_COMMON=y
NEED_RSN_AUTHENTICATOR=y
CFLAGS += -DCONFIG_AP
WPA_SUPP_CSRCS += ap.c
CFLAGS += -DCONFIG_NO_RADIUS
CFLAGS += -DCONFIG_NO_ACCOUNTING
CFLAGS += -DCONFIG_NO_VLAN
VPATH += $(WPA_SUPP_ROOT)/src/ap
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/hostapd.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/wpa_auth_glue.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/utils.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/authsrv.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/ap_config.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/ip_addr.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/sta_info.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/tkip_countermeasures.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/ap_mlme.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/ieee802_1x.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eapol_auth/eapol_auth_sm.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/ieee802_11_auth.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/ieee802_11_shared.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/drv_callbacks.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/ap_drv_ops.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/beacon.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/bss_load.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/eap_user_db.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/neighbor_db.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/rrm.c
ifdef CONFIG_IEEE80211N
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/ieee802_11_ht.c
ifdef CONFIG_IEEE80211AC
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/ieee802_11_vht.c
endif
ifdef CONFIG_IEEE80211AX
OBJS += ../src/ap/ieee802_11_he.o
endif
endif
ifdef CONFIG_WNM_AP
CFLAGS += -DCONFIG_WNM_AP
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/wnm_ap.c
endif
ifdef CONFIG_MBO
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/mbo_ap.c
endif
ifdef CONFIG_FILS
OBJS += ../src/ap/fils_hlp.o
endif
ifdef CONFIG_CTRL_IFACE
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/ctrl_iface_ap.c
endif

CFLAGS += -DEAP_SERVER -DEAP_SERVER_IDENTITY
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_server/eap_server.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_server/eap_server_identity.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_server/eap_server_methods.c

ifdef CONFIG_IEEE80211N
CFLAGS += -DCONFIG_IEEE80211N
ifdef CONFIG_IEEE80211AC
CFLAGS += -DCONFIG_IEEE80211AC
endif
ifdef CONFIG_IEEE80211AX
CFLAGS += -DCONFIG_IEEE80211AX
endif
endif

ifdef NEED_AP_MLME
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/wmm.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/ap_list.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/ieee802_11.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/hw_features.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/dfs.c
CFLAGS += -DNEED_AP_MLME
endif
ifdef CONFIG_WPS
CFLAGS += -DEAP_SERVER_WSC
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/wps_hostapd.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_server/eap_server_wsc.c
endif
ifdef CONFIG_DPP
OBJS += ../src/ap/dpp_hostapd.o
OBJS += ../src/ap/gas_query_ap.o
endif
ifdef CONFIG_INTERWORKING
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/gas_serv.c
endif
ifdef CONFIG_HS20
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/hs20.c
endif
endif

ifdef CONFIG_MBO
OBJS += mbo.o
CFLAGS += -DCONFIG_MBO
endif

ifdef NEED_RSN_AUTHENTICATOR
CFLAGS += -DCONFIG_NO_RADIUS
NEED_AES_WRAP=y
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/wpa_auth.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/wpa_auth_ie.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/pmksa_cache_auth.c
endif

ifdef CONFIG_ACS
CFLAGS += -DCONFIG_ACS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/ap/acs.c
LIBS += -lm
endif

ifdef CONFIG_PCSC
# PC/SC interface for smartcards (USIM, GSM SIM)
CFLAGS += -DPCSC_FUNCS -I/usr/include/PCSC
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/pcsc_funcs.c
# -lpthread may not be needed depending on how pcsc-lite was configured
ifdef CONFIG_NATIVE_WINDOWS
#Once MinGW gets support for WinScard, -lwinscard could be used instead of the
#dynamic symbol loading that is now used in pcsc_funcs.c
#LIBS += -lwinscard
else
ifdef CONFIG_OSX
LIBS += -framework PCSC
else
LIBS += -lpcsclite -lpthread
endif
endif
endif

ifdef CONFIG_SIM_SIMULATOR
CFLAGS += -DCONFIG_SIM_SIMULATOR
NEED_MILENAGE=y
endif

ifdef CONFIG_USIM_SIMULATOR
CFLAGS += -DCONFIG_USIM_SIMULATOR
NEED_MILENAGE=y
endif

ifdef NEED_MILENAGE
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/milenage.c
NEED_AES_ENCBLOCK=y
endif

ifdef CONFIG_PKCS12
CFLAGS += -DPKCS12_FUNCS
endif

ifdef CONFIG_SMARTCARD
CFLAGS += -DCONFIG_SMARTCARD
endif

ifdef NEED_DRAGONFLY
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/common/dragonfly.c
endif

ifdef MS_FUNCS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/ms_funcs.c
NEED_DES=y
NEED_MD4=y
endif

ifdef CHAP
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_common/chap.c
endif

ifdef TLS_FUNCS
NEED_DES=y
# Shared TLS functions (needed for EAP_TLS, EAP_PEAP, EAP_TTLS, EAP_FAST, and
# EAP_TEAP)
OWPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_peer/eap_tls_common.c
OBJS += ../src/eap_peer/eap_tls_common.o
ifndef CONFIG_FIPS
NEED_TLS_PRF=y
NEED_SHA1=y
NEED_MD5=y
endif
endif

ifndef CONFIG_TLS
CONFIG_TLS=openssl
endif

ifdef CONFIG_TLSV11
CFLAGS += -DCONFIG_TLSV11
endif

ifdef CONFIG_TLSV12
CFLAGS += -DCONFIG_TLSV12
NEED_SHA256=y
endif

ifeq ($(CONFIG_TLS), wolfssl)
ifdef TLS_FUNCS
CFLAGS += -DWOLFSSL_DER_LOAD -I/usr/local/include/wolfssl
OBJS += ../src/crypto/tls_wolfssl.o
endif
OBJS += ../src/crypto/crypto_wolfssl.o
OBJS_p += ../src/crypto/crypto_wolfssl.o
ifdef NEED_FIPS186_2_PRF
OBJS += ../src/crypto/fips_prf_wolfssl.o
endif
NEED_TLS_PRF_SHA256=y
LIBS += -lwolfssl -lm
LIBS_p += -lwolfssl -lm
endif

ifeq ($(CONFIG_TLS), openssl)
ifdef TLS_FUNCS
CFLAGS += -DEAP_TLS_OPENSSL
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/tls_openssl.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/tls_openssl_ocsp.c
LIBS += -lssl
endif
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/crypto_openssl.c
WPA_SUPP_CSRCS_p += $(WPA_SUPP_ROOT)/src/crypto/crypto_openssl.c
WPA_SUPP_CSRCS_priv += $(WPA_SUPP_ROOT)/src/crypto/crypto_openssl.c
ifdef NEED_FIPS186_2_PRF
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/fips_prf_openssl.c
endif
NEED_SHA256=y
NEED_TLS_PRF_SHA256=y
LIBS += -lcrypto
LIBS_p += -lcrypto
ifdef CONFIG_TLS_ADD_DL
LIBS += -ldl
LIBS_p += -ldl
endif
ifndef CONFIG_TLS_DEFAULT_CIPHERS
CONFIG_TLS_DEFAULT_CIPHERS = "DEFAULT:!EXP:!LOW"
endif
CFLAGS += -DTLS_DEFAULT_CIPHERS=\"$(CONFIG_TLS_DEFAULT_CIPHERS)\"
endif

ifeq ($(CONFIG_TLS), gnutls)
ifndef CONFIG_CRYPTO
# default to libgcrypt
CONFIG_CRYPTO=gnutls
endif
ifdef TLS_FUNCS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/tls_gnutls.c
LIBS += -lgnutls -lgpg-error
endif
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/crypto_$(CONFIG_CRYPTO).c
WPA_SUPP_CSRCS_p += $(WPA_SUPP_ROOT)/src/crypto/crypto_$(CONFIG_CRYPTO).c
WPA_SUPP_CSRCS_priv += $(WPA_SUPP_ROOT)/src/crypto/crypto_$(CONFIG_CRYPTO).c
ifdef NEED_FIPS186_2_PRF
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/fips_prf_internal.c
SHA1WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha1-internal.c
endif
ifeq ($(CONFIG_CRYPTO), gnutls)
LIBS += -lgcrypt
LIBS_p += -lgcrypt
CONFIG_INTERNAL_RC4=y
CONFIG_INTERNAL_DH_GROUP5=y
endif
ifeq ($(CONFIG_CRYPTO), nettle)
LIBS += -lnettle -lgmp
LIBS_p += -lnettle -lgmp
CONFIG_INTERNAL_RC4=y
CONFIG_INTERNAL_DH_GROUP5=y
endif
endif

ifeq ($(CONFIG_TLS), internal)
ifndef CONFIG_CRYPTO
CONFIG_CRYPTO=internal
endif
ifdef TLS_FUNCS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/crypto_internal-rsa.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/tls_internal.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/tlsv1_common.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/tlsv1_record.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/tlsv1_cred.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/tlsv1_client.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/tlsv1_client_write.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/tlsv1_client_read.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/tlsv1_client_ocsp.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/asn1.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/rsa.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/x509v3.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/pkcs1.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/pkcs5.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/pkcs8.c
NEED_SHA256=y
NEED_BASE64=y
NEED_TLS_PRF=y
ifdef CONFIG_TLSV12
NEED_TLS_PRF_SHA256=y
endif
NEED_MODEXP=y
NEED_CIPHER=y
CFLAGS += -DCONFIG_TLS_INTERNAL_CLIENT
endif
ifdef NEED_CIPHER
NEED_DES=y
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/crypto_internal-cipher.c
endif
ifdef NEED_MODEXP
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/crypto_internal-modexp.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/tls/bignum.c
endif
ifeq ($(CONFIG_CRYPTO), libtomcrypt)
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/crypto_libtomcrypt.c
WPA_SUPP_CSRCS_p += $(WPA_SUPP_ROOT)/src/crypto/crypto_libtomcrypt.c
LIBS += -ltomcrypt -ltfm
LIBS_p += -ltomcrypt -ltfm
CONFIG_INTERNAL_SHA256=y
CONFIG_INTERNAL_RC4=y
CONFIG_INTERNAL_DH_GROUP5=y
endif
ifeq ($(CONFIG_CRYPTO), internal)
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/crypto_internal.c
WPA_SUPP_CSRCS_p += $(WPA_SUPP_ROOT)/src/crypto/crypto_internal.c
NEED_AES_ENC=y
CFLAGS += -DCONFIG_CRYPTO_INTERNAL
ifdef CONFIG_INTERNAL_LIBTOMMATH
CFLAGS += -DCONFIG_INTERNAL_LIBTOMMATH
ifdef CONFIG_INTERNAL_LIBTOMMATH_FAST
CFLAGS += -DLTM_FAST
endif
else
LIBS += -ltommath
LIBS_p += -ltommath
endif
CONFIG_INTERNAL_AES=n
CONFIG_INTERNAL_DES=n
CONFIG_INTERNAL_SHA1=y
CONFIG_INTERNAL_MD4=n
CONFIG_INTERNAL_MD5=n
CONFIG_INTERNAL_SHA256=n
CONFIG_INTERNAL_SHA384=n
CONFIG_INTERNAL_SHA512=n
CONFIG_INTERNAL_RC4=n
CONFIG_INTERNAL_DH_GROUP5=n
endif
ifeq ($(CONFIG_CRYPTO), cryptoapi)
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/crypto_cryptoapi.c
WPA_SUPP_CSRCS_p += $(WPA_SUPP_ROOT)/src/crypto/crypto_cryptoapi.c
CFLAGS += -DCONFIG_CRYPTO_CRYPTOAPI
CONFIG_INTERNAL_SHA256=y
CONFIG_INTERNAL_RC4=y
endif
endif

ifeq ($(CONFIG_TLS), linux)
OBJS += ../src/crypto/crypto_linux.o
OBJS_p += ../src/crypto/crypto_linux.o
ifdef TLS_FUNCS
OBJS += ../src/crypto/crypto_internal-rsa.o
OBJS += ../src/crypto/tls_internal.o
OBJS += ../src/tls/tlsv1_common.o
OBJS += ../src/tls/tlsv1_record.o
OBJS += ../src/tls/tlsv1_cred.o
OBJS += ../src/tls/tlsv1_client.o
OBJS += ../src/tls/tlsv1_client_write.o
OBJS += ../src/tls/tlsv1_client_read.o
OBJS += ../src/tls/tlsv1_client_ocsp.o
OBJS += ../src/tls/asn1.o
OBJS += ../src/tls/rsa.o
OBJS += ../src/tls/x509v3.o
OBJS += ../src/tls/pkcs1.o
OBJS += ../src/tls/pkcs5.o
OBJS += ../src/tls/pkcs8.o
NEED_SHA256=y
NEED_BASE64=y
NEED_TLS_PRF=y
ifdef CONFIG_TLSV12
NEED_TLS_PRF_SHA256=y
endif
NEED_MODEXP=y
NEED_CIPHER=y
CFLAGS += -DCONFIG_TLS_INTERNAL_CLIENT
endif
ifdef NEED_MODEXP
OBJS += ../src/crypto/crypto_internal-modexp.o
OBJS += ../src/tls/bignum.o
CFLAGS += -DCONFIG_INTERNAL_LIBTOMMATH
CFLAGS += -DLTM_FAST
endif
CONFIG_INTERNAL_DH_GROUP5=y
ifdef NEED_FIPS186_2_PRF
OBJS += ../src/crypto/fips_prf_internal.o
OBJS += ../src/crypto/sha1-internal.o
endif
endif

ifeq ($(CONFIG_TLS), none)
ifdef TLS_FUNCS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/tls_none.c
CFLAGS += -DEAP_TLS_NONE
CONFIG_INTERNAL_AES=y
CONFIG_INTERNAL_SHA1=y
CONFIG_INTERNAL_MD5=y
endif
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/crypto_none.c
WPA_SUPP_CSRCS_p += $(WPA_SUPP_ROOT)/src/crypto/crypto_none.c
CONFIG_INTERNAL_SHA256=y
CONFIG_INTERNAL_RC4=y
endif

ifdef TLS_FUNCS
ifdef CONFIG_SMARTCARD
ifndef CONFIG_NATIVE_WINDOWS
ifneq ($(CONFIG_L2_PACKET), freebsd)
LIBS += -ldl
endif
endif
endif
endif

ifndef TLS_FUNCS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/tls_none.c
ifeq ($(CONFIG_TLS), internal)
CONFIG_INTERNAL_AES=y
CONFIG_INTERNAL_SHA1=y
CONFIG_INTERNAL_MD5=y
CONFIG_INTERNAL_RC4=y
endif
endif

AESWPA_SUPP_CSRCS = # none so far (see below)
ifdef CONFIG_INTERNAL_AES
AESWPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/aes-internal.c $(WPA_SUPP_ROOT)/src/crypto/aes-internal-dec.c
endif

ifneq ($(CONFIG_TLS), openssl)
ifneq ($(CONFIG_TLS), wolfssl)
NEED_INTERNAL_AES_WRAP=y
endif
endif
ifdef CONFIG_OPENSSL_INTERNAL_AES_WRAP
# Seems to be needed at least with BoringSSL
NEED_INTERNAL_AES_WRAP=y
CFLAGS += -DCONFIG_OPENSSL_INTERNAL_AES_WRAP
endif
ifdef CONFIG_FIPS
# Have to use internal AES key wrap routines to use OpenSSL EVP since the
# OpenSSL AES_wrap_key()/AES_unwrap_key() API is not available in FIPS mode.
NEED_INTERNAL_AES_WRAP=y
endif

ifdef NEED_INTERNAL_AES_WRAP
AESWPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/aes-unwrap.c
endif
ifdef NEED_AES_EAX
AESWPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/aes-eax.c
NEED_AES_CTR=y
endif
ifdef NEED_AES_CTR
AESWPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/aes-ctr.c
endif
ifdef NEED_AES_ENCBLOCK
AESWPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/aes-encblock.c
endif
ifdef NEED_AES_OMAC1
NEED_AES_ENC=y
ifdef CONFIG_OPENSSL_CMAC
CFLAGS += -DCONFIG_OPENSSL_CMAC
else
AESWPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/aes-omac1.c
endif
endif
ifdef NEED_AES_SIV
AESWPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/aes-siv.c
endif
ifdef NEED_AES_WRAP
NEED_AES_ENC=y
ifdef NEED_INTERNAL_AES_WRAP
AESWPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/aes-wrap.c
endif
endif
ifdef NEED_AES_CBC
NEED_AES_ENC=y
ifneq ($(CONFIG_TLS), openssl)
AESWPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/aes-cbc.c
endif
endif
ifdef NEED_AES_ENC
ifdef CONFIG_INTERNAL_AES
AESWPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/aes-internal-enc.c
endif
endif
ifdef NEED_AES
WPA_SUPP_CSRCS += $(AESWPA_SUPP_CSRCS)
endif

ifdef NEED_SHA1
ifneq ($(CONFIG_TLS), openssl)
ifneq ($(CONFIG_TLS), mbedtls)
SHA1WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha1.c
endif
endif
SHA1WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha1-prf.c
ifdef CONFIG_INTERNAL_SHA1
SHA1WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha1-internal.c
ifdef NEED_FIPS186_2_PRF
SHA1WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/fips_prf_internal.c
endif
endif
ifdef CONFIG_NO_WPA_PASSPHRASE
CFLAGS += -DCONFIG_NO_PBKDF2
else
ifneq ($(CONFIG_TLS), openssl)
ifneq ($(CONFIG_TLS), mbedtls)
SHA1WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha1-pbkdf2.c
endif
endif
endif
ifdef NEED_T_PRF
SHA1WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha1-tprf.c
endif
ifdef NEED_TLS_PRF
SHA1WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha1-tlsprf.c
endif
endif

ifndef CONFIG_FIPS
ifneq ($(CONFIG_TLS), openssl)
ifneq ($(CONFIG_TLS), mbedtls)
MD5WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/md5.c
endif
endif
endif
ifdef NEED_MD5
ifdef CONFIG_INTERNAL_MD5
MD5WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/md5-internal.c
endif
WPA_SUPP_CSRCS += $(MD5WPA_SUPP_CSRCS)
WPA_SUPP_CSRCS_p += $(MD5WPA_SUPP_CSRCS)
WPA_SUPP_CSRCS_priv += $(MD5WPA_SUPP_CSRCS)
endif

ifdef NEED_MD4
ifdef CONFIG_INTERNAL_MD4
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/md4-internal.c
endif
endif

DESWPA_SUPP_CSRCS = # none needed when not internal
ifdef NEED_DES
ifdef CONFIG_INTERNAL_DES
DESWPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/des-internal.c
endif
endif

ifdef CONFIG_NO_RC4
CFLAGS += -DCONFIG_NO_RC4
endif

ifdef NEED_RC4
ifdef CONFIG_INTERNAL_RC4
ifndef CONFIG_NO_RC4
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/rc4.c
endif
endif
endif

SHA256WPA_SUPP_CSRCS = # none by default
ifdef NEED_SHA256
CFLAGS += -DCONFIG_SHA256
ifneq ($(CONFIG_TLS), openssl)
SHA256WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha256-ori.c
#SHA256WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha256.c
endif
SHA256WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha256-prf.c
ifdef CONFIG_INTERNAL_SHA256
SHA256WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha256-internal.c
endif
ifdef CONFIG_INTERNAL_SHA384
CFLAGS += -DCONFIG_INTERNAL_SHA384
SHA256WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha384-internal.c
endif
ifdef CONFIG_INTERNAL_SHA512
CFLAGS += -DCONFIG_INTERNAL_SHA512
SHA256WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha512-internal.c
endif
ifdef NEED_TLS_PRF_SHA256
SHA256WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha256-tlsprf.c
endif
ifdef NEED_HMAC_SHA256_KDF
CFLAGS += -DCONFIG_HMAC_SHA256_KDF
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha256-kdf.c
endif
ifdef NEED_HMAC_SHA384_KDF
CFLAGS += -DCONFIG_HMAC_SHA256_KDF
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha384-kdf.c
endif
ifdef NEED_HMAC_SHA512_KDF
CFLAGS += -DCONFIG_HMAC_SHA256_KDF
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha512-kdf.c
endif
WPA_SUPP_CSRCS += $(SHA256WPA_SUPP_CSRCS)
endif
ifdef NEED_SHA384
CFLAGS += -DCONFIG_SHA384
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/sha384-prf.c
endif

ifdef NEED_DH_GROUPS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/dh_groups.c
endif
ifdef NEED_DH_GROUPS_ALL
CFLAGS += -DALL_DH_GROUPS
endif
ifdef CONFIG_INTERNAL_DH_GROUP5
ifdef NEED_DH_GROUPS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/dh_group5.c
endif
endif

ifdef NEED_ECC
CFLAGS += -DCONFIG_ECC
endif

ifdef CONFIG_NO_RANDOM_POOL
CFLAGS += -DCONFIG_NO_RANDOM_POOL
else
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/random.c
endif

ifdef CONFIG_CTRL_IFACE
ifeq ($(CONFIG_CTRL_IFACE), y)
ifdef CONFIG_NATIVE_WINDOWS
CONFIG_CTRL_IFACE=named_pipe
else
CONFIG_CTRL_IFACE=unix
endif
endif
CFLAGS += -DCONFIG_CTRL_IFACE
ifeq ($(CONFIG_CTRL_IFACE), unix)
CFLAGS += -DCONFIG_CTRL_IFACE_UNIX
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/common/ctrl_iface_common.c
endif
ifeq ($(CONFIG_CTRL_IFACE), udp)
CFLAGS += -DCONFIG_CTRL_IFACE_UDP
endif
ifeq ($(CONFIG_CTRL_IFACE), udp6)
CONFIG_CTRL_IFACE=udp
CFLAGS += -DCONFIG_CTRL_IFACE_UDP
CFLAGS += -DCONFIG_CTRL_IFACE_UDP_IPV6
endif
ifeq ($(CONFIG_CTRL_IFACE), named_pipe)
CFLAGS += -DCONFIG_CTRL_IFACE_NAMED_PIPE
endif
ifeq ($(CONFIG_CTRL_IFACE), udp-remote)
CONFIG_CTRL_IFACE=udp
CFLAGS += -DCONFIG_CTRL_IFACE_UDP
CFLAGS += -DCONFIG_CTRL_IFACE_UDP_REMOTE
endif
ifeq ($(CONFIG_CTRL_IFACE), udp6-remote)
CONFIG_CTRL_IFACE=udp
CFLAGS += -DCONFIG_CTRL_IFACE_UDP
CFLAGS += -DCONFIG_CTRL_IFACE_UDP_REMOTE
CFLAGS += -DCONFIG_CTRL_IFACE_UDP_IPV6
endif
WPA_SUPP_CSRCS += ctrl_iface.c ctrl_iface_$(CONFIG_CTRL_IFACE).c
endif

ifdef CONFIG_CTRL_IFACE_DBUS
DBUS=y
DBUS_CFLAGS += -DCONFIG_CTRL_IFACE_DBUS -DDBUS_API_SUBJECT_TO_CHANGE
DBUS_WPA_SUPP_CSRCS += dbus/dbus_old.c dbus/dbus_old_handlers.c
ifdef CONFIG_WPS
DBUS_WPA_SUPP_CSRCS += dbus/dbus_old_handlers_wps.c
endif
DBUS_WPA_SUPP_CSRCS += dbus/dbus_dict_helpers.c
ifndef DBUS_LIBS
DBUS_LIBS := $(shell $(PKG_CONFIG) --libs dbus-1)
endif
ifndef DBUS_INCLUDE
DBUS_INCLUDE := $(shell $(PKG_CONFIG) --cflags dbus-1)
endif
DBUS_CFLAGS += $(DBUS_INCLUDE)
DBUS_INTERFACE=fi.epitest.hostap.WPASupplicant
endif

ifdef CONFIG_CTRL_IFACE_DBUS_NEW
DBUS=y
DBUS_CFLAGS += -DCONFIG_CTRL_IFACE_DBUS_NEW
DBUS_WPA_SUPP_CSRCS ?= dbus/dbus_dict_helpers.c
DBUS_WPA_SUPP_CSRCS += dbus/dbus_new_helpers.c
DBUS_WPA_SUPP_CSRCS += dbus/dbus_new.c dbus/dbus_new_handlers.c
ifdef CONFIG_WPS
DBUS_WPA_SUPP_CSRCS += dbus/dbus_new_handlers_wps.c
endif
ifdef CONFIG_P2P
DBUS_WPA_SUPP_CSRCS += dbus/dbus_new_handlers_p2p.c
endif
ifndef DBUS_LIBS
DBUS_LIBS := $(shell $(PKG_CONFIG) --libs dbus-1)
endif
ifndef DBUS_INCLUDE
DBUS_INCLUDE := $(shell $(PKG_CONFIG) --cflags dbus-1)
endif
ifdef CONFIG_CTRL_IFACE_DBUS_INTRO
DBUS_WPA_SUPP_CSRCS += dbus/dbus_new_introspect.c
DBUS_CFLAGS += -DCONFIG_CTRL_IFACE_DBUS_INTRO
endif
DBUS_CFLAGS += $(DBUS_INCLUDE)
DBUS_INTERFACE=fi.w1.wpa_supplicant1
endif

ifdef DBUS
DBUS_CFLAGS += -DCONFIG_DBUS
DBUS_WPA_SUPP_CSRCS += dbus/dbus_common.c
endif

WPA_SUPP_CSRCS += $(DBUS_WPA_SUPP_CSRCS)
CFLAGS += $(DBUS_CFLAGS)
LIBS += $(DBUS_LIBS)

ifdef CONFIG_READLINE
WPA_SUPP_CSRCS_c += $(WPA_SUPP_ROOT)/src/utils/edit_readline.c
LIBS_c += -lreadline -lncurses
else
ifdef CONFIG_WPA_CLI_EDIT
WPA_SUPP_CSRCS_c += $(WPA_SUPP_ROOT)/src/utils/edit.c
else
WPA_SUPP_CSRCS_c += $(WPA_SUPP_ROOT)/src/utils/edit_simple.c
endif
endif

ifdef CONFIG_NATIVE_WINDOWS
CFLAGS += -DCONFIG_NATIVE_WINDOWS
LIBS += -lws2_32 -lgdi32 -lcrypt32
LIBS_c += -lws2_32
LIBS_p += -lws2_32 -lgdi32
ifeq ($(CONFIG_CRYPTO), cryptoapi)
LIBS_p += -lcrypt32
endif
endif

ifdef CONFIG_NO_STDOUT_DEBUG
CFLAGS += -DCONFIG_NO_STDOUT_DEBUG
ifndef CONFIG_CTRL_IFACE
CFLAGS += -DCONFIG_NO_WPA_MSG
endif
endif

ifdef CONFIG_IPV6
# for eapol_test only
CFLAGS += -DCONFIG_IPV6
endif

ifdef CONFIG_NO_LINUX_PACKET_SOCKET_WAR
CFLAGS += -DCONFIG_NO_LINUX_PACKET_SOCKET_WAR
endif

ifdef NEED_BASE64
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/base64.c
endif

ifdef NEED_SME
WPA_SUPP_CSRCS += sme.c
CFLAGS += -DCONFIG_SME
endif

WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/common/ieee802_11_common.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/common/hw_features_common.c

ifdef NEED_EAP_COMMON
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/eap_common/eap_common.c
endif

ifndef CONFIG_MAIN
CONFIG_MAIN=main
endif

ifdef CONFIG_DEBUG_SYSLOG
CFLAGS += -DCONFIG_DEBUG_SYSLOG
ifdef CONFIG_DEBUG_SYSLOG_FACILITY
CFLAGS += -DLOG_HOSTAPD="$(CONFIG_DEBUG_SYSLOG_FACILITY)"
endif
endif


ifdef CONFIG_DEBUG_LINUX_TRACING
CFLAGS += -DCONFIG_DEBUG_LINUX_TRACING
endif

ifdef CONFIG_DEBUG_FILE
CFLAGS += -DCONFIG_DEBUG_FILE
endif

ifdef CONFIG_DELAYED_MIC_ERROR_REPORT
CFLAGS += -DCONFIG_DELAYED_MIC_ERROR_REPORT
endif

ifdef CONFIG_FIPS
CFLAGS += -DCONFIG_FIPS
ifneq ($(CONFIG_TLS), openssl)
$(error CONFIG_FIPS=y requires CONFIG_TLS=openssl)
endif
endif

WPA_SUPP_CSRCS += $(SHA1WPA_SUPP_CSRCS) $(DESWPA_SUPP_CSRCS)

WPA_SUPP_CSRCS_p += $(SHA1WPA_SUPP_CSRCS)
WPA_SUPP_CSRCS_p += $(SHA256WPA_SUPP_CSRCS)
WPA_SUPP_CSRCS_priv += $(SHA1WPA_SUPP_CSRCS)

ifdef CONFIG_BGSCAN_SIMPLE
CFLAGS += -DCONFIG_BGSCAN_SIMPLE
WPA_SUPP_CSRCS += bgscan_simple.c
NEED_BGSCAN=y
endif

ifdef CONFIG_BGSCAN_LEARN
CFLAGS += -DCONFIG_BGSCAN_LEARN
WPA_SUPP_CSRCS += bgscan_learn.c
NEED_BGSCAN=y
endif

ifdef NEED_BGSCAN
CFLAGS += -DCONFIG_BGSCAN
WPA_SUPP_CSRCS += bgscan.c
endif

ifdef CONFIG_AUTOSCAN_EXPONENTIAL
CFLAGS += -DCONFIG_AUTOSCAN_EXPONENTIAL
WPA_SUPP_CSRCS += autoscan_exponential.c
NEED_AUTOSCAN=y
endif

ifdef CONFIG_AUTOSCAN_PERIODIC
CFLAGS += -DCONFIG_AUTOSCAN_PERIODIC
WPA_SUPP_CSRCS += autoscan_periodic.c
NEED_AUTOSCAN=y
endif

ifdef NEED_AUTOSCAN
CFLAGS += -DCONFIG_AUTOSCAN
WPA_SUPP_CSRCS += autoscan.c
endif

ifdef CONFIG_EXT_PASSWORD_TEST
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/ext_password_test.c
CFLAGS += -DCONFIG_EXT_PASSWORD_TEST
NEED_EXT_PASSWORD=y
endif

ifdef NEED_EXT_PASSWORD
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/ext_password.c
CFLAGS += -DCONFIG_EXT_PASSWORD
endif

ifdef NEED_GAS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/common/gas.c
WPA_SUPP_CSRCS += gas_query.c
CFLAGS += -DCONFIG_GAS
NEED_OFFCHANNEL=y
endif

ifdef NEED_OFFCHANNEL
WPA_SUPP_CSRCS += offchannel.c
CFLAGS += -DCONFIG_OFFCHANNEL
endif

ifdef CONFIG_MODULE_TESTS
CFLAGS += -DCONFIG_MODULE_TESTS
WPA_SUPP_CSRCS += wpas_module_tests.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/utils_module_tests.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/common/common_module_tests.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/crypto/crypto_module_tests.c
ifdef CONFIG_WPS
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/wps/wps_module_tests.c
endif
ifndef CONFIG_P2P
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/utils/bitfield.c
endif
endif

WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/drivers/driver_common.c
WPA_SUPP_CSRCS_priv += $(WPA_SUPP_ROOT)/src/drivers/driver_common.c

WPA_SUPP_CSRCS += wpa_supplicant.c events.c blacklist.c wpas_glue.c scan.c
WPA_SUPP_CSRCS_t := $(WPA_SUPP_CSRCS) $(WPA_SUPP_CSRCS_l2) eapol_test.c
WPA_SUPP_CSRCS_t += $(WPA_SUPP_ROOT)/src/radius/radius_client.c
WPA_SUPP_CSRCS_t += $(WPA_SUPP_ROOT)/src/radius/radius.c
ifndef CONFIG_AP
WPA_SUPP_CSRCS_t += $(WPA_SUPP_ROOT)/src/utils/ip_addr.c
endif
WPA_SUPP_CSRCS_t2 := $(WPA_SUPP_CSRCS) $(WPA_SUPP_CSRCS_l2) preauth_test.c

WPA_SUPP_CSRCS_nfc := $(WPA_SUPP_CSRCS) $(WPA_SUPP_CSRCS_l2) nfc_pw_token.c
WPA_SUPP_CSRCS_nfc += $(DRV_OBJS) $(WPA_SUPP_ROOT)/src/drivers/drivers.c

WPA_SUPP_CSRCS += $(CONFIG_MAIN).c

ifdef CONFIG_PRIVSEP
WPA_SUPP_CSRCS_priv += $(DRV_OBJS) $(WPA_SUPP_ROOT)/src/drivers/drivers.c
WPA_SUPP_CSRCS_priv += $(WPA_SUPP_CSRCS_l2)
WPA_SUPP_CSRCS_priv += $(WPA_SUPP_ROOT)/src/utils/os_$(CONFIG_OS).c
WPA_SUPP_CSRCS_priv += $(WPA_SUPP_ROOT)/src/utils/$(CONFIG_ELOOP).c
WPA_SUPP_CSRCS_priv += $(WPA_SUPP_ROOT)/src/utils/common.c
WPA_SUPP_CSRCS_priv += $(WPA_SUPP_ROOT)/src/utils/wpa_debug.c
WPA_SUPP_CSRCS_priv += $(WPA_SUPP_ROOT)/src/utils/wpabuf.c
WPA_SUPP_CSRCS_priv += wpa_priv.c
ifdef CONFIG_DRIVER_NL80211
WPA_SUPP_CSRCS_priv += $(WPA_SUPP_ROOT)/src/common/ieee802_11_common.c
endif
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/l2_packet/l2_packet_privsep.c
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/drivers/driver_privsep.c
EXTRA_progs += wpa_priv
endif

ifdef CONFIG_DRIVER_FREERTOS
CFLAGS += -DCONFIG_DRIVER_FREERTOS
WPA_SUPP_CSRCS += $(DRV_OBJS) $(WPA_SUPP_ROOT)/src/drivers/drivers.c
WPA_SUPP_CSRCS += $(WPA_SUPP_CSRCS_l2)
endif

ifdef CONFIG_NDIS_EVENTS_INTEGRATED
CFLAGS += -DCONFIG_NDIS_EVENTS_INTEGRATED
WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/drivers/ndis_events.c
EXTRALIBS += -loleaut32 -lole32 -luuid
ifdef PLATFORMSDKLIB
EXTRALIBS += $(PLATFORMSDKLIB)/WbemUuid.Lib
else
EXTRALIBS += WbemUuid.Lib
endif
endif

ifdef CONFIG_FST
CFLAGS += -DCONFIG_FST
ifdef CONFIG_FST_TEST
CFLAGS += -DCONFIG_FST_TEST
endif
FST_WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/fst/fst.c
FST_WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/fst/fst_session.c
FST_WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/fst/fst_iface.c
FST_WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/fst/fst_group.c
FST_WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/fst/fst_ctrl_aux.c
ifdef CONFIG_CTRL_IFACE
FST_WPA_SUPP_CSRCS += $(WPA_SUPP_ROOT)/src/fst/fst_ctrl_iface.c
endif
WPA_SUPP_CSRCS += $(FST_WPA_SUPP_CSRCS)
WPA_SUPP_CSRCS_t += $(FST_WPA_SUPP_CSRCS)
WPA_SUPP_CSRCS_t2 += $(FST_WPA_SUPP_CSRCS)
WPA_SUPP_CSRCS_nfc += $(FST_WPA_SUPP_CSRCS)
endif

ifdef CONFIG_NDP_PREQ
CFLAGS += -DCONFIG_NDP_PREQ
endif

ifdef CONFIG_NO_FAST_ASSOC
CFLAGS += -DCONFIG_NO_FAST_ASSOC
endif

ifdef CONFIG_WPA_MSG
CFLAGS += -DCONFIG_WPA_MSG
endif
