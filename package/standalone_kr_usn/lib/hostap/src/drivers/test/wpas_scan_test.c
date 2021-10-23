#include "nrc-unit-test.h"
#include "CUnit/Basic.h"
#include "system.h"
#include "utils/common.h"
#include "common/ieee802_11_common.h"
#include "driver.h"
#include "driver_nrc_scan.h"

static void cancel_cb(struct nrc_scan_info* scan)
{
	wpa_printf(MSG_DEBUG, "Cancel scan: Reason Overflow");
	scan_stop(scan);
	scan_start(scan);
}

/* The suite initialization function.
 * Configure initial settings and variables if necessary.
 * Returns zero on success, non-zero otherwise.
 */
static int init_wpas_scan(void)
{
	return 0;
}

/* The suite cleanup function.
 * Clean up any variables used for testing if needed.
 * Returns zero on success, non-zero otherwise.
 */
static int clean_wpas_scan(void)
{
	return 0;
}

static void test_ds_param(void)
{
	uint8_t f[] = {
		0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0x02, 0x00, 0xeb, 0x1b, 0xe9, 0xcd, 0x02,
		0x00, 0xeb, 0x1b, 0xe9, 0xcd, 0x00, 0xb0, 0x00,
		0x00, 0x45, 0xce, 0x97, 0x00, 0x64, 0x00, 0x64,
		0x00, 0x01, 0x02, 0x00, 0x05, 0x68, 0x61, 0x6c,
		0x6f, 0x77, 0x01, 0x08, 0x00, 0x00, 0x00, 0x04,
		0x00, 0x00, 0x00, 0xfd, /* DS PARAM ELEMENT */0x03, 0x01, 0x01, };
	int i = 0;
	struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *) f;
	const int ie_offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);
	struct ieee802_11_elems elems;
	struct nrc_scan_info* scan = scan_init(cancel_cb);

	ieee802_11_parse_elems(f + ie_offset, sizeof(f) - ie_offset, &elems, 1);

	wpa_printf(MSG_DEBUG, "Current dsparam is %d", elems.ds_params[0]);

	scan_start(scan);

	/* should be ok with 2412 */
	CU_ASSERT(scan_add(scan, 2412, 0, f, sizeof(f)) == 0);
	mgmt->sa[1]++;
	/* scan_add() should return false with other channels */
	for (int i = 2417; i < 2472; i += 5) {
		CU_ASSERT_FALSE(scan_add(scan, i, 0, f, sizeof(f)) == 0);
		mgmt->sa[1]++;
	}
	CU_ASSERT_FALSE(scan_add(scan, 2484 /* ch 14 */, 0, f, sizeof(f)) == 0);

	scan_stop(scan);
	scan_deinit(scan);
}

static void test_scan_run(void)
{
	uint8_t f[] = {
		0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0x02, 0x00, 0xeb, 0x1b, 0xe9, 0xcd, 0x02,
		0x00, 0xeb, 0x1b, 0xe9, 0xcd, 0x00, 0xb0, 0x00,
		0x00, 0x45, 0xce, 0x97, 0x00, 0x64, 0x00, 0x64,
		0x00, 0x01, 0x02, 0x00, 0x05, 0x68, 0x61, 0x6c,
		0x6f, 0x77, 0x01, 0x08, 0x00, 0x00, 0x00, 0x04,
		0x00, 0x00, 0x00, 0xfd, /* DS PARAM ELEMENT */0x03, 0x01, 0x01, };
	struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *) f;
	struct nrc_scan_info* scan = scan_init(cancel_cb);
	/**
	  * scan_add() should be failed before scan_start() called
	  */
	CU_ASSERT_FALSE(scan_add(scan, 2412, 0, f, sizeof(f)) == 0);
	scan_start(scan);
	mgmt->sa[1]++;
	CU_ASSERT(scan_add(scan, 2412, 0, f, sizeof(f)) == 0);
	scan_stop(scan);
	scan_deinit(scan);
}

static void test_scan_same_ch_same_ap(void)
{
	uint8_t f[] = {
		0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0x02, 0x00, 0xeb, 0x1b, 0xe9, 0xcd, 0x02,
		0x00, 0xeb, 0x1b, 0xe9, 0xcd, 0x00, 0xb0, 0x00,
		0x00, 0x45, 0xce, 0x97, 0x00, 0x64, 0x00, 0x64,
		0x00, 0x01, 0x02, 0x00, 0x05, 0x68, 0x61, 0x6c,
		0x6f, 0x77, 0x01, 0x08, 0x00, 0x00, 0x00, 0x04,
		0x00, 0x00, 0x00, 0xfd, /* DS PARAM ELEMENT */0x03, 0x01, 0x01, };

	struct nrc_scan_info* scan = scan_init(cancel_cb);
	scan_start(scan);
	CU_ASSERT(scan_add(scan, 2412, 0, f, sizeof(f)) == 0);
	CU_ASSERT_FALSE(scan_add(scan, 2412, 0, f, sizeof(f)) == 0);
	scan_stop(scan);
	scan_deinit(scan);
}

static void test_scan_diff_ch_same_ap(void)
{
	uint8_t f[] = {
		0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0x02, 0x00, 0xeb, 0x1b, 0xe9, 0xcd, 0x02,
		0x00, 0xeb, 0x1b, 0xe9, 0xcd, 0x00, 0xb0, 0x00,
		0x00, 0x45, 0xce, 0x97, 0x00, 0x64, 0x00, 0x64,
		0x00, 0x01, 0x02, 0x00, 0x05, 0x68, 0x61, 0x6c,
		0x6f, 0x77, 0x01, 0x08, 0x00, 0x00, 0x00, 0x04,
		0x00, 0x00, 0x00, 0xfd, /* DS PARAM ELEMENT */0x03, 0x01, 0x01, };

	struct nrc_scan_info* scan = scan_init(cancel_cb);
	scan_start(scan);
	CU_ASSERT(scan_add(scan, 2412, 0, f, sizeof(f)) == 0);
	/* Chaged channel, expect probe req. is accepted */
	CU_ASSERT_FALSE(scan_add(scan, 2437, 0, f, sizeof(f)) == 0);
	scan_stop(scan);
	scan_deinit(scan);
}

int add_test_suite_wpas()
{
	CU_pSuite pSuite = NULL;
	pSuite = CU_add_suite("wpa_supplicant", init_wpas_scan, clean_wpas_scan);
	if (!pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
	if (
	!CU_add_test(pSuite, "Test: Scan running flag", test_scan_run)
	|| !CU_add_test(pSuite, "Test: DS Param Set element", test_ds_param)
	|| !CU_add_test(pSuite, "Test: Ignore presp frame from scanned AP #1\n",
			 test_scan_same_ch_same_ap)
	|| !CU_add_test(pSuite, "Test: Ignore presp frame from scanned AP #2\n",
			 test_scan_diff_ch_same_ap)
	) {
		CU_cleanup_registry();
		return CU_get_error();
	}
	return 0;
}
TEST_SUIT_DECLARE(add_test_suite_wpas);
