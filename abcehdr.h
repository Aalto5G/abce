#ifndef _ABCE_HDR_H_
#define _ABCE_HDR_H_

#include <string.h>
#include <netinet/in.h>

static inline uint64_t abce_hdr_get64h(const void *buf)
{
  uint64_t res;
  memcpy(&res, buf, sizeof(res));
  return res;
}

static inline uint32_t abce_hdr_get32h(const void *buf)
{
  uint32_t res;
  memcpy(&res, buf, sizeof(res));
  return res;
}

static inline uint16_t abce_hdr_get16h(const void *buf)
{
  uint16_t res;
  memcpy(&res, buf, sizeof(res));
  return res;
}

static inline uint8_t abce_hdr_get8h(const void *buf)
{
  uint8_t res;
  memcpy(&res, buf, sizeof(res));
  return res;
}

static inline void abce_hdr_set64h(void *buf, uint64_t val)
{
  memcpy(buf, &val, sizeof(val));
}

static inline void abce_hdr_set32h(void *buf, uint32_t val)
{
  memcpy(buf, &val, sizeof(val));
}

static inline void abce_hdr_set16h(void *buf, uint16_t val)
{
  memcpy(buf, &val, sizeof(val));
}

static inline void abce_hdr_set8h(void *buf, uint8_t val)
{
  memcpy(buf, &val, sizeof(val));
}

static inline uint32_t abce_hdr_get32n(const void *buf)
{
  return ntohl(abce_hdr_get32h(buf));
}

static inline uint16_t abce_hdr_get16n(const void *buf)
{
  return ntohs(abce_hdr_get16h(buf));
}

static inline void abce_hdr_set32n(void *buf, uint32_t val)
{
  abce_hdr_set32h(buf, htonl(val));
}

static inline void abce_hdr_set16n(void *buf, uint16_t val)
{
  abce_hdr_set16h(buf, htons(val));
}

#endif
