#ifdef _WIN32
#define EvoTCPOperatorLibAPI __declspec(dllexport)
#else
#define EvoTCPOperatorLibAPI
#endif


#include "../Include/EvoTCPCommon.h"

SOCKET InitTCPListener(const char* addr , unsigned short port)
{
	int value = 1;
	SOCKET tmpsocket = socket(AF_INET , SOCK_STREAM , IPPROTO_IP);

	struct sockaddr_in localaddr;
	//设置本地地址族		
	if(addr!=NULL)
		localaddr.sin_addr.s_addr = inet_addr(addr);
	else
		localaddr.sin_addr.s_addr = INADDR_ANY;
	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons(port);

	
	if (setsockopt(tmpsocket, IPPROTO_TCP, TCP_NODELAY, (char *)&value, sizeof(value)) < 0)
	{
		closesocket(tmpsocket);
		return INVALID_SOCKET;
	}

	if (setsockopt(tmpsocket, SOL_SOCKET, SO_REUSEADDR, (char *)&value, sizeof(value)) < 0)
	{
		closesocket(tmpsocket);
		return INVALID_SOCKET;
	}
	value = TCP_BUFFERSIZE;
	if (setsockopt(tmpsocket, SOL_SOCKET, SO_RCVBUF, (char *)&value, sizeof(value)) < 0)
	{
		closesocket(tmpsocket);
		return INVALID_SOCKET;
	}

	if(bind(tmpsocket , (struct sockaddr*)&localaddr , sizeof(localaddr)) == SOCKET_ERROR)
	{
		closesocket(tmpsocket);
		return INVALID_SOCKET;
	}

    listen(tmpsocket , 8);

	return tmpsocket;
}

SOCKET InitActiveConnection(const char* addr , unsigned short port)
{
	int rst = 0;
	struct sockaddr_in tgtaddr;
	
	SOCKET tmpsocket = socket(AF_INET , SOCK_STREAM , IPPROTO_IP);

	//设置本地地址族		
	if(addr!=NULL)
		tgtaddr.sin_addr.s_addr = inet_addr(addr);
	else
		tgtaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	tgtaddr.sin_family = AF_INET;
	tgtaddr.sin_port = htons(port);

	//开始发起连接
	rst = connect(tmpsocket , (struct sockaddr*)&tgtaddr , sizeof(tgtaddr));

	if(rst == SOCKET_ERROR)
	{
        //int err = GetLastError();
        //printf(strerror(err));
		closesocket(tmpsocket);
		return INVALID_SOCKET;
	}

	if(SetConnectionSockOPT(tmpsocket) == -1)
	{
		closesocket(tmpsocket);
		return INVALID_SOCKET;
	}

	return tmpsocket;
}


int SetConnectionSockOPT(SOCKET socket)
{
	int value = 1;
#ifdef _WIN32
    unsigned long mode = 1;
#endif

	if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *)&value, sizeof(value)) < 0)
	{
		return -1;
	}

	if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&value, sizeof(value)) < 0)
	{
		return -1;
	}
	value = TCP_BUFFERSIZE;
	if (setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char *)&value, sizeof(value)) < 0)
	{
		return -1;
	}

	//设置为非阻塞模式
#ifdef _WIN32
    if( ioctlsocket(socket, FIONBIO, &mode)<0)
	{
		return -1;
    }
#else
    int flag = fcntl(socket, F_GETFL, 0);
    if(fcntl(socket, F_SETFL, flag | O_NONBLOCK) < 0)
    {
        return -1;
    }
#endif
	return 1;
}



//该函数执行阶段已经离开g_tcp_connection_list的临界区
//但是由于发送和读取线程分离，可以保证对connectionNode的操作具有排它性
//一切出错处理均不涉及到对资源的删除操作，而仅仅是对状态的置位
//最终删除connectionNode结构的时候，已经可以保证发送和接收线程均已离开此结构。
TCPNetworkTask* EvoTCPRecv(TCPConnectionNode* connectionNode)
{
	SOCKET socket = connectionNode->socket;
	//已经实际读取的字节数
    u_int32_t totalrecvlength = 0;
    u_int32_t tmplen = 0;
    u_int32_t recvlength = 0;
	//从报文头获取的后续完整数据报文的总长度
    u_int32_t datalength = 0;

	//读取报文片段的长度。每次读取的最大片段长度为MSG_ENCODE_BUF_LENGTH
	//如果小于该长度则读取实际长度
    u_int32_t dataseglength = 0;


	//用来进行流量控制，如果超过recvcount的报文数或者超过recvsize尺寸，则退出该函数
	//如果读取过程没有包含完整的报文，则保存上下文。
    u_int32_t recvcount = 0 , recvsize = 0;

	//固定分配的读取缓存，采用分片读取，每次最多该长度
	char tmpbuf[MSG_ENCODE_BUF_LENGTH];
	char* pos = tmpbuf;

	//已经读取的数据报文的链表，该链表最长TCP_MAX_RECV_COUNT
	//链表中数据最多TCP_MAX_RECV_SIZE
	TCPNetworkTask* tmphead = NULL;
	TCPNetworkTask* tmptail = NULL;
	TCPNetworkTask* tmpnode = NULL;

	EvoTCPMessageHeader tmpMsgHeader;

	//每一组数据中最多包含一个心跳包文就够了，而且是在队列头部
	//实际上，如果队列包含多个报文，则连这个心跳报文也不需要保
	//留
    //unsigned char haveHeartbeat = 0;

	while(recvcount<TCP_MAX_RECV_COUNT && recvsize<TCP_MAX_RECV_SIZE)
	{
		//首先查看上下文环境，接着上次未完成的数据进行读取
		if(connectionNode->recvContext.dataLength>0)
		{
			//开始根据上下文环境中的信息读取数据
			datalength = connectionNode->recvContext.dataLength;
			totalrecvlength = connectionNode->recvContext.curLength;

			while(totalrecvlength<datalength) //每次最多读取 MSG_ENCODE_BUF_LENGTH 个字节
			{
				//确定本次读取的片段长度
				if((datalength - totalrecvlength)>MSG_ENCODE_BUF_LENGTH) //如果需要读取的长度大于缓冲区长度
				{
					dataseglength = MSG_ENCODE_BUF_LENGTH;
				}
				else
				{
					dataseglength = datalength - totalrecvlength;
				}

				//开始读取数据片段
				pos = tmpbuf;
				recvlength = 0;
				tmplen = 0;
				while(recvlength<dataseglength)
				{
					//由于读取行为发生在临界区意外，因此可能此时该了连接已经发送失效，
					//因此在执行实际的读取之前需要首先判断连接的状态。
					//由于对于连接的删除有回收站完成，并且需要分别判定发送和接收状态，
					//因此即使连接出错，但是对节点的操作是安全的，不需要担心被删除的情况
					if(connectionNode->curState & CONNECTION_STATE_SENDERR ||  connectionNode->curState & CONNECTION_STATE_INVALID)
					{
						connectionNode->curState |= CONNECTION_STATE_RECVERR;
						return NULL;
					}
					if((tmplen = recv(socket , pos , dataseglength - recvlength , 0)) == SOCKET_ERROR)
					{
						int err = GetLastError();
						if(err == WSAEWOULDBLOCK)
						{
							//设置上下文环境，然后退出
							break;
						}
						else
						{
							//出错处理，所有的资源由监控线程负责释放
							connectionNode->curState |= CONNECTION_STATE_RECVERR;
							return NULL;
						}
					}
					else if(tmplen == 0)
					{
						//几乎不可能发生，对端主动关闭，作为异常处理
						connectionNode->curState |= CONNECTION_STATE_RECVERR;
						return NULL;
					}
					//数据读取成功
					connectionNode->heartbeat = 0;
					recvlength+=tmplen;
					totalrecvlength+=tmplen;
					pos+=tmplen;
				}
				//读取了一个片段，也有可能是没有读取到
				if(recvlength>0) //如果读取到数据
				{
					memcpy((void*)(connectionNode->recvContext.dataBuf+connectionNode->recvContext.curLength) , 
						(void*)tmpbuf , recvlength);
					connectionNode->recvContext.curLength += recvlength;
					//统计已经读取的尺寸
					recvcount++;
					recvsize+=recvlength;
					if(recvcount>TCP_MAX_RECV_COUNT || recvsize>TCP_MAX_RECV_SIZE)
					{
						//这个数据包真的很大，又超出了限度
						break;
					}
				}
				else // 端口上没有数据可以读取，本片断没有读取到数据，说明当前数据包仍然没有读取完整
				{
					//与达到读取限度处理方式一样，保存上下文环境之后退出。
					//在该阶段，如果发生以上情况，说明连一个完整的数据包都没有读取（数据包太大，或者发送太慢）
					break;

				}

			} //读取完毕


			//走到这有两种可能，之前的数据包读取完毕，或者没有读取完毕
			//如果没有读取，也存在两种可能，超过读取限度，或者没有超过
			if(totalrecvlength<datalength) //仍然没有读完
			{
				return NULL;
			}
			else //读取完毕
			{
				
				tmphead = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
				tmphead->buf = connectionNode->recvContext.dataBuf;
				tmphead->buflength = connectionNode->recvContext.dataLength;
				tmphead->type = EVO_TCP_MSG_DATA;
				tmphead->eleNum = connectionNode->recvContext.eleNum;
				tmphead->flags = connectionNode->recvContext.flags;
				tmphead->next = NULL;
				tmptail = tmphead;
				
				//清空当前的上下文
				connectionNode->recvContext.curLength = 0;
				connectionNode->recvContext.dataBuf = NULL;
				connectionNode->recvContext.dataLength = 0;
				connectionNode->recvContext.eleNum = 0;
				connectionNode->recvContext.flags = 0;
				if(recvcount>TCP_MAX_RECV_COUNT || recvsize>TCP_MAX_RECV_SIZE)
				{
					return tmphead;
				}
			}
			//这里采用一个比较稳妥的设计，就是当上包数据读取完毕之
			//后直接退出。这是因为在报文头的都区设计中采用了简化设
			//计，如果没有可读取的数据会导致死循环
			return tmphead;
		}

		//到此为止，上一次读取操作已经完成，并存储在数据链表中
		//当然，如果上一次读取操作不存在遗留的部分，则上面部分
		//代码不会执行，直接从这里开始读取

		//开始读取
		//首先读取头
		recvlength = 0;
		tmplen = 0;
		pos = tmpbuf;
		while(recvlength<SIZEOF_MSGHEADER)
		{
			if(connectionNode->curState & CONNECTION_STATE_SENDERR ||  connectionNode->curState & CONNECTION_STATE_INVALID)
			{
				connectionNode->curState |= CONNECTION_STATE_RECVERR;
				return NULL;
			}
			if((tmplen = recv(socket , pos , SIZEOF_MSGHEADER - recvlength , 0)) == SOCKET_ERROR)
			{
				int err = GetLastError();
				if(err == WSAEWOULDBLOCK)
				{
					//报文头采用比较简化的实现机制，如果没有读取完整则循环读取
					//如果没有读取到数据，则说明端口没有数据可读，直接退出
					if(recvlength>0)
					{
						Sleep(0);
						continue;
					}
					else
					{
						return tmphead;
					}
					
				}
				else
				{
					//出错处理，所有的资源由监控线程负责释放
					connectionNode->curState |= CONNECTION_STATE_RECVERR;
					return tmphead;
				}
			}
			else if(tmplen == 0)
			{
				//几乎不可能发生，对端主动关闭，作为异常处理
				connectionNode->curState |= CONNECTION_STATE_RECVERR;
				return tmphead;
			}
			//数据读取成功
			connectionNode->heartbeat = 0;
			recvlength+=tmplen;
			pos+=tmplen;
		}

		//分析报文头，判断报文类型
		tmpMsgHeader = UnMapMsgHeader(tmpbuf);
		
		if(tmpMsgHeader.type == EVO_TCP_MSG_HEARTBEAT)
		{
			recvcount++;
			recvsize+=SIZEOF_MSGHEADER;
			if(tmphead == NULL) //只有在空队列的情况下才保留心跳报文
			{
				tmphead = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
				tmphead->buf = NULL;
				tmphead->type = EVO_TCP_MSG_HEARTBEAT;
				tmphead->next = NULL;
				tmptail = tmphead;
			}
			//如果之前存在报文，则忽略心跳，继续读取后续报文
			connectionNode->heartbeat = 0;
			continue;
		}
		else if(tmpMsgHeader.type == EVO_TCP_MSG_DATA) //需要读取后续数据
		{
			recvcount++;
			recvsize+=SIZEOF_MSGHEADER;

			datalength = tmpMsgHeader.octetsToNextHeader;

			//建立上下文环境，暂时将数据保存在上下文环境中，读取完毕之后插入到
			//读取队列中。如果本次读取限度内无法完整读取本包数据，则留待下次读取，
			//向上层传递之前的读取队列内容
			//printf("获取数据，总长度 %d \n" , datalength);
			connectionNode->recvContext.dataBuf = (char*)malloc(datalength);
			connectionNode->recvContext.flags = tmpMsgHeader.flags;
			connectionNode->recvContext.eleNum = tmpMsgHeader.extFlags;
			//printf("获取数据，包含%d个子数据\n" ,tmpMsgHeader.extFlags);
			connectionNode->recvContext.dataLength = datalength;
			connectionNode->recvContext.curLength = 0;

			//开始读取数据
			totalrecvlength = 0;
			while(totalrecvlength<datalength) //每次最多读取 MSG_ENCODE_BUF_LENGTH 个字节
			{
				//确定本次读取的片段长度
				if((datalength - totalrecvlength)>MSG_ENCODE_BUF_LENGTH) //如果需要读取的长度大于缓冲区长度
				{
					dataseglength = MSG_ENCODE_BUF_LENGTH;
				}
				else
				{
					dataseglength = datalength - totalrecvlength;
				}

				//开始读取数据片段
				pos = tmpbuf;
				recvlength = 0;
				tmplen = 0;
				while(recvlength<dataseglength)
				{
					if(connectionNode->curState & CONNECTION_STATE_SENDERR ||  connectionNode->curState & CONNECTION_STATE_INVALID)
					{
						connectionNode->curState |= CONNECTION_STATE_RECVERR;
						return NULL;
					}
					if((tmplen = recv(socket , pos , dataseglength - recvlength , 0)) == SOCKET_ERROR)
					{
						int err = GetLastError();
						if(err == WSAEWOULDBLOCK)
						{
							//没有可以读取的数据，保存上下文环境之后退出
							break;
						}
						else
						{
							//出错处理，所有的资源由监控线程负责释放
							connectionNode->curState |= CONNECTION_STATE_RECVERR;
							return tmphead;

						}
					}
					else if(tmplen == 0)
					{
						//几乎不可能发生，对端主动关闭，作为异常处理
						connectionNode->curState |= CONNECTION_STATE_RECVERR;
						return tmphead;
					}
					connectionNode->heartbeat = 0;
					recvlength+=tmplen;
					totalrecvlength+=tmplen;
					pos+=tmplen;
				}
				//存在两种情况，一种是读取了一个片段，另一种是中途没有数据可读
				if(recvlength>0)
				{
					memcpy((void*)(connectionNode->recvContext.dataBuf+connectionNode->recvContext.curLength) , 
						(void*)tmpbuf , recvlength);
					connectionNode->recvContext.curLength += recvlength;
				//统计已经读取的尺寸
					recvcount++;
					recvsize+=recvlength;
					if(recvcount>TCP_MAX_RECV_COUNT || recvsize>TCP_MAX_RECV_SIZE)
					{
						break;
					}
				}
				else //本次没有读取任何数据
				{
					break;
				}
				
			} //读取完毕


			//走到这，存在三种可能，一种是数据包读取完毕，一种是到达数据读取限度，还有一种是端口没有足够数据
			//如果有必要，则将数据从上下文中复制出来
			if(totalrecvlength==datalength)   //整个数据包读取完整，需要将上下文中暂存的数据读取出来
			{
				
				if(connectionNode->recvContext.dataBuf!=NULL)
				{
					if(tmphead == NULL) //只有在空队列的情况下才保留心跳报文
					{
						
						tmphead = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
						tmphead->buf = connectionNode->recvContext.dataBuf;
						tmphead->type = EVO_TCP_MSG_DATA;
						tmphead->eleNum = connectionNode->recvContext.eleNum;
						tmphead->flags = connectionNode->recvContext.flags;
						tmphead->buflength = connectionNode->recvContext.dataLength;
						tmphead->next = NULL;
						//printf("形成接收数据节点，包含子数据 : %d\n" , tmphead->eleNum);
						tmptail = tmphead;
					}
					else
					{
						tmpnode = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
						tmpnode->buf = connectionNode->recvContext.dataBuf;
						tmpnode->type = EVO_TCP_MSG_DATA;
						tmpnode->eleNum = connectionNode->recvContext.eleNum;
						tmpnode->flags = connectionNode->recvContext.flags;
						tmpnode->buflength = connectionNode->recvContext.dataLength;
						tmpnode->next = NULL;
						tmptail->next = tmpnode;
						tmptail = tmpnode;
						//printf("形成接收数据节点，包含子数据 : %d\n" , tmpnode->eleNum);
					}

					//清空当前的上下文
					connectionNode->recvContext.curLength = 0;
					connectionNode->recvContext.dataBuf = NULL;
					connectionNode->recvContext.dataLength = 0;
					connectionNode->recvContext.eleNum = 0;
					connectionNode->recvContext.flags = 0;
				}
				
				//继续去读取下一个报文
			}
			else //本包数据没有读取完整，说明到达读取限额，或者端口没有数据，保存上下文中的环境下次继续读取
			{
				return tmphead;
			}
			
		}//else if(msgtype == EVO_TCP_MSG_DATA) 
	}//while(recvcount<TCP_MAX_RECV_COUNT && recvsize<TCP_MAX_RECV_SIZE)

	//走到这，只有一种可能，到大数据读取限度，且上下文环境中为空
	return tmphead;

}

u_int32_t EvoTCPSend(TCPConnectionNode* connectionNode)
{
    int hasheartbeat = 0;
	SOCKET socket = connectionNode->socket;
	//控制单次发送的变量
    u_int32_t totalSendLength = 0; //已经发送的数据尺寸
    u_int32_t datalength = 0; //总共需要发送的数据尺寸
    u_int32_t dataseglength = 0; //分片发送的情况下，本次分片的尺寸
    u_int32_t sendLength = 0;
    u_int32_t tmplen = 0;
	char* pos = NULL;

	//用来进行流量控制的变量，
	//如果发送过程没有包含完整的报文，则保存上下文。
    u_int32_t sendcount = 0 , sendsize = 0;

	//任何一次本函数的调用过程中，只要存在数据报文，则不需要心跳报文
	unsigned char needHeartBeat = 1;

	TCPNetworkTask* tmpTask; //连接对应的发送队列

	char tmpbuf[MSG_ENCODE_BUF_LENGTH];  //发送缓冲区
	EvoTCPMessageHeader msgHeader; //用于发送心跳报文，目前已不单独发送报文头

	//每次发送的报文数量以及尺寸受到限制
	while(sendcount<TCP_MAX_SEND_COUNT && sendsize<TCP_MAX_SEND_SIZE)
	{
		//开始根据上下文环境中的信息发送数据
		datalength = connectionNode->sendContext.dataLength;
		totalSendLength = connectionNode->sendContext.curLength;

		if(datalength>0) //如果需要发送上一包数据，则不需要发送心跳
		{
			needHeartBeat = 0;
		}

		while(totalSendLength<datalength) //每次最多读取 MSG_ENCODE_BUF_LENGTH 个字节
		{
			//确定本次需要的片段长度
			if((datalength - totalSendLength)>MSG_ENCODE_BUF_LENGTH) //如果需要发送的长度大于发送片段
			{
				dataseglength = MSG_ENCODE_BUF_LENGTH; //发送一个完成片段
			}
			else
			{
				dataseglength = datalength - totalSendLength; //发送剩余数据
			}

			//开始发送数据片段
			pos = connectionNode->sendContext.dataBuf+totalSendLength; //指向需要开始发送的位置
			sendLength = 0; //调用recv累计发送的尺寸
			tmplen = 0; //单次调用recv发送的尺寸
			while(sendLength<dataseglength)
			{
				//由于发送行为发生在临界区意外，可能此时该了连接已经读取失效，
				//因此在执行实际的发送之前需要首先判断连接的状态。
				//由于对于连接的删除有回收站完成，并且需要分别判定发送和接收状态，
				//因此即使连接出错，但是对节点的操作是安全的，不需要担心被删除的情况
				if(connectionNode->curState & CONNECTION_STATE_RECVERR ||  connectionNode->curState & CONNECTION_STATE_INVALID)
				{
					connectionNode->curState |= CONNECTION_STATE_SENDERR;
					return sendsize;
				}
				if((tmplen = send(socket , pos , dataseglength - sendLength , 0)) == SOCKET_ERROR)
				{
					
					int err = GetLastError();
					if(err == WSAEWOULDBLOCK)
					{
						//设置上下文环境，然后退出
						connectionNode->sendBlock = 1;
						break;
					}
					else
					{
						printf("发送错误 : %d\n" , err);
						//出错处理，所有的资源由监控线程负责释放
						connectionNode->curState |= CONNECTION_STATE_SENDERR;
						return sendsize;
					}
				}
				sendLength+=tmplen;
				sendsize+=tmplen;
				totalSendLength+=tmplen;
				pos+=tmplen;
			}

			if(connectionNode->sendBlock == 1)
			{
				break;
			}

			//发送了一个片段，也有可能是没有发送成功
			if(sendLength>0) //如果发送了数据，当然，未必是全部数据片段
			{
				connectionNode->sendContext.curLength += sendLength;
				//统计已经发送的尺寸
				sendcount++;
				
				if(sendcount>TCP_MAX_SEND_COUNT || sendsize>TCP_MAX_SEND_SIZE)
				{
					//这个数据包真的很大，又超出了限度，下次接着发送
					break;
				}
			}
			else // 端口发生阻塞，对端接收很缓慢
			{
				//与达到发送限度处理方式一样，保存上下文环境之后退出。
				//在该阶段，如果发生以上情况，说明连一个完整的数据包都没有发送
				connectionNode->sendBlock = 1;
				break;
			}

		} //发送完毕


		//走到这有两种可能，之前的数据包发送完毕，或者没有发送完毕
		//如果没有发送完毕，也存在两种可能，超过发送限度（太大），或者没有阻塞
		if(totalSendLength<datalength) //仍然没有发送完毕
		{
			return sendsize;
		}
		else //发送完毕
		{
			connectionNode->outcomingPacketNum+=connectionNode->sendContext.eleNum;
			//清空当前的上下文
			connectionNode->sendContext.curLength = 0;
			if(connectionNode->sendContext.dataBuf!=NULL)
				free(connectionNode->sendContext.dataBuf);
			connectionNode->sendContext.dataBuf = NULL;
			connectionNode->sendContext.dataLength = 0;
			connectionNode->sendContext.eleNum = 0;

			if(sendcount>TCP_MAX_SEND_COUNT || sendsize>TCP_MAX_SEND_SIZE)
			{
				return sendsize;
			}
			//开始发送后续的报文
			break;
		}
	} //只要发送尺寸不超出

	
	//到此为止，上一次发送操作已经完成
	//当然，如果上一次发送操作不存在遗留的部分，则上面部分
	//代码不会执行，直接从这里开始数据发送
	//获取一个发送任务
	while(connectionNode->curTasks->count)
	{
		tmpTask = connectionNode->curTasks->task_head;
		//printf("发送数据包大小 %d ， 包含数据包 %d\n" , tmpTask->buflength , tmpTask->eleNum);

		if(tmpTask->type == EVO_TCP_MSG_HEARTBEAT)
		{
			hasheartbeat = 1;
			if(!needHeartBeat || tmpTask->next!=NULL)
			{
				//不需要发送，查看下一个任务
				connectionNode->curTasks->task_head = tmpTask->next;
				free(tmpTask);
				connectionNode->curTasks->count--;
				connectionNode->curTasks->size-=SIZEOF_MSGHEADER;
				continue;
			}
			else 
			{
				//发送心跳报文
				msgHeader.type = EVO_TCP_MSG_HEARTBEAT;
				msgHeader.flags = 0;
				msgHeader.octetsToNextHeader = 0;

				datalength = MapMsgHeader(tmpbuf , msgHeader);

				//发送心跳报文
				sendLength = 0;
				tmplen = 0;
				pos = tmpbuf;
				while(sendLength<datalength)
				{
					if(connectionNode->curState & CONNECTION_STATE_RECVERR ||  connectionNode->curState & CONNECTION_STATE_INVALID)
					{
						connectionNode->curState |= CONNECTION_STATE_SENDERR;
						return sendsize;
					}
					if((tmplen = send(socket , pos , datalength - sendLength , 0)) == SOCKET_ERROR)
					{
						int err = GetLastError();
						if(err == WSAEWOULDBLOCK)
						{
							//对于心跳包文这种短报文采用循环等待
							Sleep(0);
							printf("发送心跳超时\n");
							continue;
						}
						else
						{
							printf("发送错误 : %d\n" , err);
							//出错处理，所有的资源由监控线程负责释放
							connectionNode->curState |= CONNECTION_STATE_SENDERR;
							return sendsize;
						}
					}
					sendLength+=tmplen;
					sendsize+=tmplen;
					totalSendLength+=tmplen;
					pos+=tmplen;
				}//发送心跳报文

				//发送完毕
				sendcount++;
				connectionNode->curTasks->count--;
				connectionNode->curTasks->size-=SIZEOF_MSGHEADER;
				connectionNode->curTasks->task_head = tmpTask->next;
				free(tmpTask);
				
				if(sendcount>TCP_MAX_SEND_COUNT || sendsize>TCP_MAX_SEND_SIZE)
				{
					return sendsize;
				}
			}//需要发送心跳报文
		}//处理心跳报文
		else if(tmpTask->type == EVO_TCP_MSG_DATA)
		{
			//数据包中已经包含了报文头
			//将报文头纳入到数据发送过程中，避免小包数据发送
			//将数据放入上下文环境
			connectionNode->sendContext.curLength = 0;
			connectionNode->sendContext.dataLength = tmpTask->buflength;
			connectionNode->sendContext.dataBuf = tmpTask->buf;
			connectionNode->sendContext.eleNum = tmpTask->eleNum;
			
			//printf("发送数据，包含%d个子数据\n" ,msgHeader.extFlags);
	
			//数据已经保存在上下文环境中，删除任务节点
			//不应删除buf字段
			connectionNode->curTasks->count--;
			connectionNode->curTasks->size-=connectionNode->sendContext.dataLength;
			connectionNode->curTasks->task_head = tmpTask->next;
			free(tmpTask);
			//后续发送通过上下文环境记录
			datalength = connectionNode->sendContext.dataLength;
			totalSendLength = 0;

			//进行分片传输
			while(totalSendLength<datalength) //每次最多发送 MSG_ENCODE_BUF_LENGTH 个字节
			{
				//确定本次需要的片段长度
				if((datalength - totalSendLength)>MSG_ENCODE_BUF_LENGTH) //如果需要发送的长度大于发送片段
				{
					dataseglength = MSG_ENCODE_BUF_LENGTH; //发送一个完成片段
				}
				else
				{
					dataseglength = datalength - totalSendLength; //发送剩余数据
				}

				//开始发送数据片段
				pos = connectionNode->sendContext.dataBuf+totalSendLength; //指向需要开始发送的位置
				sendLength = 0;
				tmplen = 0;
				while(sendLength<dataseglength)
				{
					if(connectionNode->curState & CONNECTION_STATE_RECVERR ||  connectionNode->curState & CONNECTION_STATE_INVALID)
					{
						connectionNode->curState |= CONNECTION_STATE_SENDERR;
						return sendsize;
					}
					if((tmplen = send(socket , pos , dataseglength - sendLength , 0)) == SOCKET_ERROR)
					{
	
						int err = GetLastError();
						if(err == WSAEWOULDBLOCK)
						{
							//设置上下文环境，然后退出
							connectionNode->sendBlock = 1;
							break;
						}
						else
						{
							printf("发送错误 : %d\n" , err);
							//出错处理，所有的资源由监控线程负责释放
							connectionNode->curState |= CONNECTION_STATE_SENDERR;
							return sendsize;
						}
					}
					sendLength+=tmplen;
					sendsize+=tmplen;
					totalSendLength+=tmplen;
					pos+=tmplen;
				}

				if(connectionNode->sendBlock == 1)
					break;

				//发送了一个片段，也有可能是没有发送成功
				if(sendLength>0) //如果发送了数据，当然，未必是全部数据片段
				{
					connectionNode->sendContext.curLength += sendLength;
					//printf("分片发送完毕，已发送 %d \n" ,connectionNode->sendContext.curLength);
					//统计已经发送的尺寸
					sendcount++;

					if(sendcount>TCP_MAX_SEND_COUNT || sendsize>TCP_MAX_SEND_SIZE)
					{
						//这个数据包真的很大，超出了限度，退出发送过程
						break;
					}
				}
				else // 端口发生阻塞，对端接收很缓慢
				{
					//与达到发送限度处理方式一样，保存上下文环境之后退出。
					//在该阶段，如果发生以上情况，说明连一个完整的数据包都没有发送
					connectionNode->sendBlock = 1;
					break;
				}

			} //发送完毕

			//走到这一步，可能是一个完整的数据包发送完毕，也可能是没有发送完毕
			//后者可能是数据包太大，或者是网络拥塞
			//如果已经发送完毕，则清空上下文
			if(connectionNode->sendContext.curLength == connectionNode->sendContext.dataLength)
			{
				connectionNode->outcomingPacketNum+=connectionNode->sendContext.eleNum;
				free(connectionNode->sendContext.dataBuf);
				connectionNode->sendContext.curLength = 0;
				connectionNode->sendContext.dataBuf = NULL;
				connectionNode->sendContext.dataLength = 0;
				connectionNode->sendContext.eleNum = 0;
				//printf("数据包发送完毕\n");
			}
			else //没有发送完毕，退出
			{
				return sendsize;
			}

		}//处理数据报文

	}//循环读取该链接的所有任务
	connectionNode->curTasks->task_tail = NULL;
	//完成了对本连接所有任务的发送
	return sendsize;

}

int EvoTCPRecvInterupt(SOCKET socket)
{
	int recvlength = 0;
	int tmplen = 0;
	char tmpbuf[SIZEOF_MSGHEADER];
	char* pos = tmpbuf;
	while(recvlength<SIZEOF_MSGHEADER)
	{
		if((tmplen = recv(socket , pos , SIZEOF_MSGHEADER - recvlength , 0)) == SOCKET_ERROR)
		{
			int err = GetLastError();
			if(err == WSAEWOULDBLOCK)
			{
				//报文头采用比较简化的实现机制，循环读取
				Sleep(0);
				continue;
			}
			else
			{
				//出错处理，所有的资源由监控线程负责释放
				return -1;
			}
		}
		recvlength+=tmplen;
		pos+=tmplen;
	}
	return 1;
}