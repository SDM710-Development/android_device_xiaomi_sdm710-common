# Copyright (C) 2009 The Android Open Source Project
# Copyright (c) 2011, The Linux Foundation. All rights reserved.
# Copyright (C) 2017-2020 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import common
import re

fw_files = {}

# Parse filesmap file containing firmware residing places
def LoadFilesMap(zip, name="RADIO/filesmap"):
  try:
    data = zip.read(name)
  except KeyError:
    print "Warning: could not find %s in %s." % (name, zip)
    data = ""
  d = {}
  for line in data.split("\n"):
    line = line.strip()
    if not line or line.startswith("#"):
      continue
    pieces = line.split()
    if not (len(pieces) == 2):
      raise ValueError("malformed filesmap line: \"%s\"" % (line,))
    d[pieces[0]] = pieces[1]
  return d

# Read firmware images from target files zip
def GetFwFiles(z):
  out = {}
  for info in z.infolist():
    f = info.filename
    if f.startswith("RADIO/") and (f.__len__() > len("RADIO/")):
      fn = f[6:]
      if fn.startswith("filesmap"):
        continue
      data = z.read(f)
      out[fn] = common.File(f, data)
  return out

# Prepare firmware files and verify them
def PrepareFwFiles(info, zip_file):
  print "Loading firmware files map..."
  filesmap = LoadFilesMap(zip_file)
  if filesmap == {}:
    print "warning radio-update: no or invalid filesmap file found"
    return False

  print "Loading firmware files..."
  tgt_files = GetFwFiles(zip_file)
  if tgt_files == {}:
    print "warning radio-update: no firmware images in input target_files"
    return False

  global fw_files

  print "Preparing firmware files..."
  for fn in tgt_files:
    dest = filesmap[fn]
    if dest is None:
      continue

    tf = tgt_files[fn]
    f = "firmware-update/" + fn
    common.ZipWriteStr(info.output_zip, f, tf.data)
    fw_files[f] = dest

  return True

def IncrementalOTA_VerifyEnd(info):
  return PrepareFwFiles(info, info.target_zip)

def FullOTA_InstallEnd(info):
  OTA_InstallEnd(info)
  return

def IncrementalOTA_InstallEnd(info):
  OTA_InstallEnd(info)
  return

def FullOTA_Assertions(info):
  AddTrustZoneAssertion(info, info.input_zip)
  return

def IncrementalOTA_Assertions(info):
  AddTrustZoneAssertion(info, info.target_zip)
  return

def AddImage(info, basename, dest):
  path = "IMAGES/" + basename
  if path not in info.input_zip.namelist():
    return

  data = info.input_zip.read(path)
  common.ZipWriteStr(info.output_zip, basename, data)
  info.script.Print("Patching {} image unconditionally...".format(dest.split('/')[-1]))
  info.script.AppendExtra('package_extract_file("%s", "%s");' % (basename, dest))

def InstallFwImages(script, files):
  for f in files:
    dest = files[f]
    script.Print("Patching {} firmware unconditionally...".format(dest.split('/')[-1]))
    script.AppendExtra('package_extract_file("%s", "%s");' % (f, dest))
  return

def OTA_InstallEnd(info):
  AddImage(info, "dtbo.img", "/dev/block/bootdevice/by-name/dtbo")
  AddImage(info, "vbmeta.img", "/dev/block/bootdevice/by-name/vbmeta")
  if PrepareFwFiles(info, info.input_zip):
    InstallFwImages(info.script, fw_files)
  return

def AddTrustZoneAssertion(info, input_zip):
  android_info = info.input_zip.read("OTA/android-info.txt")
  m = re.search(r'require\s+version-trustzone\s*=\s*(\S+)', android_info)
  f = re.search(r'require\s+version-miui\s*=\s*(.+)', android_info)
  if m and f:
    versions_tz = m.group(1).split('|')
    version_miui = f.group(1).rstrip()
    if len(versions_tz) and '*' not in versions_tz:
      cmd = 'assert(xiaomi.verify_trustzone(' + ','.join(['"%s"' % tz for tz in versions_tz]) + ') == "1" || abort("ERROR: This package requires firmware from MIUI ' + version_miui + ' build or newer. Please upgrade firmware and retry!"););'
      info.script.AppendExtra(cmd)
  return
