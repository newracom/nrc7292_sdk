# NRC7292 Standalone SDK Package

## Notice
- Starting from version 1.4, kindly utilize the FirmwareFlashTool.exe located in the package/standalone/tools/external directory.
- Please do not use package/standalone/tools/external_tools/FirmwareFlashTool.exe of v1.3.4 rev08. Use other v1.3.4 revision's one instead.

### Release roadmap
- v1.4         (2023.03.31)
- v1.3.4_rev09 (2022.10.24)
- v1.3.4_rev08 (2022.08.09)
- v1.3.4_rev07 (2022.07.19)
- v1.3.4_rev06 (2022.04.05)
- v1.3.4_rev05 (2022.03.16)
- v1.3.4_rev04 (2022.03.02)
- v1.3.4_rev03 (2021.12.08)
- v1.3.4_rev02 (2021.12.07)
- v1.3.4_rev01 (2021.11.04)
- v1.3.4 (2021.10.22)
- v1.3.3 (skip to sync the version number with nrc7292_sw_pkg)
- v1.3.2 (2020.09.16)
- v1.3.1 (2020.08.05)
- v1.3.0 (2020.07.19)

### Latest release
- [NRC7292_Standalone_SDK_v1.4](https://github.com/newracom/nrc7292_sdk/releases/tag/v1.4)

### Release package contents
- standalone: NRC7292 standalone SDK package for global regulatory domains

### Apply a specific package
If you want to apply a specific package to your exiting package directory, you can choose one of following methods.
#### Method #1: replace the whole package
Let's assume that you have v1.3.0 and want to apply v1.3.1 to your package location.
1. Download a specific package you want.
   * If it is official release version 1.3.1
     1. Go to https://github.com/newracom/nrc7292_sdk/releases and choose the release package you want.
     ![sdk_release](/images/sdk_release.png)
     1. Download the compressed package: zip version or tar.gz version
     1. Check the filename: nrc7292_sdk-1.3.1.zip or nrc7292_sdk-1.3.1.tar.gz
   * If it is the latest package
     1. Click "Code" and then click "Download ZIP"
     ![sdk_latest](/images/sdk_latest.png)
     1. Check the filename: nrc7292_sdk-master.zip
1. Replace your old package directory with the one you downloaded.
#### Method #2: pull down a branch
This method requires a cloned repository and a connection to the internet.
1. Go to the repository directory
   ```
   cd repo/nrc7292_sdk
   ```
1. Pull updates
   * If you want to pull the latest updates from master branch
   ```
   git pull
   ```
   * If you want to use a specific branch checkout it by using tag version
   ```
   git tag -l
   git checkout v1.3.1
   ```

## NRC7292 Standalone SDK User Guide
### Get NRC7292 Standalone SDK Package
NRC7292 Standalone SDK package is provided in this repository. Please use the following git command to get it.
```
cd ~/
git clone https://github.com/newracom/nrc7292_sdk.git
```

### Get the detailed user guide
Please refer to [UG-7292-004-Standalone SDK.pdf](https://github.com/newracom/nrc7292_sdk/blob/master/package/standalone/doc/UG-7292-004-Standalone%20SDK.pdf) in doc directory. 
