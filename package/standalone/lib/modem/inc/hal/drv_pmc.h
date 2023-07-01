#ifndef __DRV_PMC_H__
#define __DRV_PMC_H__

#define SEC_TO_HZ(s) s * 32768
#define MS_TO_HZ(m) (m * 32768) / 1000

enum {
    PMC_REG_PWR_SW_MASK             = PMC_BASE_ADDR + 0x20,
    PMC_REG_PWR_SW_MASK_CORE        = PMC_REG_PWR_SW_MASK,
    PMC_REG_PWR_SW_MASK_CORE_SHIFT  = 8,
    PMC_REG_PWR_SW_MASK_CORE_MASK   = 0x00000100,

    PMC_REG_ETC_CTRL                = PMC_BASE_ADDR + 0x30,
    PMC_REG_ETC_CTRL_RF_OFF         = PMC_REG_ETC_CTRL,
    PMC_REG_ETC_CTRL_RF_OFF_SHIFT   = 0,
    PMC_REG_ETC_CTRL_RF_OFF_MASK    = 0x00000001
};

enum {
    PMC_X32K = 0,
    PMC_X32M,
    PMC_PWR,
};

enum {
    PMC_PWR_WAIT = 0,
    PMC_PG_SIGNAL
};

enum {
    PMC_PMU_CTRL = BIT0,
    PMC_LDO_CTRL = BIT1,
    PMC_CORE     = BIT8,
    PMC_MODEM    = BIT9,
    PMC_MEM      = BIT10,
    PMC_RF       = BIT11,
    PMC_MEMPD    = BIT16,
};

enum {
    PMC_MODEMSLEEP = 0,
    PMC_SLEEP0,
    PMC_SLEEP1,
    PMC_SLEEP2,
    PMC_DEEPSLEEP0,
    PMC_DEEPSLEEP1,
};


enum {
	PMC_EXTINT0 = BIT0,
	PMC_EXTINT1 = BIT1,
#if defined(NRC7394)
	PMC_EXTINT2 = BIT2,
	PMC_EXTINT3 = BIT3,
	PMC_RTCINT  = BIT4,
	PMC_CSPIINT = BIT5,
#else
	PMC_RTCINT  = BIT2,
	PMC_CSPIINT = BIT3,
#endif
};

bool drv_pmc_init();
void drv_pmc_deinit();

void drv_pmc_reset();
void drv_pmc_irq(bool enable);
void drv_pmc_enable(bool enable);
void drv_pmc_int0_level(uint32_t level);
void drv_pmc_int1_level(uint32_t level);
#if defined(NRC7394)
void drv_pmc_int2_level(uint32_t level);
void drv_pmc_int3_level(uint32_t level);
#endif
void drv_pmc_wakeup_source_mask(uint32_t source);
void drv_pmc_opcode(uint32_t code);
void drv_pmc_wait(uint32_t id, uint32_t value);
void drv_pmc_wait_method(uint32_t sel);

uint32_t drv_pmc_get_int_status(void);
bool drv_pmc_busy(void);
uint32_t drv_pmc_current_status(void);
void drv_pmc_set_int_status(uint32_t status);
uint32_t drv_pmc_history(void);
uint8_t drv_pmc_get_wakeup_reason(uint32_t status);

void drv_pmc_pwr_sw_mask(uint32_t pwr, bool set);
void drv_pmc_pwr_sw_manual_en(uint32_t pwr, bool set);
void drv_pmc_pwr_sw_manual(uint32_t pwr, bool set);
void drv_pmc_iso_manual(uint32_t iso, bool set);

void drv_pmc_pwr_xtal32_off(bool enable);
void drv_pmc_pwr_lpo16m_off(bool enable);
void drv_pmc_pwr_alarm_enable(bool enable);
void drv_pmc_pwr_irq_enable(bool enable);
void drv_pmc_pwr_alarm_cnt_init(bool enable);

void drv_pmc_set_pwr_alarm_cnt(uint32_t cnt);
uint32_t drv_pmc_get_pwr_alarm_cnt(void);

const char *drv_scfg_get_boot_reason_str(uint8_t reason);
void drv_scfg_store_boot_reason();
uint8_t drv_get_boot_reason();
void drv_set_boot_reason(uint32_t br);
uint8_t drv_scfg_get_boot_reason();
void drv_scfg_clr_boot_reason();

void drv_pmc_sleep(int mode, uint64_t sleep_duration_ms);
//void drv_pmc_enter_sleep0(uint32_t after_ms);
void drv_pmc_execute_deepsleep(uint64_t after_ms);
void drv_pmc_execute_modemsleep(uint64_t after_ms);
void remap_and_reset(int remap, int others);
//void drv_pmc_test(int mode, uint64_t duration_sec);

void drv_pmc_rf_power(bool power_on);
void drv_pmc_core_power(bool power_on);
void drv_pmc_wakeup_modemsleep();

void drv_pmc_show();

#endif //__DRV_PMC_H__
