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
#else //! USER_MODE
#include <linux/string.h>
#include <linux/kernel.h>
#endif //USER_MODE

#include "cp2615_iop.h"

int init_IOP_msg(struct IOP_msg *ret, enum IOP_msg_type msg, const void *data, size_t data_len)
{
	if (data_len > MAX_IOP_PAYLOAD_SIZE)
		return -2;

	if (ret) {
		ret->preamble = 0x2A2A;
		ret->length = htons(data_len+6);
		ret->msg = htons(msg);
		if(data && data_len)
			memcpy(&ret->data, data, data_len);
        return 0;
	} else
        return -5;
}

int init_IOP_I2c_msg(struct IOP_msg *ret, const struct IOP_I2cTransfer *data)
{
    return init_IOP_msg(ret, iop_DoI2cTransfer, data, 4 + data->write_len);
}
