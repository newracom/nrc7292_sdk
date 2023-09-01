#ifndef UTIL_VERSION_H
#define UTIL_VERSION_H

typedef struct{
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
}version_t;

void initVersion();
version_t* getCurrentFWVersion();
version_t* getCurrentAppVersion();
char* getCurrentAppName();
bool setCurrentAppName(char* appname);
void setCurrentFWVersion(version_t* version);
void setCurrentAppVersion(version_t* version);
bool isCurrentFWVersionLower(version_t* parsedVersion);
bool isCurrentAppVersionLower(version_t* parsedVersion);
bool isAppNameMatched(char* appname);
bool isBootLoader(version_t* parsedVersion);
uint32_t getChipID();
#endif /* UTIL_VERSION_H */
