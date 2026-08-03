@echo off
setlocal EnableExtensions

set ROOT=%~dp0
set MOBILE=%ROOT%mobile
set OUT=%ROOT%build\mobile
set SDK=
set JAVA_HOME_SET=

if defined ANDROID_HOME if exist "%ANDROID_HOME%\platform-tools\adb.exe" set SDK=%ANDROID_HOME%
if not defined SDK if defined ANDROID_SDK_ROOT if exist "%ANDROID_SDK_ROOT%\platform-tools\adb.exe" set SDK=%ANDROID_SDK_ROOT%
if not defined SDK if exist "%LOCALAPPDATA%\Android\Sdk\platform-tools\adb.exe" set SDK=%LOCALAPPDATA%\Android\Sdk

if not defined SDK (
  echo Android SDK not found. Install via Android Studio SDK Manager.
  exit /b 1
)

if exist "%ROOT%tools\jdk17\bin\java.exe" (
  set "JAVA_HOME=%ROOT%tools\jdk17"
  set JAVA_HOME_SET=1
)
if not defined JAVA_HOME_SET if exist "C:\Program Files\Android\Android Studio\jbr\bin\java.exe" (
  set "JAVA_HOME=C:\Program Files\Android\Android Studio\jbr"
  set JAVA_HOME_SET=1
)
if not defined JAVA_HOME_SET if defined JAVA_HOME if exist "%JAVA_HOME%\bin\java.exe" set JAVA_HOME_SET=1
if not defined JAVA_HOME_SET (
  echo Java not found. Run tools\bootstrap or install JDK 17 / Android Studio.
  exit /b 1
)

echo Using SDK: %SDK%
echo Using JAVA_HOME: %JAVA_HOME%

if not exist "%OUT%" mkdir "%OUT%"

REM Ensure a standard platform path AGP expects (android-36).
if not exist "%SDK%\platforms\android-36\android.jar" (
  if exist "%SDK%\platforms\android-37.0\android.jar" (
    echo Linking platforms\android-36 -^> android-37.0 for AGP...
    mklink /J "%SDK%\platforms\android-36" "%SDK%\platforms\android-37.0" >nul 2>&1
  )
)

REM Escape path for local.properties (Gradle wants \\ on Windows)
set SDK_PROP=%SDK:\=\\%
> "%MOBILE%\local.properties" (
  echo sdk.dir=%SDK_PROP%
)

pushd "%MOBILE%"
if not exist "gradlew.bat" (
  echo Gradle wrapper missing. Bootstrapping...
  call "%~dp0tools\bootstrap_gradle_wrapper.bat"
  if errorlevel 1 (
    popd
    exit /b 1
  )
)

call gradlew.bat --no-daemon :app:assembleDebug
if errorlevel 1 (
  echo Mobile build failed.
  popd
  exit /b 1
)
popd

set APK_SRC=%MOBILE%\app\build\outputs\apk\debug\app-debug.apk
if not exist "%APK_SRC%" (
  echo APK not found at %APK_SRC%
  exit /b 1
)

copy /Y "%APK_SRC%" "%OUT%\nuclear_fusion-debug.apk" >nul
echo Built %OUT%\nuclear_fusion-debug.apk
endlocal
