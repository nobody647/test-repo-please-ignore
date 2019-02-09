/*
 * CAN_messages.h
 *
 * Header file that will contain the structures for all of the different
 * CAN messages as well as the message ids.
 *
 * Not all of the messages will have their own distinct structure that is
 * needed so message ids will be tied to a structure in a many to one relation
 *
 *  Created on: Jan 22, 2019
 *      Author: gty112
 */

#ifndef CAN_MESSAGES_H_
#define CAN_MESSAGES_H_


//CAN message IDS
//Related to the NODE info
#define NODE_INFO_MSG_ID_OFFSET 0x6
#define NODE_INFO_MSG_BASE_ID 0x601
#define NODE_CELL1-2_MSG_BASE_ID 0x602
#define NODE_CELL3-4_MSG_BASE_ID 0x603
#define NODE_CELL5-6_MSG_BASE_ID 0x604
#define NODE_CELL7-8_MSG_BASE_ID 0x605
#define NODE_CELL9-10_MSG_BASE_ID 0x606
#define NODE_CELL11-12_MSG_BASE_ID 0x607

#define PACK_SOC_MSG_ID 0x6F4


#define PRECHARGE_STATUS_MSG_ID 0x6F7

#define MIN-MAX_CELL_VOLTAGE_MSG_ID 0x6F8


#define MIN-MAX_CELL_TEMP_MSG_ID 0x6F9

#define PACK_VOLTAGE_CURRENT_MSG_ID 0x6FA


#define PACK_STATUS_MSG_ID 0x6FB

#define FAN_STATUS_MSG_ID 0x6FC

#define EXTENDED_PACK_STATUS 0x6FD



#endif /* CAN_MESSAGES_H_ */
