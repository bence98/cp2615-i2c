/** CP2615 I/O Protocol implementation
 *  (c) 2021, Bence Csókás <bence98@sch.bme.hu>
 *  Licensed under GPLv2
 *  Source: https://www.silabs.com/documents/public/application-notes/an1139-cp2615-io-protocol.pdf
 */

#include <string.h>
#include <arpa/inet.h>
#include "cp2615_iop.h"

int init_IOP_msg(struct IOP_msg *ret, enum IOP_msg_type msg, const void *data, size_t data_len)
{
	if (data_len > 64-6)
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
