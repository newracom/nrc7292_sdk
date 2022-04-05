# NRC7292 Standalone SDK Package

## Notice
### Release roadmap
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
- [NRC7292_Standalone_SDK_v1.3.4_rev06](https://github.com/newracom/nrc7292_sdk/releases/tag/v1.3.4_rev06)

### Release package contents
- standalone: NRC7292 standalone SDK package for global regulatory domains
- standalone_kr_mic: NRC7292 standalone SDK package for South Korea MIC frequency regulation
- standalone_kr_usn: NRC7292 standalone SDK package for South Korea USN frequency regulation

### Apply a specific package
If you want to apply a specific package to your exiting package directory, you can choose one of following methods.
#### Method #1: replace the whole package
Let's assume that you have v1.3.0 and want to apply v1.3.1 to your package location.
1. Download a specific package you want.
   * If it is official release version 1.3.1
     1. Go to https://github.com/newracom/nrc7292_sdk/releases and choose the release package you want.
     ![sdk_release](/images/sdk_release.png)
     1. Downalod the compressed package: zip version or tar.gz version
     1. Check the filename: nrc7292_sdk-1.3.1.zip or nrc7292_sdk-1.3.1.tar.gz
   * If it is the latest pacakge
     1. Click "Code" and then click "Download ZIP"
     ![sdk_latest](/images/sdk_latest.png)
     1. Check the filename: nrc7292_sdk-master.zip
1. Replace your old package directory with the one you downloaded.
#### Method #2: pull down a branch
This needs your cloned repository and the internet connection.
1. Move to the repository directory
   ```
   cd repo/nrc7292_sdk
   ```
1. Pull down a branch
   * If you want to pull down the latest one from master branch
   ```
   git pull
   ```
   * If you want to change into a specific branch by using tag version
   ```
   git tag -l
   git checkout v1.3.1
   ```

## NRC7292 Standalone SDK User Guide
### Get NRC7292 Standalone SDK Package
NRC7292 Standalone SDK package is provided in this repository. Please refer to the following git command to get it.
```
cd ~/
git clone https://github.com/newracom/nrc7292_sdk.git
```

### Get the detailed user guide
Please refer to [UG-7292-004-Standalone SDK.pdf](https://github.com/newracom/nrc7292_sdk/blob/master/package/standalone/doc/UG-7292-004-Standalone%20SDK.pdf) in doc directory. 
