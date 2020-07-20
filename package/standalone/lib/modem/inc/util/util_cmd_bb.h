#ifndef UTIL_CMD_BB_H_
#define UTIL_CMD_BB_H_

#ifndef RELEASE
#if defined(STANDARD_11AH)
int util_str_to_bandwidth(const char *str);
bool util_str_onoff(const char *str);
#endif /* LMAC_CONFIG_11AH */
#endif /* RELEASE */

#endif /* UTIL_CMD_BB_H_ */
