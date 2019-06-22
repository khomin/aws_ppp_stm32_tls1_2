/*
 * lwip_net.h
 *
 *  Created on: Jun 22, 2019
 *      Author: khomin
 */

#ifndef AWS_INC_LWIP_NET_H_
#define AWS_INC_LWIP_NET_H_

int net_if_init(void * if_ctxt);
int net_if_deinit(void * if_ctxt);
int net_if_reinit(void * if_ctxt);

#endif /* AWS_INC_LWIP_NET_H_ */
