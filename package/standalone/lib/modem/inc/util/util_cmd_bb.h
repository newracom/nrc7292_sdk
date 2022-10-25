#ifndef UTIL_CMD_BB_H_
#define UTIL_CMD_BB_H_

#ifdef INCLUDE_USE_CLI
#if defined(STANDARD_11AH)
int util_str_to_bandwidth(const char *str);
bool util_str_onoff(const char *str);
#endif //#if defined(STANDARD_11AH)
#endif //#ifdef INCLUDE_USE_CLI

#endif /* UTIL_CMD_BB_H_ */
