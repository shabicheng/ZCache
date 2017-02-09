#ifdef _WIN32
#define EvoTCPOperatorLibAPI __declspec(dllexport)
#else
#define EvoTCPOperatorLibAPI
#endif

#include "../Include/EvoTCPCommon.h"



TCPWriteTaskList* ConstructTCPWriteTaskList()
{
	TCPWriteTaskList* tmplist = (TCPWriteTaskList*)malloc(sizeof(TCPWriteTaskList));
	tmplist->count = 0;
	tmplist->size = 0;
	tmplist->task_head = tmplist->task_tail = NULL;
	return tmplist;
}

void ReleaseTCPTaskList(TCPWriteTaskList* list)
{
	while(list->task_head!=NULL)
	{
		list->task_tail = list->task_head->next;
		free(list->task_head->buf);
		free(list->task_head);
		list->task_head = list->task_tail;
	}
	free(list);
}


int InsertTCPWriteTaskToList(TCPWriteTaskList* list , const char* buf , u_int32_t buflength)
{
	int noMerge = 1;
	int bigData = 0;
	TCPNetworkTask* tmpnode;
	if(list->count == MAX_TCP_WRITE_TASKLIST_LENGTH)
		return ERR_TCP_WRITE_TASKLIST_FULL;

	if(list->task_head == NULL)
	{
		tmpnode = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
		tmpnode->buf = NULL;
		tmpnode->buflength = 0;
		tmpnode->eleNum = 0;
		tmpnode->next = NULL;
		if(tmpnode == 0)
			return ERR_TCP_WRITE_TASKLIST_NOMEM;

		//每一块小于MSG_ENCODE_BUF_LENGTH的数据都需要加一个长度
		//合包之后，每一个子报文前都是自身长度
		if((buflength + SIZEOF_DATASEGFLAG + SIZEOF_MSGHEADER) > MSG_ENCODE_BUF_LENGTH)
		{
			//如果是大包数据则不需要附带自身长度，只需要将长度放入消息头
			//避免发送Header这样的短报文，因此每一包数据均在前预留Header的空间
			tmpnode->buf = (char*)malloc(buflength+SIZEOF_MSGHEADER);
			bigData = 1;
		}
		else
		{
			tmpnode->buf = (char*)malloc(MSG_ENCODE_BUF_LENGTH); 
		}
		if(tmpnode->buf == NULL)
		{
			free(tmpnode);
			return ERR_TCP_WRITE_TASKLIST_NOMEM;
		}
		if(bigData)
		{
			//首先构造消息头
			EvoTCPMessageHeader msgHeader;
			msgHeader.type = EVO_TCP_MSG_DATA;
			msgHeader.flags = 0;
			msgHeader.octetsToNextHeader = buflength;
			msgHeader.extFlags = 1;
			//编码消息头
			MapMsgHeader(tmpnode->buf , msgHeader);
			memcpy((void*)(tmpnode->buf+SIZEOF_MSGHEADER) , (void*)buf , buflength);
			tmpnode->buflength = buflength+SIZEOF_MSGHEADER;
		}
		else //小包数据，只需要预留Header的空间，最后编码
		{
			if(tmpnode->buflength == 0) //第一个小包数据
			{
					tmpnode->buflength  = SIZEOF_MSGHEADER; //预留Header空间	
			}
			//编码分块长度
			memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)&buflength , SIZEOF_DATASEGFLAG);
			tmpnode->buflength += SIZEOF_DATASEGFLAG;
			memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)buf , buflength);
			tmpnode->buflength += buflength;
		}
		//需要注意，如果不是大数据，则最后一定要编码Header
		tmpnode->type = EVO_TCP_MSG_DATA;
		tmpnode->eleNum = 1; //初始状态下一包数据
		tmpnode->next = NULL;
		list->task_head = list->task_tail = tmpnode;
		//printf("插入第一个数据包%d\n" , tmpnode->buflength);
	}
	//查看第一个任务是否为心跳报文，如果是，则可以删除
	else if(list->task_head->next == NULL)
	{
		if(list->task_head->type == EVO_TCP_MSG_HEARTBEAT)
		{
			//删除该任务
			free(list->task_head);
			list->task_head = list->task_tail = NULL;
			noMerge = 0; //这种情况下相当于该数据报文与心跳报文合并
			list->size-=SIZEOF_MSGHEADER;

			tmpnode = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
			if(tmpnode == 0)
				return ERR_TCP_WRITE_TASKLIST_NOMEM;
			tmpnode->buflength = 0;
			tmpnode->buf = NULL;
			tmpnode->flags = 0;
			tmpnode->next = NULL;
			tmpnode->type = EVO_TCP_MSG_DATA;

			//开始编码数据
			//每一块小于MSG_ENCODE_BUF_LENGTH的数据都需要加一个长度
			//合包之后，每一个子报文前都是自身长度
			if((buflength + SIZEOF_DATASEGFLAG + SIZEOF_MSGHEADER) > MSG_ENCODE_BUF_LENGTH)
			{
				//如果是大包数据则不需要附带自身长度，只需要将长度放入消息头
				//避免发送Header这样的短报文，因此每一包数据均在前预留Header的空间
				tmpnode->buf = (char*)malloc(buflength+SIZEOF_MSGHEADER);
				bigData = 1;
			}
			else
			{
				tmpnode->buf = (char*)malloc(MSG_ENCODE_BUF_LENGTH); 
			}
			if(tmpnode->buf == NULL)
			{
				free(tmpnode);
				return ERR_TCP_WRITE_TASKLIST_NOMEM;
			}

			if(bigData)
			{
				//首先构造消息头
				EvoTCPMessageHeader msgHeader;
				msgHeader.type = EVO_TCP_MSG_DATA;
				msgHeader.flags = 0;
				msgHeader.octetsToNextHeader = buflength;
				msgHeader.extFlags = 1;
				//编码消息头
				MapMsgHeader(tmpnode->buf , msgHeader);
				memcpy((void*)(tmpnode->buf+SIZEOF_MSGHEADER) , (void*)buf , buflength);
				tmpnode->buflength = buflength+SIZEOF_MSGHEADER;
			}
			else //小包数据，只需要预留Header的空间，最后编码
			{
				if(tmpnode->buflength == 0) //第一个小包数据
				{
					tmpnode->buflength  = SIZEOF_MSGHEADER; //预留Header空间	
				}
				//编码分块长度
				memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)&buflength , SIZEOF_DATASEGFLAG);
				tmpnode->buflength += SIZEOF_DATASEGFLAG;
				memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)buf , buflength);
				tmpnode->buflength += buflength;
			}
			
			tmpnode->type = EVO_TCP_MSG_DATA;
			tmpnode->eleNum = 1; //初始状态下只有一包数据
			tmpnode->next = NULL;
			list->task_head = list->task_tail = tmpnode;
			
		}
		else //第一个不是心跳报文
		{
			//尝试进行合包
			//查看上一包与本包尺寸之和是否大于编码缓冲尺寸
			//此时，上一包的尺寸中已经包含了Header的尺寸
			if((list->task_head->buflength + buflength + SIZEOF_DATASEGFLAG) < MSG_ENCODE_BUF_LENGTH) //可以进行合包
			{
                u_int32_t tmpLength = list->task_head->buflength;
				memcpy((void*)(list->task_head->buf + tmpLength) , (void*)&buflength , SIZEOF_DATASEGFLAG);
				memcpy((void*)(list->task_head->buf + tmpLength + SIZEOF_DATASEGFLAG) , (void*)buf , buflength);
				list->task_head->buflength = list->task_head->buflength + buflength + SIZEOF_DATASEGFLAG;
				list->task_head->eleNum++;
				noMerge = 0;
				//printf("插入合包数据包%d-%d\n" , list->task_head->eleNum , list->task_head->buflength);
			}
			else //不合包，则插入新的节点
			{
				tmpnode = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
				if(tmpnode == 0)
					return ERR_TCP_WRITE_TASKLIST_NOMEM;
				tmpnode->buflength = 0;
				tmpnode->buf = NULL;
				tmpnode->flags = 0;
				tmpnode->next = NULL;
				tmpnode->type = EVO_TCP_MSG_DATA;

				//每一块小于MSG_ENCODE_BUF_LENGTH的数据都需要加一个长度
				//合包之后，每一个子报文前都是自身长度
				if((buflength + SIZEOF_DATASEGFLAG + SIZEOF_MSGHEADER) > MSG_ENCODE_BUF_LENGTH)
				{
					//如果是大包数据则不需要附带自身长度，只需要将长度放入消息头
					//避免发送Header这样的短报文，因此每一包数据均在前预留Header的空间
					tmpnode->buf = (char*)malloc(buflength+SIZEOF_MSGHEADER);
					bigData = 1;
				}
				else
				{
					tmpnode->buf = (char*)malloc(MSG_ENCODE_BUF_LENGTH); 
				}
				if(tmpnode->buf == NULL)
				{
					free(tmpnode);
					return ERR_TCP_WRITE_TASKLIST_NOMEM;
				}
				if(bigData)
				{
					//首先构造消息头
					EvoTCPMessageHeader msgHeader;
					msgHeader.type = EVO_TCP_MSG_DATA;
					msgHeader.flags = 0;
					msgHeader.octetsToNextHeader = buflength;
					msgHeader.extFlags = 1;
					//编码消息头
					MapMsgHeader(tmpnode->buf , msgHeader);
					memcpy((void*)(tmpnode->buf+SIZEOF_MSGHEADER) , (void*)buf , buflength);
					tmpnode->buflength = buflength+SIZEOF_MSGHEADER;
				}
				else //小包数据，只需要预留Header的空间，最后编码
				{
					if(tmpnode->buflength == 0) //第一个小包数据
					{
						tmpnode->buflength  = SIZEOF_MSGHEADER; //预留Header空间	
					}
					//编码分块长度
					memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)&buflength , SIZEOF_DATASEGFLAG);
					tmpnode->buflength += SIZEOF_DATASEGFLAG;
					memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)buf , buflength);
					tmpnode->buflength += buflength;
				}
				//需要注意，如果不是大数据，则最后一定要编码Header
				tmpnode->type = EVO_TCP_MSG_DATA;
				tmpnode->eleNum = 1; //初始状态下一包数据
				tmpnode->next = NULL;

				list->task_tail->next = tmpnode;
				list->task_tail = tmpnode;
			}
		}//第一个不是心跳报文
	} //只有一个报文
	else //已经存在多个报文
	{
		//尝试进行合包
		//查看上一包与本包尺寸之和是否大于编码缓冲尺寸
		if((list->task_tail->buflength + buflength + SIZEOF_DATASEGFLAG) < MSG_ENCODE_BUF_LENGTH) //可以进行合包
		{
            u_int32_t tmpLength = list->task_tail->buflength;
			memcpy((void*)(list->task_tail->buf + tmpLength) , (void*)&buflength , SIZEOF_DATASEGFLAG);
			memcpy((void*)(list->task_tail->buf + tmpLength + SIZEOF_DATASEGFLAG) , (void*)buf , buflength);
			list->task_tail->buflength = list->task_tail->buflength + buflength + SIZEOF_DATASEGFLAG;
			list->task_tail->eleNum++;
			noMerge = 0;
			//printf("插入合包数据包%d-%d\n" , list->task_head->eleNum , list->task_tail->buflength);
		}
		else //不合包，则插入新的节点
		{
			tmpnode = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
			if(tmpnode == 0)
				return ERR_TCP_WRITE_TASKLIST_NOMEM;
			tmpnode->buflength = 0;
			tmpnode->buf = NULL;
			tmpnode->flags = 0;
			tmpnode->next = NULL;
			tmpnode->type = EVO_TCP_MSG_DATA;

			//每一块小于MSG_ENCODE_BUF_LENGTH的数据都需要加一个长度
			//合包之后，每一个子报文前都是自身长度
			if((buflength + SIZEOF_DATASEGFLAG + SIZEOF_MSGHEADER) > MSG_ENCODE_BUF_LENGTH)
			{
				//如果是大包数据则不需要附带自身长度，只需要将长度放入消息头
				//避免发送Header这样的短报文，因此每一包数据均在前预留Header的空间
				tmpnode->buf = (char*)malloc(buflength+SIZEOF_MSGHEADER);
				bigData = 1;
			}
			else
			{
				tmpnode->buf = (char*)malloc(MSG_ENCODE_BUF_LENGTH); 
			}
			if(tmpnode->buf == NULL)
			{
				free(tmpnode);
				return ERR_TCP_WRITE_TASKLIST_NOMEM;
			}
			if(bigData)
			{
				//首先构造消息头
				EvoTCPMessageHeader msgHeader;
				msgHeader.type = EVO_TCP_MSG_DATA;
				msgHeader.flags = 0;
				msgHeader.octetsToNextHeader = buflength;
				msgHeader.extFlags = 1;
				//编码消息头
				MapMsgHeader(tmpnode->buf , msgHeader);
				memcpy((void*)(tmpnode->buf+SIZEOF_MSGHEADER) , (void*)buf , buflength);
				tmpnode->buflength = buflength+SIZEOF_MSGHEADER;
			}
			else //小包数据，只需要预留Header的空间，最后编码
			{
				if(tmpnode->buflength == 0) //第一个小包数据
				{
					tmpnode->buflength  = SIZEOF_MSGHEADER; //预留Header空间	
				}
				//编码分块长度
				memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)&buflength , SIZEOF_DATASEGFLAG);
				tmpnode->buflength += SIZEOF_DATASEGFLAG;
				memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)buf , buflength);
				tmpnode->buflength += buflength;
			}
			//需要注意，如果不是大数据，则最后一定要编码Header
			tmpnode->type = EVO_TCP_MSG_DATA;
			tmpnode->eleNum = 1; //初始状态下一包数据
			tmpnode->next = NULL;

			list->task_tail->next = tmpnode;
			list->task_tail = tmpnode;
		}
	}


	list->count+=noMerge;

	if(bigData)
	{
		list->size+=buflength+SIZEOF_MSGHEADER;
		//printf("插入大数据 : %d\n" , list->task_tail->buflength);
	}
	else if(noMerge)
	{
		EvoTCPMessageHeader msgHeader;
		tmpnode = list->task_tail;
		//编码Header
		//首先构造消息头
		
		msgHeader.type = EVO_TCP_MSG_DATA;
		msgHeader.flags = 1; //存在一种可能性，只有一包子数据，但是为小报数据，没有合包
		msgHeader.octetsToNextHeader = buflength+SIZEOF_DATASEGFLAG;
		msgHeader.extFlags = tmpnode->eleNum;
		//编码消息头
		MapMsgHeader(tmpnode->buf , msgHeader);
		list->size+=buflength+SIZEOF_MSGHEADER+SIZEOF_DATASEGFLAG;
		//printf("插入小数据，不合包 : %d\n" ,list->task_tail->buflength );
	}
	else
	{
		EvoTCPMessageHeader msgHeader;
		tmpnode = list->task_tail;
		//编码Header
		//首先构造消息头
		
		msgHeader.type = EVO_TCP_MSG_DATA;
		msgHeader.flags = 1;
		msgHeader.octetsToNextHeader = list->task_tail->buflength - SIZEOF_MSGHEADER;
		msgHeader.extFlags = tmpnode->eleNum;
		//编码消息头
		MapMsgHeader(list->task_tail->buf , msgHeader);
		list->size+=buflength+SIZEOF_DATASEGFLAG;
		//printf("插入小数据，合包 : %d\n" ,list->task_tail->buflength );
	}

	return list->count;
}

int InsertTCPHeartbeatToList(TCPWriteTaskList* list )
{
	TCPNetworkTask* tmpnode;
	if(list->count == MAX_TCP_WRITE_TASKLIST_LENGTH)
		return ERR_TCP_WRITE_TASKLIST_FULL;
	tmpnode = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
	if(tmpnode == 0)
		return ERR_TCP_WRITE_TASKLIST_NOMEM;
	tmpnode->buf = NULL;
	tmpnode->buflength = 0;
	tmpnode->type = EVO_TCP_MSG_HEARTBEAT;
	tmpnode->next = NULL;

	if(list->task_head == NULL)
	{
		list->task_head = list->task_tail = tmpnode;
		list->count+=1;
		list->size+=SIZEOF_MSGHEADER;
		//printf("准备发送心跳\n");
	}
	
	return list->count;
}

void  SwitchTaskList(TCPWriteTaskList* tgtList , TCPWriteTaskList* srcList)
{

	tgtList->count = srcList->count;
	tgtList->size = srcList->size;
	tgtList->task_head = srcList->task_head;
	tgtList->task_tail = srcList->task_tail;

	srcList->count = 0;
	srcList->size = 0;
	srcList->task_head = srcList->task_tail = NULL;
}