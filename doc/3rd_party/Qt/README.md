# Qt Patches

Qt open source version 5.15.8 is built from source, as offline installers are no longer available for open source users. Two security vulnerabilities were discovered in 5.15.8:

- https://nvd.nist.gov/vuln/detail/CVE-2022-25255
- https://nvd.nist.gov/vuln/detail/CVE-2022-25634

In order to address these security vulnerabilities, the relevant patch files are included with the SALSA source code, and are applied to Qt at build time. Additionally, a minor ARL-generated patch file is also provided in order to successfully apply the security patches.

# Linux: Applying the Patches

1) Unzip the source code 
    ```
    unzip ~/qt-5.15.8-ARL-patched/qt-everywhere-opensource-src-5.15.8.zip -d ~/
    ```

1) Copy the patches to the qt source directory
    ```
    cp $HOME/qt-5.15.8-ARL-patched/*.diff ~/qt-everywhere-src-5.15.8
    ```
1) Change directories to the qt source code and apply the patches
    ```
    cd ~/qt-everywhere-src-5.15.8/qtbase
    patch -p1 -i ../ARL-2023-patch.diff
    patch -p1 -i ../CVE-2022-25255-5.15.diff
    patch -p1 -i ../CVE-2022-25643-5.15.diff
    ```
1) Now the user can build Qt using the normal build instructions

# Windows: Applying the Patches

1) Git Bash is the recommended prompt for Windows to match commands across platforms

1) Unzip the source code to a desired destination
    ```
    unzip $USERPROFILE/qt-5.15.8-ARL-patched/qt-everywhere-opensource-src-5.15.8.zip -d C:/Qt/
    ```

1) Copy the patches to the qt source directory
    ```
    cp "$USERPROFILE/qt-5.15.8-ARL-patched"*.diff C:/Qt/qt-everywhere-src-5.15.8
    ```
1) Change directories to the qt source code and apply the patches
    ```
    cd C:/Qt/qt-everywhere-src-5.15.8/qtbase
    patch -p1 -i ../ARL-2023-patch.diff
    patch -p1 -i ../CVE-2022-25255-5.15.diff
    patch -p1 -i ../CVE-2022-25643-5.15.diff
    ```
1) Now the user can build Qt using the normal build instructions
