/*
 * (C) Copyright 2009
 * Vipin Kumar, ST Micoelectronics, vipin.kumar@st.com.
 *
 * (C) Copyright 2008-2014 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __DWC3_RK_UDC_H
#define __DWC3_RK_UDC_H
#define DWCERR
#undef DWCWARN
#undef DWCINFO
#undef DWCDEBUG

/* Some kind of debugging output... */
#ifdef DWCERR
#define DWC_ERR(fmt, args...)\
    printf("ERROR: [%s]: "fmt, __func__, ##args)
#else
#define DWC_ERR(fmt, args...) do {} while (0)
#endif

#ifdef DWCWARN
#define DWC_WARN(fmt, args...)\
    printf("WARNING: [%s]: "fmt, __func__, ##args)
#else
#define DWC_WARN(fmt, args...) do {} while (0)
#endif

#ifdef DWCINFO
#define DWC_PRINT(fmt, args...)\
    printf("INFO: [%s]: "fmt, __func__, ##args)
#else
#define DWC_PRINT(fmt, args...) do {} while (0)
#endif

#ifdef DWCDEBUG
#define DWC_DBG(fmt, args...)\
    printf("DEBUG: [%s]: %d:\n"fmt, __func__, __LINE__, ##args)
#else
#define DWC_DBG(fmt, args...) do {} while (0)
#endif


struct rk_dwc3_udc_instance{
	struct usb_device_descriptor *device_desc;
	void *config_desc;
	struct usb_interface_descriptor *interface_desc;
	struct usb_endpoint_descriptor  *endpoint_desc[2];
	struct usb_string_descriptor    *string_desc[6];
	void *rx_buffer[2];
	void *tx_buffer[2];
	__u32 rx_buffer_size;
	__u32 tx_buffer_size;
	__u8 bos_desc[8];
	__u32 whole_config_size;
	__u8 whole_config_desc[100];
	void (*device_event) (int nEvent);
	void (*rx_handler)(int status, uint32_t actual, void *buf);
	void (*tx_handler)(int status, uint32_t actual, void *buf);
	__u8 connected;
	__u8 suspended;
	__u8 rx_current_buffer;
	__u8 tx_current_buffer;
};
struct dwc3_giveback_data{
	void *buf;
	int status;
	uint32_t actual;
};


int RK_Dwc3ReadBulkEndpoint(uint32_t nLen, void *buffer);
int RK_Dwc3WriteBulkEndpoint(uint32_t nLen, void *buffer);
void rk_dwc3_startup(struct rk_dwc3_udc_instance *device);
int rk_dwc3_connect(void);
void rk_dwc3_disconnect(void);

uint32_t GetVbus(void);
void ClearVbus(void);

#endif /* __DWC3_RK_UDC_H */
