@echo off
setlocal EnableExtensions
set ROOT=%~dp0..
set MOBILE=%ROOT%\mobile
set GRADLE_VER=8.11.1
set ZIP=%TEMP%\gradle-%GRADLE_VER%-bin.zip
set DEST=%TEMP%\gradle-%GRADLE_VER%

if exist "%~dp0jdk17\bin\java.exe" (
  set "JAVA_HOME=%~dp0jdk17"
) else if exist "C:\Program Files\Android\Android Studio\jbr" (
  set "JAVA_HOME=C:\Program Files\Android\Android Studio\jbr"
)

echo Downloading Gradle %GRADLE_VER%...
powershell -NoProfile -Command ^
  "Invoke-WebRequest -Uri 'https://services.gradle.org/distributions/gradle-%GRADLE_VER%-bin.zip' -OutFile '%ZIP%'"
if errorlevel 1 exit /b 1

echo Extracting...
powershell -NoProfile -Command ^
  "if (Test-Path '%DEST%') { Remove-Item -Recurse -Force '%DEST%' }; Expand-Archive -Path '%ZIP%' -DestinationPath '%TEMP%' -Force"
if errorlevel 1 exit /b 1

pushd "%MOBILE%"
call "%DEST%\bin\gradle.bat" wrapper --gradle-version %GRADLE_VER%
set ERR=%ERRORLEVEL%
popd
exit /b %ERR%
