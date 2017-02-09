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
	//���ñ��ص�ַ��		
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

	//���ñ��ص�ַ��		
	if(addr!=NULL)
		tgtaddr.sin_addr.s_addr = inet_addr(addr);
	else
		tgtaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	tgtaddr.sin_family = AF_INET;
	tgtaddr.sin_port = htons(port);

	//��ʼ��������
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

	//����Ϊ������ģʽ
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



//�ú���ִ�н׶��Ѿ��뿪g_tcp_connection_list���ٽ���
//�������ڷ��ͺͶ�ȡ�̷߳��룬���Ա�֤��connectionNode�Ĳ�������������
//һ�г���������漰������Դ��ɾ���������������Ƕ�״̬����λ
//����ɾ��connectionNode�ṹ��ʱ���Ѿ����Ա�֤���ͺͽ����߳̾����뿪�˽ṹ��
TCPNetworkTask* EvoTCPRecv(TCPConnectionNode* connectionNode)
{
	SOCKET socket = connectionNode->socket;
	//�Ѿ�ʵ�ʶ�ȡ���ֽ���
    u_int32_t totalrecvlength = 0;
    u_int32_t tmplen = 0;
    u_int32_t recvlength = 0;
	//�ӱ���ͷ��ȡ�ĺ����������ݱ��ĵ��ܳ���
    u_int32_t datalength = 0;

	//��ȡ����Ƭ�εĳ��ȡ�ÿ�ζ�ȡ�����Ƭ�γ���ΪMSG_ENCODE_BUF_LENGTH
	//���С�ڸó������ȡʵ�ʳ���
    u_int32_t dataseglength = 0;


	//���������������ƣ��������recvcount�ı��������߳���recvsize�ߴ磬���˳��ú���
	//�����ȡ����û�а��������ı��ģ��򱣴������ġ�
    u_int32_t recvcount = 0 , recvsize = 0;

	//�̶�����Ķ�ȡ���棬���÷�Ƭ��ȡ��ÿ�����ó���
	char tmpbuf[MSG_ENCODE_BUF_LENGTH];
	char* pos = tmpbuf;

	//�Ѿ���ȡ�����ݱ��ĵ������������TCP_MAX_RECV_COUNT
	//�������������TCP_MAX_RECV_SIZE
	TCPNetworkTask* tmphead = NULL;
	TCPNetworkTask* tmptail = NULL;
	TCPNetworkTask* tmpnode = NULL;

	EvoTCPMessageHeader tmpMsgHeader;

	//ÿһ��������������һ���������ľ͹��ˣ��������ڶ���ͷ��
	//ʵ���ϣ�������а���������ģ����������������Ҳ����Ҫ��
	//��
    //unsigned char haveHeartbeat = 0;

	while(recvcount<TCP_MAX_RECV_COUNT && recvsize<TCP_MAX_RECV_SIZE)
	{
		//���Ȳ鿴�����Ļ����������ϴ�δ��ɵ����ݽ��ж�ȡ
		if(connectionNode->recvContext.dataLength>0)
		{
			//��ʼ���������Ļ����е���Ϣ��ȡ����
			datalength = connectionNode->recvContext.dataLength;
			totalrecvlength = connectionNode->recvContext.curLength;

			while(totalrecvlength<datalength) //ÿ������ȡ MSG_ENCODE_BUF_LENGTH ���ֽ�
			{
				//ȷ�����ζ�ȡ��Ƭ�γ���
				if((datalength - totalrecvlength)>MSG_ENCODE_BUF_LENGTH) //�����Ҫ��ȡ�ĳ��ȴ��ڻ���������
				{
					dataseglength = MSG_ENCODE_BUF_LENGTH;
				}
				else
				{
					dataseglength = datalength - totalrecvlength;
				}

				//��ʼ��ȡ����Ƭ��
				pos = tmpbuf;
				recvlength = 0;
				tmplen = 0;
				while(recvlength<dataseglength)
				{
					//���ڶ�ȡ��Ϊ�������ٽ������⣬��˿��ܴ�ʱ���������Ѿ�����ʧЧ��
					//�����ִ��ʵ�ʵĶ�ȡ֮ǰ��Ҫ�����ж����ӵ�״̬��
					//���ڶ������ӵ�ɾ���л���վ��ɣ�������Ҫ�ֱ��ж����ͺͽ���״̬��
					//��˼�ʹ���ӳ������ǶԽڵ�Ĳ����ǰ�ȫ�ģ�����Ҫ���ı�ɾ�������
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
							//���������Ļ�����Ȼ���˳�
							break;
						}
						else
						{
							//���������е���Դ�ɼ���̸߳����ͷ�
							connectionNode->curState |= CONNECTION_STATE_RECVERR;
							return NULL;
						}
					}
					else if(tmplen == 0)
					{
						//���������ܷ������Զ������رգ���Ϊ�쳣����
						connectionNode->curState |= CONNECTION_STATE_RECVERR;
						return NULL;
					}
					//���ݶ�ȡ�ɹ�
					connectionNode->heartbeat = 0;
					recvlength+=tmplen;
					totalrecvlength+=tmplen;
					pos+=tmplen;
				}
				//��ȡ��һ��Ƭ�Σ�Ҳ�п�����û�ж�ȡ��
				if(recvlength>0) //�����ȡ������
				{
					memcpy((void*)(connectionNode->recvContext.dataBuf+connectionNode->recvContext.curLength) , 
						(void*)tmpbuf , recvlength);
					connectionNode->recvContext.curLength += recvlength;
					//ͳ���Ѿ���ȡ�ĳߴ�
					recvcount++;
					recvsize+=recvlength;
					if(recvcount>TCP_MAX_RECV_COUNT || recvsize>TCP_MAX_RECV_SIZE)
					{
						//������ݰ���ĺܴ��ֳ������޶�
						break;
					}
				}
				else // �˿���û�����ݿ��Զ�ȡ����Ƭ��û�ж�ȡ�����ݣ�˵����ǰ���ݰ���Ȼû�ж�ȡ����
				{
					//��ﵽ��ȡ�޶ȴ���ʽһ�������������Ļ���֮���˳���
					//�ڸý׶Σ�����������������˵����һ�����������ݰ���û�ж�ȡ�����ݰ�̫�󣬻��߷���̫����
					break;

				}

			} //��ȡ���


			//�ߵ��������ֿ��ܣ�֮ǰ�����ݰ���ȡ��ϣ�����û�ж�ȡ���
			//���û�ж�ȡ��Ҳ�������ֿ��ܣ�������ȡ�޶ȣ�����û�г���
			if(totalrecvlength<datalength) //��Ȼû�ж���
			{
				return NULL;
			}
			else //��ȡ���
			{
				
				tmphead = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
				tmphead->buf = connectionNode->recvContext.dataBuf;
				tmphead->buflength = connectionNode->recvContext.dataLength;
				tmphead->type = EVO_TCP_MSG_DATA;
				tmphead->eleNum = connectionNode->recvContext.eleNum;
				tmphead->flags = connectionNode->recvContext.flags;
				tmphead->next = NULL;
				tmptail = tmphead;
				
				//��յ�ǰ��������
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
			//�������һ���Ƚ����׵���ƣ����ǵ��ϰ����ݶ�ȡ���֮
			//��ֱ���˳���������Ϊ�ڱ���ͷ�Ķ�������в����˼���
			//�ƣ����û�пɶ�ȡ�����ݻᵼ����ѭ��
			return tmphead;
		}

		//����Ϊֹ����һ�ζ�ȡ�����Ѿ���ɣ����洢������������
		//��Ȼ�������һ�ζ�ȡ���������������Ĳ��֣������沿��
		//���벻��ִ�У�ֱ�Ӵ����￪ʼ��ȡ

		//��ʼ��ȡ
		//���ȶ�ȡͷ
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
					//����ͷ���ñȽϼ򻯵�ʵ�ֻ��ƣ����û�ж�ȡ������ѭ����ȡ
					//���û�ж�ȡ�����ݣ���˵���˿�û�����ݿɶ���ֱ���˳�
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
					//���������е���Դ�ɼ���̸߳����ͷ�
					connectionNode->curState |= CONNECTION_STATE_RECVERR;
					return tmphead;
				}
			}
			else if(tmplen == 0)
			{
				//���������ܷ������Զ������رգ���Ϊ�쳣����
				connectionNode->curState |= CONNECTION_STATE_RECVERR;
				return tmphead;
			}
			//���ݶ�ȡ�ɹ�
			connectionNode->heartbeat = 0;
			recvlength+=tmplen;
			pos+=tmplen;
		}

		//��������ͷ���жϱ�������
		tmpMsgHeader = UnMapMsgHeader(tmpbuf);
		
		if(tmpMsgHeader.type == EVO_TCP_MSG_HEARTBEAT)
		{
			recvcount++;
			recvsize+=SIZEOF_MSGHEADER;
			if(tmphead == NULL) //ֻ���ڿն��е�����²ű�����������
			{
				tmphead = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
				tmphead->buf = NULL;
				tmphead->type = EVO_TCP_MSG_HEARTBEAT;
				tmphead->next = NULL;
				tmptail = tmphead;
			}
			//���֮ǰ���ڱ��ģ������������������ȡ��������
			connectionNode->heartbeat = 0;
			continue;
		}
		else if(tmpMsgHeader.type == EVO_TCP_MSG_DATA) //��Ҫ��ȡ��������
		{
			recvcount++;
			recvsize+=SIZEOF_MSGHEADER;

			datalength = tmpMsgHeader.octetsToNextHeader;

			//���������Ļ�������ʱ�����ݱ����������Ļ����У���ȡ���֮����뵽
			//��ȡ�����С�������ζ�ȡ�޶����޷�������ȡ�������ݣ��������´ζ�ȡ��
			//���ϲ㴫��֮ǰ�Ķ�ȡ��������
			//printf("��ȡ���ݣ��ܳ��� %d \n" , datalength);
			connectionNode->recvContext.dataBuf = (char*)malloc(datalength);
			connectionNode->recvContext.flags = tmpMsgHeader.flags;
			connectionNode->recvContext.eleNum = tmpMsgHeader.extFlags;
			//printf("��ȡ���ݣ�����%d��������\n" ,tmpMsgHeader.extFlags);
			connectionNode->recvContext.dataLength = datalength;
			connectionNode->recvContext.curLength = 0;

			//��ʼ��ȡ����
			totalrecvlength = 0;
			while(totalrecvlength<datalength) //ÿ������ȡ MSG_ENCODE_BUF_LENGTH ���ֽ�
			{
				//ȷ�����ζ�ȡ��Ƭ�γ���
				if((datalength - totalrecvlength)>MSG_ENCODE_BUF_LENGTH) //�����Ҫ��ȡ�ĳ��ȴ��ڻ���������
				{
					dataseglength = MSG_ENCODE_BUF_LENGTH;
				}
				else
				{
					dataseglength = datalength - totalrecvlength;
				}

				//��ʼ��ȡ����Ƭ��
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
							//û�п��Զ�ȡ�����ݣ����������Ļ���֮���˳�
							break;
						}
						else
						{
							//���������е���Դ�ɼ���̸߳����ͷ�
							connectionNode->curState |= CONNECTION_STATE_RECVERR;
							return tmphead;

						}
					}
					else if(tmplen == 0)
					{
						//���������ܷ������Զ������رգ���Ϊ�쳣����
						connectionNode->curState |= CONNECTION_STATE_RECVERR;
						return tmphead;
					}
					connectionNode->heartbeat = 0;
					recvlength+=tmplen;
					totalrecvlength+=tmplen;
					pos+=tmplen;
				}
				//�������������һ���Ƕ�ȡ��һ��Ƭ�Σ���һ������;û�����ݿɶ�
				if(recvlength>0)
				{
					memcpy((void*)(connectionNode->recvContext.dataBuf+connectionNode->recvContext.curLength) , 
						(void*)tmpbuf , recvlength);
					connectionNode->recvContext.curLength += recvlength;
				//ͳ���Ѿ���ȡ�ĳߴ�
					recvcount++;
					recvsize+=recvlength;
					if(recvcount>TCP_MAX_RECV_COUNT || recvsize>TCP_MAX_RECV_SIZE)
					{
						break;
					}
				}
				else //����û�ж�ȡ�κ�����
				{
					break;
				}
				
			} //��ȡ���


			//�ߵ��⣬�������ֿ��ܣ�һ�������ݰ���ȡ��ϣ�һ���ǵ������ݶ�ȡ�޶ȣ�����һ���Ƕ˿�û���㹻����
			//����б�Ҫ�������ݴ��������и��Ƴ���
			if(totalrecvlength==datalength)   //�������ݰ���ȡ��������Ҫ�����������ݴ�����ݶ�ȡ����
			{
				
				if(connectionNode->recvContext.dataBuf!=NULL)
				{
					if(tmphead == NULL) //ֻ���ڿն��е�����²ű�����������
					{
						
						tmphead = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
						tmphead->buf = connectionNode->recvContext.dataBuf;
						tmphead->type = EVO_TCP_MSG_DATA;
						tmphead->eleNum = connectionNode->recvContext.eleNum;
						tmphead->flags = connectionNode->recvContext.flags;
						tmphead->buflength = connectionNode->recvContext.dataLength;
						tmphead->next = NULL;
						//printf("�γɽ������ݽڵ㣬���������� : %d\n" , tmphead->eleNum);
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
						//printf("�γɽ������ݽڵ㣬���������� : %d\n" , tmpnode->eleNum);
					}

					//��յ�ǰ��������
					connectionNode->recvContext.curLength = 0;
					connectionNode->recvContext.dataBuf = NULL;
					connectionNode->recvContext.dataLength = 0;
					connectionNode->recvContext.eleNum = 0;
					connectionNode->recvContext.flags = 0;
				}
				
				//����ȥ��ȡ��һ������
			}
			else //��������û�ж�ȡ������˵�������ȡ�޶���߶˿�û�����ݣ������������еĻ����´μ�����ȡ
			{
				return tmphead;
			}
			
		}//else if(msgtype == EVO_TCP_MSG_DATA) 
	}//while(recvcount<TCP_MAX_RECV_COUNT && recvsize<TCP_MAX_RECV_SIZE)

	//�ߵ��⣬ֻ��һ�ֿ��ܣ��������ݶ�ȡ�޶ȣ��������Ļ�����Ϊ��
	return tmphead;

}

u_int32_t EvoTCPSend(TCPConnectionNode* connectionNode)
{
    int hasheartbeat = 0;
	SOCKET socket = connectionNode->socket;
	//���Ƶ��η��͵ı���
    u_int32_t totalSendLength = 0; //�Ѿ����͵����ݳߴ�
    u_int32_t datalength = 0; //�ܹ���Ҫ���͵����ݳߴ�
    u_int32_t dataseglength = 0; //��Ƭ���͵�����£����η�Ƭ�ĳߴ�
    u_int32_t sendLength = 0;
    u_int32_t tmplen = 0;
	char* pos = NULL;

	//���������������Ƶı�����
	//������͹���û�а��������ı��ģ��򱣴������ġ�
    u_int32_t sendcount = 0 , sendsize = 0;

	//�κ�һ�α������ĵ��ù����У�ֻҪ�������ݱ��ģ�����Ҫ��������
	unsigned char needHeartBeat = 1;

	TCPNetworkTask* tmpTask; //���Ӷ�Ӧ�ķ��Ͷ���

	char tmpbuf[MSG_ENCODE_BUF_LENGTH];  //���ͻ�����
	EvoTCPMessageHeader msgHeader; //���ڷ����������ģ�Ŀǰ�Ѳ��������ͱ���ͷ

	//ÿ�η��͵ı��������Լ��ߴ��ܵ�����
	while(sendcount<TCP_MAX_SEND_COUNT && sendsize<TCP_MAX_SEND_SIZE)
	{
		//��ʼ���������Ļ����е���Ϣ��������
		datalength = connectionNode->sendContext.dataLength;
		totalSendLength = connectionNode->sendContext.curLength;

		if(datalength>0) //�����Ҫ������һ�����ݣ�����Ҫ��������
		{
			needHeartBeat = 0;
		}

		while(totalSendLength<datalength) //ÿ������ȡ MSG_ENCODE_BUF_LENGTH ���ֽ�
		{
			//ȷ��������Ҫ��Ƭ�γ���
			if((datalength - totalSendLength)>MSG_ENCODE_BUF_LENGTH) //�����Ҫ���͵ĳ��ȴ��ڷ���Ƭ��
			{
				dataseglength = MSG_ENCODE_BUF_LENGTH; //����һ�����Ƭ��
			}
			else
			{
				dataseglength = datalength - totalSendLength; //����ʣ������
			}

			//��ʼ��������Ƭ��
			pos = connectionNode->sendContext.dataBuf+totalSendLength; //ָ����Ҫ��ʼ���͵�λ��
			sendLength = 0; //����recv�ۼƷ��͵ĳߴ�
			tmplen = 0; //���ε���recv���͵ĳߴ�
			while(sendLength<dataseglength)
			{
				//���ڷ�����Ϊ�������ٽ������⣬���ܴ�ʱ���������Ѿ���ȡʧЧ��
				//�����ִ��ʵ�ʵķ���֮ǰ��Ҫ�����ж����ӵ�״̬��
				//���ڶ������ӵ�ɾ���л���վ��ɣ�������Ҫ�ֱ��ж����ͺͽ���״̬��
				//��˼�ʹ���ӳ������ǶԽڵ�Ĳ����ǰ�ȫ�ģ�����Ҫ���ı�ɾ�������
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
						//���������Ļ�����Ȼ���˳�
						connectionNode->sendBlock = 1;
						break;
					}
					else
					{
						printf("���ʹ��� : %d\n" , err);
						//���������е���Դ�ɼ���̸߳����ͷ�
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

			//������һ��Ƭ�Σ�Ҳ�п�����û�з��ͳɹ�
			if(sendLength>0) //������������ݣ���Ȼ��δ����ȫ������Ƭ��
			{
				connectionNode->sendContext.curLength += sendLength;
				//ͳ���Ѿ����͵ĳߴ�
				sendcount++;
				
				if(sendcount>TCP_MAX_SEND_COUNT || sendsize>TCP_MAX_SEND_SIZE)
				{
					//������ݰ���ĺܴ��ֳ������޶ȣ��´ν��ŷ���
					break;
				}
			}
			else // �˿ڷ����������Զ˽��պܻ���
			{
				//��ﵽ�����޶ȴ���ʽһ�������������Ļ���֮���˳���
				//�ڸý׶Σ�����������������˵����һ�����������ݰ���û�з���
				connectionNode->sendBlock = 1;
				break;
			}

		} //�������


		//�ߵ��������ֿ��ܣ�֮ǰ�����ݰ�������ϣ�����û�з������
		//���û�з�����ϣ�Ҳ�������ֿ��ܣ����������޶ȣ�̫�󣩣�����û������
		if(totalSendLength<datalength) //��Ȼû�з������
		{
			return sendsize;
		}
		else //�������
		{
			connectionNode->outcomingPacketNum+=connectionNode->sendContext.eleNum;
			//��յ�ǰ��������
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
			//��ʼ���ͺ����ı���
			break;
		}
	} //ֻҪ���ͳߴ粻����

	
	//����Ϊֹ����һ�η��Ͳ����Ѿ����
	//��Ȼ�������һ�η��Ͳ��������������Ĳ��֣������沿��
	//���벻��ִ�У�ֱ�Ӵ����￪ʼ���ݷ���
	//��ȡһ����������
	while(connectionNode->curTasks->count)
	{
		tmpTask = connectionNode->curTasks->task_head;
		//printf("�������ݰ���С %d �� �������ݰ� %d\n" , tmpTask->buflength , tmpTask->eleNum);

		if(tmpTask->type == EVO_TCP_MSG_HEARTBEAT)
		{
			hasheartbeat = 1;
			if(!needHeartBeat || tmpTask->next!=NULL)
			{
				//����Ҫ���ͣ��鿴��һ������
				connectionNode->curTasks->task_head = tmpTask->next;
				free(tmpTask);
				connectionNode->curTasks->count--;
				connectionNode->curTasks->size-=SIZEOF_MSGHEADER;
				continue;
			}
			else 
			{
				//������������
				msgHeader.type = EVO_TCP_MSG_HEARTBEAT;
				msgHeader.flags = 0;
				msgHeader.octetsToNextHeader = 0;

				datalength = MapMsgHeader(tmpbuf , msgHeader);

				//������������
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
							//���������������ֶ̱��Ĳ���ѭ���ȴ�
							Sleep(0);
							printf("����������ʱ\n");
							continue;
						}
						else
						{
							printf("���ʹ��� : %d\n" , err);
							//���������е���Դ�ɼ���̸߳����ͷ�
							connectionNode->curState |= CONNECTION_STATE_SENDERR;
							return sendsize;
						}
					}
					sendLength+=tmplen;
					sendsize+=tmplen;
					totalSendLength+=tmplen;
					pos+=tmplen;
				}//������������

				//�������
				sendcount++;
				connectionNode->curTasks->count--;
				connectionNode->curTasks->size-=SIZEOF_MSGHEADER;
				connectionNode->curTasks->task_head = tmpTask->next;
				free(tmpTask);
				
				if(sendcount>TCP_MAX_SEND_COUNT || sendsize>TCP_MAX_SEND_SIZE)
				{
					return sendsize;
				}
			}//��Ҫ������������
		}//������������
		else if(tmpTask->type == EVO_TCP_MSG_DATA)
		{
			//���ݰ����Ѿ������˱���ͷ
			//������ͷ���뵽���ݷ��͹����У�����С�����ݷ���
			//�����ݷ��������Ļ���
			connectionNode->sendContext.curLength = 0;
			connectionNode->sendContext.dataLength = tmpTask->buflength;
			connectionNode->sendContext.dataBuf = tmpTask->buf;
			connectionNode->sendContext.eleNum = tmpTask->eleNum;
			
			//printf("�������ݣ�����%d��������\n" ,msgHeader.extFlags);
	
			//�����Ѿ������������Ļ����У�ɾ������ڵ�
			//��Ӧɾ��buf�ֶ�
			connectionNode->curTasks->count--;
			connectionNode->curTasks->size-=connectionNode->sendContext.dataLength;
			connectionNode->curTasks->task_head = tmpTask->next;
			free(tmpTask);
			//��������ͨ�������Ļ�����¼
			datalength = connectionNode->sendContext.dataLength;
			totalSendLength = 0;

			//���з�Ƭ����
			while(totalSendLength<datalength) //ÿ����෢�� MSG_ENCODE_BUF_LENGTH ���ֽ�
			{
				//ȷ��������Ҫ��Ƭ�γ���
				if((datalength - totalSendLength)>MSG_ENCODE_BUF_LENGTH) //�����Ҫ���͵ĳ��ȴ��ڷ���Ƭ��
				{
					dataseglength = MSG_ENCODE_BUF_LENGTH; //����һ�����Ƭ��
				}
				else
				{
					dataseglength = datalength - totalSendLength; //����ʣ������
				}

				//��ʼ��������Ƭ��
				pos = connectionNode->sendContext.dataBuf+totalSendLength; //ָ����Ҫ��ʼ���͵�λ��
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
							//���������Ļ�����Ȼ���˳�
							connectionNode->sendBlock = 1;
							break;
						}
						else
						{
							printf("���ʹ��� : %d\n" , err);
							//���������е���Դ�ɼ���̸߳����ͷ�
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

				//������һ��Ƭ�Σ�Ҳ�п�����û�з��ͳɹ�
				if(sendLength>0) //������������ݣ���Ȼ��δ����ȫ������Ƭ��
				{
					connectionNode->sendContext.curLength += sendLength;
					//printf("��Ƭ������ϣ��ѷ��� %d \n" ,connectionNode->sendContext.curLength);
					//ͳ���Ѿ����͵ĳߴ�
					sendcount++;

					if(sendcount>TCP_MAX_SEND_COUNT || sendsize>TCP_MAX_SEND_SIZE)
					{
						//������ݰ���ĺܴ󣬳������޶ȣ��˳����͹���
						break;
					}
				}
				else // �˿ڷ����������Զ˽��պܻ���
				{
					//��ﵽ�����޶ȴ���ʽһ�������������Ļ���֮���˳���
					//�ڸý׶Σ�����������������˵����һ�����������ݰ���û�з���
					connectionNode->sendBlock = 1;
					break;
				}

			} //�������

			//�ߵ���һ����������һ�����������ݰ�������ϣ�Ҳ������û�з������
			//���߿��������ݰ�̫�󣬻���������ӵ��
			//����Ѿ�������ϣ������������
			if(connectionNode->sendContext.curLength == connectionNode->sendContext.dataLength)
			{
				connectionNode->outcomingPacketNum+=connectionNode->sendContext.eleNum;
				free(connectionNode->sendContext.dataBuf);
				connectionNode->sendContext.curLength = 0;
				connectionNode->sendContext.dataBuf = NULL;
				connectionNode->sendContext.dataLength = 0;
				connectionNode->sendContext.eleNum = 0;
				//printf("���ݰ��������\n");
			}
			else //û�з�����ϣ��˳�
			{
				return sendsize;
			}

		}//�������ݱ���

	}//ѭ����ȡ�����ӵ���������
	connectionNode->curTasks->task_tail = NULL;
	//����˶Ա�������������ķ���
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
				//����ͷ���ñȽϼ򻯵�ʵ�ֻ��ƣ�ѭ����ȡ
				Sleep(0);
				continue;
			}
			else
			{
				//���������е���Դ�ɼ���̸߳����ͷ�
				return -1;
			}
		}
		recvlength+=tmplen;
		pos+=tmplen;
	}
	return 1;
}