/** CP2615 I/O Protocol implementation
 *  (c) 2021, Bence Csókás <bence98@sch.bme.hu>
 *  Licensed under GPLv2
 *  Source: https://www.silabs.com/documents/public/application-notes/an1139-cp2615-io-protocol.pdf
 */

#ifdef USER_MODE
#include <string.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#else //! USER_MODE
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#endif //USER_MODE

#include "cp2615_iop.h"

int cp2615_init_iop_msg(struct cp2615_iop_msg *ret, enum cp2615_iop_msg_type msg, const void *data, size_t data_len)
{
	if (data_len > MAX_IOP_PAYLOAD_SIZE)
		return -EFBIG;

	if (ret) {
		ret->preamble = 0x2A2A;
		ret->length = htons(data_len+6);
		ret->msg = htons(msg);
		if(data && data_len)
			memcpy(&ret->data, data, data_len);
        return 0;
	} else {
        return -EINVAL;
	}
}

int cp2615_init_i2c_msg(struct cp2615_iop_msg *ret, const struct cp2615_i2c_transfer *data)
{
    return cp2615_init_iop_msg(ret, iop_DoI2cTransfer, data, 4 + data->write_len);
}

int cp2615_check_status(enum cp2615_i2c_status status)
{
	switch (status) {
	case CP2615_SUCCESS:
			return 0;
	case CP2615_BUS_ERROR:
		return -ECOMM;
	case CP2615_BUS_BUSY:
		return -EAGAIN;
	case CP2615_TIMEOUT:
		return -ETIMEDOUT;
	case CP2615_INVALID_PARAM:
		return -EINVAL;
	case CP2615_CFG_LOCKED:
		return -EPERM;
	}
	/* Unknown error code */
	return -EPROTO;
}
