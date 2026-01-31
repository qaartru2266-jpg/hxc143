#ifndef __GPS_INTERFACE_H__
#define __GPS_INTERFACE_H__

#define GPS_BUF_SIZE  1024

void gps_init(void);
void gps_power_set(bool enable);

unsigned int GpsReadData(unsigned char *r_data);

unsigned int GpsSendData(const char* logName, const char* data, const int len);

#endif
