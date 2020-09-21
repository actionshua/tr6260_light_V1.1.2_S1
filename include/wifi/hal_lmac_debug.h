#ifndef HAL_LMAC_DEBUG_H
#define HAL_LMAC_DEBUG_H
//#define HAL_VER_HTOL
void hal_lmac_report();
void lmac_check_txrx_timer_cb(TimerHandle_t xTimer);
int32_t lmac_check_txrx_timer_init(uint32_t interval);
int reset_mac(void);
#endif //HAL_LMAC_DEBUG_H
