#!/usr/bin/env python3
import argparse, glob, re, subprocess, datetime, time, os
#from shutil import which
from os import path
from enum import Enum

#versionHeaderFile = "{project_dir}/component/common/include/FirmwareVersion.h"
versionHeaderFile = "{project_dir}\\components\\common\\include\\FirmwareVersion.h"
#versionFile = "{project_dir}/firmware_version"
versionFile = "{project_dir}\\firmware_version"
#gitIsExist = which("git") is not None
gitIsExist = True

versionFileTemplate = '''#pragma once

#define MODEL "ATOM32"

#define FIRMWARE_VER_MAJOR {fw_major}
#define FIRMWARE_VER_MINOR {fw_minor}
#define FIRMWARE_VER_PATCH {fw_patch}
#define FIRMWARE_VERSION "{fw_major}.{fw_minor}.{fw_patch}"
#define KERNEL_VERSION "{kernel}"
#define HARDWARE_ARCH "esp32"
#define HARDWARE_VERSION "{board}"

#define BUILD_TIME "{build_time}"
#define BUILD_BUILDER "{build_builder}"
#define BUILD_BRANCH "{build_branch}"
#define BUILD_COMMIT "{build_commit}"
'''

class Board(Enum):
    devboard = 'devboard'
    rev1 = 'rev1'
    plug = 'plug'
    oldrev = 'old_rev'

    def __str__(self):
        return self.value

def executeWhoami():
    return "user:" + os.environ.get('USERNAME')
    #return subprocess.check_output('echo "$(whoami)@$(hostname)"', shell=True).decode('utf-8').rstrip('\n')

def executeGit(params, default="unknown", execDir=None):
    if gitIsExist:
        return subprocess.check_output("git {0}".format(params), shell=True, cwd=execDir).decode('utf-8').rstrip('\n')
    return default

def writeVersionInfo(major, minor, build, board, wifiSsid, wifiPsw, localtime=time.localtime()):
    projectDir = os.path.dirname(os.path.realpath(__file__))

    revision = executeGit('rev-parse HEAD')
    branch = executeGit('rev-parse --abbrev-ref HEAD')
    firmware = executeGit('describe --first-parent --always')
    # Try parse version from git describe
    if major == 0 and minor == 0 and build == 0:
        a = firmware.split('-')
        if len(a) > 3 and a[2].isdigit():
            build = a[2]
        elif len(a) > 1:
            if a[1].isdigit():
                build = a[1]
        b = a[0].split('.')
        if b[0].isdigit():
            major = b[0]
        if len(b) > 1 and b[1].isdigit():
            minor = b[1]

    kernel = executeGit('describe --first-parent --always', execDir=os.environ['IDF_PATH'])
    builder = executeWhoami()
    buildTime = time.strftime("%Y-%m-%dT%H:%M:%S%z", localtime)
#    if len(buildTime) > 19:
#        buildTime = buildTime[:22] + ':' + buildTime[22:]

    fullVersionHeaderFilePath = versionHeaderFile.format(project_dir=projectDir)

    versionFileHandle = open(fullVersionHeaderFilePath, "w")
    versionFileHandle.writelines(versionFileTemplate.format(fw_major=major, fw_minor=minor, fw_patch=build, kernel=kernel, board=board, build_time=buildTime, build_builder=builder, build_branch=branch, build_commit=revision))
    if wifiSsid is not None:
        versionFileHandle.writelines( "#define DEFAULT_WIFI_SSID \"{ssid}\"\n".format(ssid=wifiSsid) );
        if wifiPsw is None:
            versionFileHandle.writelines( "#define DEFAULT_WIFI_PASSWORD \"\"\n" );
        else:
            versionFileHandle.writelines( "#define DEFAULT_WIFI_PASSWORD \"{password}\"\n".format(password=wifiPsw) );
    versionFileHandle.close()

    fullVersionFilePath = versionFile.format(project_dir=projectDir)
    versionFileHandle = open(fullVersionFilePath, "w")
    versionFileHandle.write("{0}.{1}.{2}".format(major, minor, build))
    versionFileHandle.close()

    return major, minor, build

def main():
    if not gitIsExist:
        print( "Warning: git tool is not exist!" )

    localtime = time.localtime()

    arguments_parser = argparse.ArgumentParser(description = "Generating version information.")
    arguments_parser.add_argument("--major", help="Major version", type=int, default=0, required = False)
    arguments_parser.add_argument("--minor", help="Minor version", type=int, default=0, required = False)
    arguments_parser.add_argument("--build", help="Build version", type=int, default=0, required = False)
    arguments_parser.add_argument("--board", help="Board type", type=Board, choices=list(Board), default=Board.devboard, required = False)
    arguments_parser.add_argument("--wifi-ssid", help="WiFi network name(Only for debugging)", type=str, required = False)
    arguments_parser.add_argument("--wifi-psw", help="WiFi network password(Only for debugging)", type=str, required = False)
    args = arguments_parser.parse_args()

    writeVersionInfo(args.major, args.minor, args.build, args.board, args.wifi_ssid, args.wifi_psw, localtime)

if __name__ == '__main__':
    exit(main())
