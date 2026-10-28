#include "sensors.h"
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <string.h>

typedef struct { char major, minor, build, reserved[1]; UInt16 release; } vers_t;
typedef struct { UInt16 version, length; UInt32 cpuPLimit, gpuPLimit, memPLimit; } plimit_t;
typedef struct { UInt32 dataSize; UInt32 dataType; char dataAttributes; } keyinfo_t;
typedef struct {
  UInt32 key; vers_t vers; plimit_t pLimitData; keyinfo_t keyInfo;
  char result, status, data8; UInt32 data32; unsigned char bytes[32];
} smcdata_t;

static io_connect_t conn;

static UInt32 fourcc(const char *s) { return (s[0]<<24)|(s[1]<<16)|(s[2]<<8)|s[3]; }

int smc_open(void) {
  io_service_t s = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("AppleSMC"));
  if (!s) return -1;
  kern_return_t r = IOServiceOpen(s, mach_task_self(), 0, &conn);
  IOObjectRelease(s);
  return r == kIOReturnSuccess ? 0 : -1;
}

static double smc_num(const char *key) {
  if (!conn) return -1;
  smcdata_t in = {0}, res = {0}; size_t sz = sizeof(res);
  in.key = fourcc(key); in.data8 = 9;  // read key info
  if (IOConnectCallStructMethod(conn, 2, &in, sizeof(in), &res, &sz) != kIOReturnSuccess || res.result) return -1;
  UInt32 type = res.keyInfo.dataType;
  in.keyInfo = res.keyInfo; in.data8 = 5;  // read bytes
  memset(&res, 0, sizeof(res)); sz = sizeof(res);
  if (IOConnectCallStructMethod(conn, 2, &in, sizeof(in), &res, &sz) != kIOReturnSuccess || res.result) return -1;
  unsigned char *b = res.bytes;
  if (type == fourcc("flt ")) { float f; memcpy(&f, b, 4); return f; }
  if (type == fourcc("fpe2")) return ((b[0]<<8)|b[1]) / 4.0;
  if (type == fourcc("ui8 ")) return b[0];
  if (type == fourcc("ui16")) return (b[0]<<8)|b[1];
  return -1;
}

int fan_count(void) { return (int)smc_num("FNum"); }

double fan_value(int fan, const char *suffix) {
  char key[5];
  snprintf(key, sizeof(key), "F%d%.2s", fan, suffix);
  return smc_num(key);
}

typedef struct __IOHIDEventSystemClient *HIDClient;
typedef struct __IOHIDServiceClient *HIDService;
typedef struct __IOHIDEvent *HIDEvent;
HIDClient IOHIDEventSystemClientCreate(CFAllocatorRef);
int IOHIDEventSystemClientSetMatching(HIDClient, CFDictionaryRef);
CFArrayRef IOHIDEventSystemClientCopyServices(HIDClient);
HIDEvent IOHIDServiceClientCopyEvent(HIDService, int64_t, int32_t, int64_t);
double IOHIDEventGetFloatValue(HIDEvent, int32_t);
CFTypeRef IOHIDServiceClientCopyProperty(HIDService, CFStringRef);

static HIDClient hid;

double max_die_temp(void) {
  if (!hid) {
    int page = 0xff00, usage = 5;  // Apple vendor page, temperature sensors
    CFNumberRef p = CFNumberCreate(NULL, kCFNumberIntType, &page), u = CFNumberCreate(NULL, kCFNumberIntType, &usage);
    const void *keys[] = { CFSTR("PrimaryUsagePage"), CFSTR("PrimaryUsage") }, *vals[] = { p, u };
    CFDictionaryRef match = CFDictionaryCreate(NULL, keys, vals, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    hid = IOHIDEventSystemClientCreate(kCFAllocatorDefault);
    IOHIDEventSystemClientSetMatching(hid, match);
    CFRelease(match); CFRelease(p); CFRelease(u);
  }
  CFArrayRef svcs = IOHIDEventSystemClientCopyServices(hid);
  double mx = -1;
  for (CFIndex i = 0; svcs && i < CFArrayGetCount(svcs); i++) {
    HIDService sv = (HIDService)CFArrayGetValueAtIndex(svcs, i);
    CFStringRef name = IOHIDServiceClientCopyProperty(sv, CFSTR("Product"));
    char buf[64] = "";
    if (name) { CFStringGetCString(name, buf, sizeof(buf), kCFStringEncodingUTF8); CFRelease(name); }
    if (!strstr(buf, "tdie")) continue;  // skip "tcal" calibration constants etc.
    HIDEvent e = IOHIDServiceClientCopyEvent(sv, 15, 0, 0);  // kIOHIDEventTypeTemperature
    if (!e) continue;
    double v = IOHIDEventGetFloatValue(e, 15 << 16);
    if (v > 0 && v < 150 && v > mx) mx = v;
    CFRelease(e);
  }
  if (svcs) CFRelease(svcs);
  return mx;
}
